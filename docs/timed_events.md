# Timed Events

## SMPTE ST 2067-206

[SMPTE ST 2067-206](https://pub.smpte.org/doc/st2067-206/) specifies a plug-in mechanism to add generic, event-based, text-based metadata (such as an XML, JSON or YAML document) and associated binary resources (such as images) to IMF Compositions.

To do this, it specifies the Timed Events Track File. This is an MXF file that wraps together:
* The text-based metadata document that defines the timeline of events (the Timed Events Resource)
* Any binary resources (the Timed Events Ancillary Resources)
* Essential metadata

[SMPTE ST 2067-206](https://pub.smpte.org/doc/st2067-206/) also includes:
* Example Timed Events Track File (MXF file)
* Definition of an "Event Scheme"
* The "Simple Scheme": an example Event Scheme providing an optional starting-point for an event-based metadata document (XML or JSON)
* For IMF Compositions: Timed Events Virtual Track and TimedEventsVirtualTrackParameterSet

## bmx support

bmx implements the mechanisms needed to create a Timed Events Track File. In bmx this is called "Timed Events".

Timed Events are embedded in the same way as [Timed Text](./timed_text.md) and consequently much of the implementation code is shared. The implementation code uses the Timed Data naming when combining the implementation code for Timed Text and Timed Events. As with the Timed Text implementation, bmx does not limit the use of Timed Events to IMF Track Files. For example, Timed Events can be added to MXF files that also contain video and audio.

The description for [Timed Text](./timed_text.md) mostly applies to Timed Events as well, except for these differences:
* The manifest file format uses the XML representation. It is described below in the [Manifest File Format](#manifest-file-format) section.
* The `raw2bmx` option is `--timed-events`.
* [Not used in IMF] The time offset applied in the Material Package to position the Timed Events is named `timed_events_offset` in the `mxf2raw` output.
* The Timed Events metadata is slightly different. The [Commandline Utilities: `mxf2raw`](#commandline-utilities-mxf2raw) section provides an example output from `mxf2raw`.
* The manifest class is `TimedEventsManifest`.
* The method to read ancillary resources by ID is `MXFTimedDataTrackReader::ReadTimedEventsAncillaryResourceById()`.

## Practical workflow steps

1. **Choose/create the Event Scheme.** An Event Scheme is identified by a URI and simply specifies what kind of text-based metadata document and (potentially) binary resources are to be used. An Event Scheme might specify a particular XML/JSON Schema, for example. An existing Event Scheme can be used or one can be created.
2. **Create the data files and bmx manifest XML file.** The manifest references the text-based metadata document and any binary resources, and provides the Event Scheme URI and other essential metadata.
3. **Create an MXF file using the manifest.** The `raw2bmx` command can be used to wrap all the referenced items (and the metadata) into an MXF file.

## Manifest File Format

The manifest file provides `raw2bmx` the metadata and the locations for timed events and ancillary resources. It is represented in XML and defined in the [timed_events_manifest.xsd](../meta/timed_events_manifest.xsd) XML Schema.

The XML namespace is `http://bbc.co.uk/bmx/timed-events/202403` and the root element is `<manifest>`.

The `<file>` element (*required*) provides information about the Timed Events essence (that is, the text-based metadata document) and the position in the Material Package:
* `path` (*required*): The file location of the Timed Events essence. A relative file location is relative to the location of the manifest.
* `mime_type` (*required*): The MIME type for the Timed Events essence. This attribute maps to the `TextMIMEType` property in the `TimedEventsTextDescriptor` class.
* `character_encoding` (*required*): The text encoding for the Timed Events essence. This attribute maps to the `TextCharacterEncoding` property in the `TimedEventsTextDescriptor` class.
* [Not used in IMF] `start` (*optional*): Specifies the non-zero start position for the Timed Events. The value is either a timecode (HH:MM:SS:FF or HH:MM:SS;FF) or a position in media edit rate. It is assumed to be 0 if not set.

The `<file>` element contains `<event_schemes>` (*required*) that contains 1 or more URIs in `<uri>` elements. These URIs map to elements of the `EventSchemeURIs` property in the `TimedEventsTextDescriptor` class. The order of these `<uri>` elements is significant: per [SMPTE ST 2067-206](https://pub.smpte.org/doc/st2067-206/), the first `<uri>` element must contain the Event Scheme URI with any subsequent `uri` elements containing Parent Event Scheme URIs.

The `<ancillary_resources>` element (*optional*) lists ancillary resource data files in `<resource>` elements:
* `path` (*required*): The file location of the ancillary resource data. A relative file location is relative to the location of the manifest.
* `id` (*optional*): The resource identifier. The default is the filename (name plus suffix) component of the file path. This attribute maps to the `TimedEventsAncillaryResourceID` property of the `TimedEventsAncillaryResourceSubDescriptor` class.
* `mime_type` (*required*): The MIME type for the ancillary data. This attribute maps to the `TimedEventsAncillaryResourceMIMEType` property in the `TimedEventsAncillaryResourceSubDescriptor` class.

## IMF example using `raw2bmx`

An example command and manifest is shown below for creating a Timed Events Track File (MXF file) for use in an IMF Composition.

In this example, the Event Scheme is identified by `http://www.example.com/props` and so this is given in the first `<uri>` element. It is based on the Simple Scheme defined in [SMPTE ST 2067-206](https://pub.smpte.org/doc/st2067-206/) and so the Event Scheme URI of the Simple Scheme is given in the second `<uri>` element. The text-based metadata document is called `props_data.xml`. It contains events that describe when each of the listed props (tables, chairs, etc) appears in the associated media. `image-104.png` contains an image of one of these props. The image is referenced from inside `props_data.xml` using the id `image:chair`.

The `raw2bmx` command to create the Timed Events Track File (MXF file) is:

`raw2bmx -t imf -o {Type}_{fp_uuid}.mxf -f 25 --dur 2160000 --timed-events manifest.xml`

In this example, the output file is called `DATA_5f4e2ac6-0572-4c23-93ce-9950da333223.mxf`.

The values of `-f` and `-dur` must meet the requirements of the IMF "Sequence" elements that will be used to create the Timed Events Virtual Track in the IMF Composition. Here, it is assumed that one of the "Sequence elements for use with the Simple Scheme" defined in [SMPTE ST 2067-206](https://pub.smpte.org/doc/st2067-206/) will be used. As such, `-f` is set to match the `<editRate>` property of `props_data.xml` (assumed to be `<editRate>25 1</editRate>`) and `-dur` is set to a very large value at this rate.

`manifest.xml` is:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<manifest xmlns="http://bbc.co.uk/bmx/timed-events/202403">
    <file path="props_data.xml" mime_type="text/xml" character_encoding="UTF-8">
        <event_schemes>
            <uri>http://www.example.com/props</uri>
            <uri>http://www.smpte-ra.org/ns/2067-206/2023/simple-scheme</uri>
        </event_schemes>
    </file>
    <ancillary_resources>
        <resource path="image-104.png" id="image:chair" mime_type="image/png"/>
    </ancillary_resources>
</manifest>
```

## Commandline Utilities: `mxf2raw`

The `mxf2raw` utility can be used to show metadata about the Timed Events tracks and extract the essence data to files.

The Timed Events tracks will have metadata shown similar to the extract below (produced for the MXF file generated in the example above). It shows the properties in the Timed Events data file descriptor and sub-descriptors.

[Not used in IMF] A non-zero Timed Events offset, which corresponds to the `start` field in the manifest, is given in the `timed_events_offset` field in the Track information (not relevant in this example).

```text
  Tracks: (1)
    Track #0:
      essence_kind    : Data
      essence_type    : Timed_Events
      ec_label        : urn:smpte:ul:060e2b34.0401010d.0d010301.02130102
      edit_rate       : 25/1
      duration        : 24:00:00:00 (count='2160000')
      Packages: (1)
        Package #0:
          Material:
            package_uid     : urn:smpte:umid:060a2b34.01010105.01010f20.13000000.923ded26.ada14b08.8a71c0b0.6b518dfa
            track_id        : 3001
            track_number    : 0
          FileSource:
            package_uid     : urn:smpte:umid:060a2b34.01010105.01010f20.13000000.5f4e2ac6.05724c23.93ce9950.da333223
            track_id        : 3001
            track_number    : 0x17011001
            file_uri        : file:///tmp/DATA_5f4e2ac6-0572-4c23-93ce-9950da333223.mxf
      DataDescriptor:
        TimedEventsDescriptor:
          mime_type       : text/xml
          character_encoding : UTF-8
          EventSchemes: (2)
            Element #0:
              uri             : http://www.example.com/props
            Element #1:
              uri             : http://www.smpte-ra.org/ns/2067-206/2023/simple-scheme
          AncillaryResources: (1)
            Element #0:
              resource_id     : image:chair
              mime_type       : image/png
              stream_id       : 12
```

Using the `--ess-out` or `-p` option will create a [manifest file](#manifest-file-format), and extract the text-based metadata document and any ancillary resource files. The file listing example shown below is the result of a run of `mxf2raw -p output DATA_5f4e2ac6-0572-4c23-93ce-9950da333223.mxf`.

```
output_d0_manifest.xml
output_d0.txt
output_d0_12.raw
```

The listed files are for the first MXF data track (`d0`). The manifest is stored in `output_d0_manifest.xml`, the text-based metadata document is `output_d0.txt` and the image resource is `output_d0_12.raw` (which was stored in the MXF generic stream `12`). The [manifest file](#manifest-file-format) can be used to re-wrap the Timed Events into a new MXF file using `raw2bmx`.
