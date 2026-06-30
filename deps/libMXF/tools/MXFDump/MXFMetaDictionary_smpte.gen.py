#!/usr/bin/env python3
"""
Code generator for MXFDump.cpp dictionary of SMPTE Metadata Registers.
Translate the SMPTE Metadata Registers (Groups + Elements + Essence + Types
XML files) into MXFDump dictionary format.

Reads the official SMPTE register XMLs published at
https://smpte-ra.org/smpte-metadata-registry and emits a header that uses
the ``MXF_CLASS`` / ``MXF_PROPERTY`` / ``MXF_CLASS_END`` macros consumed by
``MXFDump.cpp``.

Note that the Draft registers can be preferred over the published ones.

The generated header is included directly from ``MXFDump.cpp`` alongside
``MXFMetaDictionary_smpte.h`` (the hand-maintained complement) when CMake
sees a file named ``MXFMetaDictionary_smpte.gen.h`` next to ``CMakeLists.txt``.

Run manually before building. With no arguments it downloads the registers
straight from the SMPTE registry (Groups/Elements/Types from the *draft*
tree, Essence from the *published* tree, since the draft tree has no Essence
register), caches them under the system temp dir (``<tempdir>/mxfdump_smpte/``)
and writes ``MXFMetaDictionary_smpte.gen.h`` into the same folder as this
script::

    py -3 deps/libMXF/tools/MXFDump/MXFMetaDictionary_smpte.gen.py

To use local XML files instead of downloading, pass all five register paths
(if you pass one, you must pass all five)::

    py -3 deps/libMXF/tools/MXFDump/MXFMetaDictionary_smpte.gen.py \\
        --groups   Groups.xml \\
        --elements Elements.xml \\
        --essence  Essence.xml \\
        --types    Types.xml \\
        --labels   Labels.xml
"""

from __future__ import annotations

import argparse
import re
import sys
import tempfile
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path


# ---------------------------------------------------------------------------
# Source registers. The draft tree is preferred where available; the Essence
# register only exists in the published tree.
_DRAFT_BASE     = "https://registry.smpte-ra.org/view/draft"
_PUBLISHED_BASE = "https://registry.smpte-ra.org/view/published"
REGISTER_URLS: dict[str, str] = {
    "groups":   f"{_DRAFT_BASE}/Groups.xml",
    "elements": f"{_DRAFT_BASE}/Elements.xml",
    "types":    f"{_DRAFT_BASE}/Types.xml",
    "essence":  f"{_PUBLISHED_BASE}/Essence.xml",
    "labels":   f"{_DRAFT_BASE}/Labels.xml",
}


def download_registers(dest_dir: str | Path) -> dict[str, str]:
    """Download every register in ``REGISTER_URLS`` into ``dest_dir``.

    Self-contained: uses only the standard library (``urllib``). Returns a
    mapping of register key (``groups``/``elements``/``types``/``essence``) to
    the local file path it was written to. Aborts the program on any failure
    so a partial/stale set is never silently fed to the generator.
    """
    dest = Path(dest_dir)
    dest.mkdir(parents=True, exist_ok=True)
    paths: dict[str, str] = {}
    for key, url in REGISTER_URLS.items():
        target = dest / Path(url).name
        print(f"Downloading {url}\n          -> {target}")
        req = urllib.request.Request(
            url, headers={"User-Agent": "mxfdump-smpte-dict-gen/1.0"}
        )
        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                data = resp.read()
        except (urllib.error.URLError, OSError) as exc:
            raise SystemExit(f"error: failed to download {url}: {exc}")
        target.write_bytes(data)
        paths[key] = str(target)
    return paths


# ---------------------------------------------------------------------------
def strip_ns(tag: str) -> str:
    """Return local name from a namespaced tag like ``{ns}Entry``."""
    return tag.split("}", 1)[1] if "}" in tag else tag


def child_text(entry: ET.Element, name: str) -> str | None:
    for c in entry:
        if strip_ns(c.tag) == name:
            return (c.text or "").strip()
    return None


def child(entry: ET.Element, name: str) -> ET.Element | None:
    for c in entry:
        if strip_ns(c.tag) == name:
            return c
    return None


def iter_entries(root: ET.Element):
    """Yield every <Entry> regardless of namespace, in document order."""
    for elem in root.iter():
        if strip_ns(elem.tag) == "Entry":
            yield elem


