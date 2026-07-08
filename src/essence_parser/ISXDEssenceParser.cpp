/*
 * Copyright (C) 2023, Dolby Laboratories
 * All Rights Reserved.
 *
 * Author: Markus Pfundstein
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

#include <string.h>

#include <bmx/essence_parser/ISXDEssenceParser.h>
#include <bmx/BMXException.h>
#include "EssenceParserUtils.h"
#include <bmx/Logging.h>
#include <expat.h>

using namespace std;
using namespace bmx;

static const char NAMESPACE_SEPARATOR = ' ';

static void split_qname(const char *qname, string *ns, string *name)
{
    const char *ptr = qname;
    while (*ptr && *ptr != NAMESPACE_SEPARATOR)
    {
        ptr++;
    }

    if (!(*ptr))
    {
        ns->clear();
        name->assign(qname);
    }
    else
    {
        ns->assign(qname, (size_t)(ptr - qname));
        name->assign(ptr + 1);
    }
}

void ISXDEssenceParser::StartElement(void *user_data, const char *qname, const char **atts)
{
    ISXDEssenceParser *parser = static_cast<ISXDEssenceParser*>(user_data);
    (void)atts;

    string ns, name;
    split_qname(qname, &ns, &name);
    if (parser->mRootElementKey == "")
    {
        string ns, name;
        split_qname(qname, &ns, &name);

        parser->mRootElementKey = name;
        parser->mNamespace = ns;
    }

    // if we dont have a namespace yet, we try to get it from DolbyVisionGlobalData element and version attribute
    if (parser->mNamespace == "" && name == "DolbyVisionGlobalData")
    {
        for (int i = 0; atts[i] != nullptr; i += 2) {
            string attr_ns, attr_name;
            split_qname(atts[i], &attr_ns, &attr_name);
            
            const char* value = atts[i + 1];
            
            if (attr_name == "version") {
                if (strncmp(value, "2.0", 3) == 0) {
                    parser->mNamespace = "http://www.dolby.com/schemas/dvmd/2_0_5";
                }
            }
        }
    }
}

void ISXDEssenceParser::EndElement(void *user_data, const char *qname)
{
    ISXDEssenceParser *parser = static_cast<ISXDEssenceParser*>(user_data);

    string ns, name;
    split_qname(qname, &ns, &name);

    if (name == parser->mRootElementKey)
    {
        parser->mFullFrameParsed = true;
        XML_StopParser(parser->mParser, true);
    }
}

ISXDEssenceParser::ISXDEssenceParser()
: EssenceParser(), mBytesRead(0), mFullFrameParsed(false), mParser(nullptr)
{
    ResetParser();
}

ISXDEssenceParser::~ISXDEssenceParser()
{
    if (mParser)
    {
        XML_ParserFree(mParser);
        mParser = nullptr;
    }
}

std::string ISXDEssenceParser::GetNamespaceURI()
{
    return mNamespace;
}

std::string ISXDEssenceParser::GetRootElementKey()
{
    return mRootElementKey;
}

void ISXDEssenceParser::ResetParser()
{
    if (mParser)
    {
        bool success  = XML_ParserReset(mParser, "utf-8");
        BMX_ASSERT(success);
    }
    else
    {
        mParser = XML_ParserCreateNS("utf-8", NAMESPACE_SEPARATOR);
    }

    XML_SetStartElementHandler(mParser, ISXDEssenceParser::StartElement);
    XML_SetEndElementHandler(mParser, ISXDEssenceParser::EndElement);
    XML_SetUserData(mParser, this);

    mBytesRead = 0;
    mFullFrameParsed = 0;
    mPos = 0;
}

uint32_t ISXDEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    (void)data;

    return 0;
}

uint32_t ISXDEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // done with file
    if (data_size == 0)
    {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    if (!mFullFrameParsed) {
      ParseFrameInfo(data, data_size);
    }

    if (mFullFrameParsed)
    {
        // doesnt seem to be necessary
        if (mBytesRead + 1 >= data_size) {
            // one more round
            return ESSENCE_PARSER_NULL_OFFSET;
        }

        mBytesRead += 1;
        if (data[mBytesRead] == '\n') {
            mBytesRead += 1;
        }
        

        uint32_t len = mBytesRead;
        ResetParser();
        return len;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

void ISXDEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    const char *data_start = (const char*)data + mPos;
    
    XML_Status status = XML_Parse(mParser, data_start, data_size - mPos, 0);
    if (status == XML_STATUS_ERROR)
    {
        XML_Error err = XML_GetErrorCode(mParser);
        if (err != XML_ERROR_ABORTED)
        {
            int32_t err_index = XML_GetErrorByteIndex(mParser);
            throw BMXException("XML Error: %d (%s) (err_idx: %d, char: %c)\n",
                err, XML_ErrorString(err), err_index, data[err_index]);
        }
        throw BMXException("XML ABORTED error");
    }

    int32_t byte_index = XML_GetCurrentByteIndex(mParser);
    if (byte_index < 0)
    {
        byte_index = 0;
    }

    mBytesRead = byte_index;
    mPos = data_size;
}
