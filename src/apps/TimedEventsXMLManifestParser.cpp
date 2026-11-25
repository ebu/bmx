/*
 * Copyright (C) 2024, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/apps/TimedEventsXMLManifestParser.h>

#include <stdio.h>
#include <expat.h>
#include <regex>

#include <bmx/URI.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define START_EXPAT_CALLBACK                              \
    try                                                   \
    {

#define END_EXPAT_CALLBACK                                \
    }                                                     \
    catch (const BMXException &ex)                        \
    {                                                     \
        log_error("%s\n", ex.what());                     \
        XML_StopParser((XML_Parser)mParser, XML_FALSE);   \
    }                                                     \
    catch (const std::exception &ex)                      \
    {                                                     \
        log_error("Exception: %s\n", ex.what());          \
        XML_StopParser((XML_Parser)mParser, XML_FALSE);   \
    }                                                     \
    catch (...)                                           \
    {                                                     \
        log_error("Unknown exception thrown\n");          \
        XML_StopParser((XML_Parser)mParser, XML_FALSE);   \
    }


static const char NAMESPACE_SEPARATOR = ' ';

static const char *MANIFEST_NAMESPACE = "http://bbc.co.uk/bmx/timed-events/202403";


static void split_qname(const char *qname, string *ns, string *name)
{
    const char *ptr = qname;
    while (*ptr && *ptr != NAMESPACE_SEPARATOR)
        ptr++;

    if (!(*ptr)) {
        ns->clear();
        name->assign(qname);
    } else {
        ns->assign(qname, (size_t)(ptr - qname));
        name->assign(ptr + 1);
    }
}

static void expat_StartElement(void *user_data, const char *qname, const char **atts)
{
    string ns, name;
    split_qname(qname, &ns, &name);
    reinterpret_cast<TimedEventsXMLManifestParser*>(user_data)->StartElement(ns, name, atts);
}

static void expat_EndElement(void *user_data, const char *qname)
{
    string ns, name;
    split_qname(qname, &ns, &name);
    reinterpret_cast<TimedEventsXMLManifestParser*>(user_data)->EndElement(ns, name);
}

static void expat_CharacterData(void *user_data, const char *s, int len)
{
    reinterpret_cast<TimedEventsXMLManifestParser*>(user_data)->CharacterData(s, len);
}


static bool have_attribute(const string &name, const char **atts)
{
    const char **atts_ptr = atts;
    while (atts_ptr && atts_ptr[0] && atts_ptr[1]) {
        const char *att_name = *atts_ptr++;
        atts_ptr++; // value
        if (name == att_name)
            return true;
    }

    return false;
}

static string get_attribute(const string &element_name, const string &name, const char **atts)
{
    const char **atts_ptr = atts;
    while (atts_ptr && atts_ptr[0] && atts_ptr[1]) {
        const char *att_name = *atts_ptr++;
        const char *att_value = *atts_ptr++;
        if (name == att_name)
            return att_value;
    }

    throw BMXException("Missing '%s' attribute in <%s>", name.c_str(), element_name.c_str());
}


static bool create_abs_file_path(const string &manifest_filename, const string &item_filename,
                                 string *abs_item_file_path)
{
    // convert a relative (to the manifest file) filename into a absolute file path
    URI item_uri;
    if (!item_uri.ParseFilename(item_filename)) {
        log_error("Failed to parse 'file' into URI from timed events manifest\n");
        return false;
    }
    if (item_uri.IsRelative()) {
        URI mf_uri;
        mf_uri.ParseFilename(manifest_filename);

        if (mf_uri.IsRelative()) {
            URI cwd_uri;
            cwd_uri.ParseDirectory(get_cwd());
            mf_uri.MakeAbsolute(cwd_uri);
        }

        item_uri.MakeAbsolute(mf_uri);
    }

    *abs_item_file_path = item_uri.ToFilename();
    return true;
}

static bool parse_position(const string &value, Timecode start_timecode, Rational frame_rate,
                           int64_t *int64_value)
{
    if (value.find(":") == string::npos) {
        if (sscanf(value.c_str(), "%" PRId64 "", int64_value) == 1)
            return true;
    } else {
        int hour, min, sec, frame;
        char c;
        if (sscanf(value.c_str(), "%d:%d:%d%c%d", &hour, &min, &sec, &c, &frame) == 5) {
            Timecode tc(frame_rate, (c != ':'), hour, min, sec, frame);
            *int64_value = tc.GetOffset() - start_timecode.GetOffset();
            return true;
        }
    }

    return false;
}


TimedEventsXMLManifestParser::TimedEventsXMLManifestParser()
: TimedEventsManifest()
{
    Reset();
}

TimedEventsXMLManifestParser::~TimedEventsXMLManifestParser()
{
}

bool TimedEventsXMLManifestParser::Parse(const string &filename, Timecode start_tc, Rational frame_rate)
{
    Reset();
    mParseState.clear();
    mManifestFilename = filename;
    mStartTC = start_tc;
    mFrameRate = frame_rate;

    FILE *file = 0;
    char *buffer = 0;
    const size_t buffer_size = 4096;
    bool parse_result = true;
    mParser = 0;

    try
    {
        file = fopen(mManifestFilename.c_str(), "rb");
        if (!file) {
            throw BMXException("Failed to open timed events XML manifest file '%s': %s",
                               mManifestFilename.c_str(), bmx_strerror(errno).c_str());
        }

        mParser = XML_ParserCreateNS(0, NAMESPACE_SEPARATOR);
        if (!mParser)
            throw BMXException("XML_ParserCreateNS returned NULL");

        XML_SetStartElementHandler((XML_Parser)mParser,  expat_StartElement);
        XML_SetEndElementHandler((XML_Parser)mParser,    expat_EndElement);
        XML_SetCharacterDataHandler((XML_Parser)mParser, expat_CharacterData);
        XML_SetUserData((XML_Parser)mParser, this);

        buffer = new char[buffer_size];
        size_t num_read;
        do {
            num_read = fread(buffer, 1, buffer_size, file);
            if (num_read != buffer_size && ferror(file))
                throw BMXException("Failed to read from XML manifest file: %s", bmx_strerror(errno).c_str());

            if (XML_Parse((XML_Parser)mParser, buffer, (int)num_read, num_read != buffer_size) == XML_STATUS_ERROR) {
                throw BMXException("XML manifest parse error near line %lu: %s",
                                   XML_GetCurrentLineNumber((XML_Parser)mParser),
                                   XML_ErrorString(XML_GetErrorCode((XML_Parser)mParser)));
            }
        } while (num_read == buffer_size);
    }
    catch (const BMXException &ex)
    {
        log_error("Failed to parse timed events manifest XML file '%s': %s\n", mManifestFilename.c_str(), ex.what());
        parse_result = false;
    }
    catch (...)
    {
        log_error("Failed to parse timed events manifest XML file '%s': unknown exception thrown\n", mManifestFilename.c_str());
        parse_result = false;
    }

    delete [] buffer;
    if (file)
        fclose(file);
    if (mParser) {
        XML_ParserFree((XML_Parser)mParser);
        mParser = 0;
    }

    return parse_result;
}

bool TimedEventsXMLManifestParser::CheckCanReadTimedEventsFile()
{
    FILE *file = fopen(mFilename.c_str(), "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}

void TimedEventsXMLManifestParser::StartElement(const string &ns, const string &name, const char **atts)
{
    START_EXPAT_CALLBACK

    if (mParseState.empty()) {
        ParseStateElement element;
        element.ns = ns;
        element.name = name;

        if (name != "manifest")
            throw BMXException("Root element name should be <manifest>");
        else if (ns != MANIFEST_NAMESPACE)
            throw BMXException("Root element namespace should be '%s'", MANIFEST_NAMESPACE);

        mParseState.push_back(element);
        return;
    }

    // Ignore an element if it or a parent is from another namespace or a parent is unknown
    if (ns != mParseState.front().ns || mParseState.back().ns.empty()) {
        ParseStateElement ignore_element;
        mParseState.push_back(ignore_element);
        return;
    }

    // Parse child elements of <manifest>
    if (mParseState.back().name == "manifest") {
        ParseStateElement element;
        element.ns = ns;
        element.name = name;

        if (name == "file") {
            string path = get_attribute(name, "path", atts);
            if (!create_abs_file_path(mManifestFilename, path, &mFilename))
                throw BMXException("Failed to create absolute path for timed events file '%s'", path.c_str());

            mMIMEType = get_attribute(name, "mime_type", atts);
            mCharacterEncoding = get_attribute(name, "character_encoding", atts);

            if (have_attribute("start", atts)) {
                string start = get_attribute(name, "start", atts);
                if (!parse_position(start, mStartTC, mFrameRate, &mStart))
                    throw BMXException("Failed to parse timed events start '%s'\n", start.c_str());
            }

        } else if (name != "ancillary_resources") {
            // Ignore unknown element
            element.ns = "";
            element.name = "";
        }

        mParseState.push_back(element);
        return;
    }

    // Parse child elements of <manifest><file>
    if (mParseState.back().name == "file") {
        ParseStateElement element;
        element.ns = ns;
        element.name = name;

        if (name != "event_schemes") {
            // Ignore unknown element
            element.ns = "";
            element.name = "";
        }

        mParseState.push_back(element);
        return;
    }

    // Parse child elements of <manifest><file><event_schemes>
    if (mParseState.back().name == "event_schemes") {
        ParseStateElement element;
        element.ns = ns;
        element.name = name;

        if (name != "uri") {
            // Ignore unknown element
            element.ns = "";
            element.name = "";
        }

        mParseState.push_back(element);
        return;
    }

    // Parse child elements of <manifest><ancillary_resources>
    if (mParseState.back().name == "ancillary_resources") {
        ParseStateElement element;
        element.ns = ns;
        element.name = name;

        if (name == "resource") {
            TimedEventsAncillaryResource *resource = new TimedEventsAncillaryResource();
            try {
                string path = get_attribute(name, "path", atts);
                if (!create_abs_file_path(mManifestFilename, path, &resource->filename))
                    throw BMXException("Failed to create absolute path for resource file '%s'", path.c_str());

                if (have_attribute("id", atts))
                    resource->resource_id = get_attribute(name, "id", atts);
                else
                    resource->resource_id = strip_path(path);

                resource->mime_type = get_attribute(name, "mime_type", atts);
            } catch (...) {
                delete resource;
                throw;
            }

            mAncillaryResources.push_back(resource);
        } else {
            // Ignore unknown element
            element.ns = "";
            element.name = "";
        }

        mParseState.push_back(element);
        return;
    }

    END_EXPAT_CALLBACK
}

bool TimedEventsXMLManifestParser::EndElement(const string &ns, const string &name)
{
    (void)ns;
    (void)name;

    START_EXPAT_CALLBACK

    if (mParseState.back().name == "manifest") {
        if (mFilename.empty())
            throw BMXException("Missing <file> in <manifest> element");
    } else if (mParseState.back().name == "file") {
        if (mEventSchemes.empty())
            throw BMXException("Missing or empty <event_schemes> in <file> element");
    }

    mParseState.pop_back();
    return true;

    END_EXPAT_CALLBACK
    return false;
}

void TimedEventsXMLManifestParser::CharacterData(const char *s, int len)
{
    START_EXPAT_CALLBACK

    if (mParseState.back().name == "uri") {
        string uri = trim_string(string(s, len));
        if (uri.empty())
            throw BMXException("Empty <uri> character data");
        if (regex_search(uri, regex(" ")))
            // Note: SMPTE ST 2067-206 states: "An Event Scheme URI shall not include the space character"
            throw BMXException("<uri> character data contains a space character after trimming: an Event Scheme URI shall not include the space character (SMPTE ST 2067-206)");
        mEventSchemes.push_back(uri);
    }

    END_EXPAT_CALLBACK
}