_UL_RE = re.compile(
    r"urn:smpte:ul:([0-9a-fA-F]{8})\.([0-9a-fA-F]{8})\.([0-9a-fA-F]{8})\.([0-9a-fA-F]{8})"
)


def parse_ul(urn: str) -> list[int] | None:
    """Convert ``urn:smpte:ul:AAAAAAAA.BBBBBBBB.CCCCCCCC.DDDDDDDD`` to 16 bytes."""
    if not urn:
        return None
    m = _UL_RE.search(urn)
    if not m:
        return None
    hex_str = "".join(m.groups())
    return [int(hex_str[i:i + 2], 16) for i in range(0, 32, 2)]


def parse_local_tag(s: str | None) -> int | None:
    """Parse a register ``<LocalTag>`` value (4 hex digits, e.g. ``3f06``).

    Returns the tag as an int, or None when absent/unparsable. A record with no
    LocalTag keeps the 0x0000 sentinel, which MXFDump fills in dynamically from
    the Primer Pack. Registered static tags (< 0x8000) are globally unique per
    UL in the register, so emitting them is safe for MXFDump's global,
    first-match local-key lookup."""
    if not s:
        return None
    try:
        return int(s.strip(), 16)
    except ValueError:
        return None


def ul_key(b: list[int]) -> bytes:
    """Hashable key for an MXF UL ignoring registry-version differences."""
    return bytes(b) if not b else bytes([b[0], b[1], b[2], b[3],
                                         b[4], 0x00, b[6], 0x00] + b[8:])


def fmt_label(bytes16: list[int], indent: str) -> str:
    head = ", ".join(f"0x{b:02x}" for b in bytes16[:8])
    tail = ", ".join(f"0x{b:02x}" for b in bytes16[8:])
    return f"MXF_LABEL({head},\n{indent}          {tail})"


def sanitize_token(sym: str) -> str:
    """Reduce a register Symbol to a single safe C token.

    The MXF_PROPERTY ``type`` argument must be one preprocessing token or it
    breaks the macro arity. Register Symbols are normally valid identifiers,
    but replace any stray non ``[A-Za-z0-9_]`` character defensively.
    """
    return re.sub(r"[^0-9A-Za-z_]", "_", sym) if sym else "DataValue"


def c_string(s: str) -> str:
    """Escape a register Name so it is safe inside a C string literal.

    Backslashes and double-quotes are escaped; any embedded newline/tab is
    collapsed to a space so the literal stays on one line.
    """
    s = (s or "").replace("\\", "\\\\").replace('"', '\\"')
    return s.replace("\n", " ").replace("\r", " ").replace("\t", " ")


def emit_property(sym: str, ul: list[int], owner: str, req: str,
                  type_sym: str = "DataValue", tag: int | None = None) -> list[str]:
    """Lines for one MXF_PROPERTY record.

    ``tag`` is the registered static local tag from the register's
    ``<LocalTag>`` element. When None (no LocalTag in the register), emit the
    0x0000 sentinel so MXFDump assigns the local key dynamically from the
    Primer Pack."""
    tag_str = f"0x{tag:04x}" if tag is not None else "0x0000"
    return [
        f"  MXF_PROPERTY({sym},",
        f"    {fmt_label(ul, indent='    ')},",
        f"    {tag_str},",
        f"    {type_sym},",
        f"    {req},",
        f"    false,",
        f"    {owner})",
    ]


def emit_class(out: list[str], symbol: str, ul: list[int], parent: str,
               concrete: bool, prop_lines: list[str] = ()) -> None:
    """Append a full MXF_CLASS / [props] / MXF_CLASS_END / separator block."""
    label = fmt_label(ul, indent='  ')
    flag = "true" if concrete else "false"
    out.append(f"MXF_CLASS({symbol},")
    out.append(f"  {label},")
    out.append(f"  {parent},")
    out.append(f"  {flag})")
    out.extend(prop_lines)
    out.append(f"MXF_CLASS_END({symbol},")
    out.append(f"  {label},")
    out.append(f"  {parent},")
    out.append(f"  {flag})")
    out.append("MXF_CLASS_SEPARATOR()")
    out.append("")


def iter_leaf_symbols(root: ET.Element):
    """Yield (symbol, ul_bytes) for every LEAF <Entry> that has both set."""
    for e in iter_entries(root):
        if child_text(e, "Kind") != "LEAF":
            continue
        sym = child_text(e, "Symbol")
        ul = parse_ul(child_text(e, "UL") or "")
        if sym and ul:
            yield sym, ul


def iter_leaf_labels(root: ET.Element):
    """Yield (symbol, ul_bytes, name, deprecated) for every LEAF <Entry>.

    Like ``iter_leaf_symbols`` but also returns the human-readable <Name>
    (falling back to the Symbol) and the deprecation flag, both used when
    emitting label entries.
    """
    for e in iter_entries(root):
        if child_text(e, "Kind") != "LEAF":
            continue
        sym = child_text(e, "Symbol")
        ul = parse_ul(child_text(e, "UL") or "")
        if not (sym and ul):
            continue
        name = child_text(e, "Name") or sym
        deprecated = (child_text(e, "IsDeprecated") or "").lower() == "true"
        yield sym, ul, name, deprecated


def main() -> int:
    here = Path(__file__).resolve().parent
    default_output = here / "MXFMetaDictionary_smpte.gen.h"
    # Cross-OS cache location: the system temp dir works on Linux/macOS/Windows
    # and inside a GitHub Action runner (honours TMPDIR/TMP/TEMP/RUNNER_TEMP).
    default_dir      = Path(tempfile.gettempdir()) / "mxfdump_smpte"

    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    # Paths default to None so we can tell whether the user supplied one and,
    # if so, use the local files instead of downloading.
    ap.add_argument("--groups",   default=None)
    ap.add_argument("--elements", default=None)
    ap.add_argument("--essence",  default=None)
    ap.add_argument("--types",    default=None)
    ap.add_argument("--labels",   default=None)
    ap.add_argument("--download-dir", default=str(default_dir),
                    help="directory to download the register XMLs into")
    ap.add_argument("--output",   default=str(default_output),
                    help="path to write generated header")
    args = ap.parse_args()

    # If no register path is given, download all four from the SMPTE registry.
    # If any is given, all four must be given (we use the local files as-is and
    # do not download).
    register_args = {"--groups": args.groups, "--elements": args.elements,
                     "--essence": args.essence, "--types": args.types,
                     "--labels": args.labels}
    if not any(register_args.values()):
        fetched = download_registers(args.download_dir)
        args.groups, args.elements = fetched["groups"], fetched["elements"]
        args.types,  args.essence  = fetched["types"],  fetched["essence"]
        args.labels = fetched["labels"]
    else:
        missing = [name for name, val in register_args.items() if not val]
        if missing:
            ap.error("when passing a register path, all five must be given; "
                     f"missing: {', '.join(missing)}")

    # ---- Parse Types.xml: UL key -> type symbol ---------------------------
    print(f"Loading Types from {args.types}")
    types_root = ET.parse(args.types).getroot()
    types_by_ul: dict[bytes, str] = {
        ul_key(ul): sanitize_token(sym)
        for sym, ul in iter_leaf_symbols(types_root)
    }

    # ---- Parse Elements.xml: UL key -> (symbol, ul, type_sym) -------------
    # The element's <Type> child is a UL into the Types register; resolve it to
    # the type Symbol (falling back to DataValue when absent/unknown).
    print(f"Loading Elements from {args.elements}")
    elements_root = ET.parse(args.elements).getroot()
    elements_by_ul: dict[bytes, tuple[str, list[int], str]] = {}
    type_resolved = 0
    type_fallback = 0
    for e in iter_entries(elements_root):
        if child_text(e, "Kind") != "LEAF":
            continue
        sym = child_text(e, "Symbol")
        ul = parse_ul(child_text(e, "UL") or "")
        if not (sym and ul):
            continue
        type_ul = parse_ul(child_text(e, "Type") or "")
        type_sym = types_by_ul.get(ul_key(type_ul)) if type_ul else None
        if type_sym:
            type_resolved += 1
        else:
            type_sym = "DataValue"
            type_fallback += 1
        elements_by_ul[ul_key(ul)] = (sym, ul, type_sym)

    # ---- Parse Groups.xml -------------------------------------------------
    print(f"Loading Groups from {args.groups}")
    groups_root = ET.parse(args.groups).getroot()
    # First pass: register every group entry by UL so parents resolve.
    groups_by_ul: dict[bytes, dict] = {}
    groups_in_order: list[dict] = []
    for e in iter_entries(groups_root):
        if child_text(e, "Kind") != "LEAF":
            continue
        sym = child_text(e, "Symbol")
        raw_ul = parse_ul(child_text(e, "UL") or "")
        if not (sym and raw_ul):
            continue
        is_concrete = (child_text(e, "IsConcrete") or "").lower() == "true"
        parent_ul   = parse_ul(child_text(e, "Parent") or "") or None
        contents = []
        contents_node = child(e, "Contents")
        if contents_node is not None:
            for rec in contents_node:
                if strip_ns(rec.tag) != "Record":
                    continue
                rec_ul       = parse_ul(child_text(rec, "UL") or "")
                rec_optional = (child_text(rec, "IsOptional") or "true").lower() == "true"
                rec_tag      = parse_local_tag(child_text(rec, "LocalTag"))
                if rec_ul:
                    contents.append({"ul": rec_ul, "optional": rec_optional,
                                     "tag": rec_tag})
        info = {
            "symbol":     sym,
            "ul":         raw_ul,
            "concrete":   is_concrete,
            "parent_ul":  parent_ul,
            "contents":   contents,
        }
        groups_by_ul[ul_key(raw_ul)] = info
        groups_in_order.append(info)

    # ---- Resolve parent symbols ------------------------------------------
    for g in groups_in_order:
        parent_sym = None
        if g["parent_ul"]:
            p = groups_by_ul.get(ul_key(g["parent_ul"]))
            if p:
                parent_sym = p["symbol"]
        g["parent_sym"] = parent_sym or "InterchangeObject"

    # ---- Emit -------------------------------------------------------------
    out: list[str] = []
    out.append("//")
    out.append("// AUTO-GENERATED by MXFMetaDictionary_smpte.gen.py")
    out.append("// Source: SMPTE Metadata Registers (Groups.xml + Elements.xml + Essence.xml + Types.xml)")
    out.append("// Do not edit by hand. Re-run the generator instead.")
    out.append("//")
    out.append("")

    emitted_concrete = 0
    emitted_abstract = 0
    unknown_props = 0
    emitted_elem_keys: set[bytes] = set()

    new_class_names = {g["symbol"] for g in groups_in_order}

    for g in groups_in_order:
        concrete_flag = "true" if g["concrete"] else "false"
        # Resolve parent: prefer something MXFDump can already find.
        parent_sym = g["parent_sym"]
        if parent_sym not in new_class_names:
            parent_sym = "InterchangeObject"

        # Resolve property records.
        prop_lines: list[str] = []
        for rec in g["contents"]:
            ekey = ul_key(rec["ul"])
            elem = elements_by_ul.get(ekey)
            if not elem:
                unknown_props += 1
                continue
            emitted_elem_keys.add(ekey)
            psym, _, ptype = elem
            req = "optional" if rec["optional"] else "required"
            prop_lines += emit_property(psym, rec["ul"], g["symbol"], req, ptype,
                                        tag=rec["tag"])

        if g["concrete"]:
            emitted_concrete += 1
        else:
            emitted_abstract += 1
        out.append(f"// {g['symbol']} (parent {g['parent_sym']}, concrete={concrete_flag})")
        emit_class(out, g["symbol"], g["ul"], parent_sym, g["concrete"], prop_lines)

    # ---- Emit organisation private-use NODE entries -----------------------
    # Organisationally-registered ULs (octet 8 = 0x0e, e.g. Sony Corporation =
    # 06.0e.2b.34.02.7f.01.01.0e.06.00...) are tree NODEs, not LEAF groups, so
    # the loop above skips them. Each names an organisation that registers its
    # own private metadata; the specific private keys are never in the register.
    # Emit each org node as a property-less class so MXFDump resolves any private
    # key of that organisation to its name: octet 5 is already 0x7f (wildcard) 
    # and matchMXFKeyMasked treats the trailing 0x00 suffix as a
    # wildcard for these org-prefix keys (see its pvtGroup rule).
    org_nodes = []
    for e in iter_entries(groups_root):
        if child_text(e, "Kind") != "NODE":
            continue
        sym = child_text(e, "Symbol")
        ul = parse_ul(child_text(e, "UL") or "")
        if not (sym and ul):
            continue
        # org registration root: ...0e.<org>.00.00.00.00.00.00, org id != 0
        if ul[8] != 0x0e or ul[9] == 0x00 or any(ul[10:]):
            continue
        org_nodes.append((sym, ul, child_text(e, "Name") or sym))
    if org_nodes:
        out.append("//")
        out.append("// Organisation private-use nodes (Groups.xml Kind=NODE, octet 8 = 0x0e).")
        out.append("// Resolve any privately-registered key to its organisation name; the")
        out.append("// trailing 0x00 suffix is wildcarded by matchMXFKeyMasked's pvtGroup rule.")
        out.append("//")
        out.append("")
        for sym, ul, name in org_nodes:
            out.append(f"// {name}")
            emit_class(out, sym, ul, "InterchangeObject", False)

    # Emit elements that exist in Elements.xml but were not referenced by any
    # Group's Contents (bare KLV elements, Generic Container items, Indirect-type
    # properties such as XMLDocumentText_Indirect).
    orphan_list = sorted(
        ((sym, pul, ptype)
         for ekey, (sym, pul, ptype) in elements_by_ul.items()
         if ekey not in emitted_elem_keys),
        key=lambda t: t[0],
    )
    if orphan_list:
        out.append("//")
        out.append("// Orphan elements: present in Elements.xml but not referenced by any")
        out.append("// Group's Contents. Includes bare KLV / Generic Container items and")
        out.append("// Indirect-type properties (e.g. XMLDocumentText_Indirect).")
        out.append("//")
        out.append("")
        for sym, pul, ptype in orphan_list:
            props = emit_property(sym, pul, "InterchangeObject", "optional", ptype)
            emit_class(out, sym, pul, "InterchangeObject", False, props)

    # ---- Parse Essence.xml and emit essence element keys -------------------
    print(f"Loading Essence from {args.essence}")
    essence_root = ET.parse(args.essence).getroot()
    essence_list = list(iter_leaf_symbols(essence_root))

    if essence_list:
        out.append("//")
        out.append("// Essence element keys (SMPTE ST 2088 / Essence.xml).")
        out.append("// 0x7f bytes are wildcards matched by matchMXFKeyMasked.")
        out.append("//")
        out.append("")
        for sym, ul in essence_list:
            emit_class(out, sym, ul, "InterchangeObject", True)

    # ---- Parse Labels.xml and emit label entries --------------------------
    # Labels are UL VALUES (essence-container labels, coding/colour labels,
    # operational patterns, etc.), not KLV set keys. They are emitted with the
    # MXF_LABEL_ENTRY macro, which is consumed ONLY by mxfSmpteLabelTable in
    # MXFDump.cpp (the other tables #define it empty).  
    print(f"Loading Labels from {args.labels}")
    labels_root = ET.parse(args.labels).getroot()
    label_seen: set[bytes] = set()
    label_list: list[tuple[str, list[int], str, bool]] = []
    label_dups = 0
    for sym, ul, name, deprecated in iter_leaf_labels(labels_root):
        lkey = ul_key(ul)
        if lkey in label_seen:
            label_dups += 1
            continue
        label_seen.add(lkey)
        label_list.append((sym, ul, name, deprecated))

    if label_list:
        out.append("//")
        out.append("// SMPTE Labels (Labels.xml). UL VALUES resolved to names; emitted via")
        out.append("// MXF_LABEL_ENTRY and consumed only by mxfSmpteLabelTable in MXFDump.cpp.")
        out.append("// 0x7f bytes are wildcards matched by matchMXFKeyMasked.")
        out.append("//")
        out.append("")
        for sym, ul, name, deprecated in label_list:
            if deprecated:
                out.append("// deprecated")
            out.append(f"MXF_LABEL_ENTRY({sanitize_token(sym)}, \"{c_string(name)}\",")
            out.append(f"  {fmt_label(ul, indent='  ')})")
            out.append("")

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(out) + "\n", encoding="utf-8")

    print(f"gen_mxfdump_smpte_dict: emitted {emitted_concrete} concrete + {emitted_abstract} abstract classes, {len(org_nodes)} org nodes, {len(orphan_list)} orphan elements, {len(essence_list)} essence element keys, {len(label_list)} labels")
    print(f"  (abstract classes are included so their properties are discoverable)")
    print(f"  element types: {type_resolved} resolved from Types.xml, {type_fallback} fell back to DataValue")
    if label_dups:
        print(f"  labels: {label_dups} duplicate ULs (version-insensitive) dropped")
    if unknown_props:
        print(f"  warning: {unknown_props} property ULs were not in Elements.xml", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
