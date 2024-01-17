/*
 * Copyright (C) 2023, British Broadcasting Corporation
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

#include "bmx/essence_parser/EssenceParser.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <bmx/essence_parser/SADMEssenceParser.h>
#include <bmx/BMXException.h>
#include "EssenceParserUtils.h"
#include <bmx/Logging.h>
#include <expat.h>

using namespace std;
using namespace bmx;

class InvalidDataError : public exception
{
public:
    InvalidDataError(const char *message)
    : msg_(message) {}

    InvalidDataError(const std::string &message)
    : msg_(message) {}

    virtual const char* what() const noexcept {
       return msg_.c_str();
    }

    const char* msg() const noexcept {
        return msg_.c_str();
    }

protected:
    std::string msg_;
};

class NotEnoughDataError : public exception
{
public:
    virtual const char* what() const noexcept {
        return "Not enough data";
    }
};

SADMEssenceParser::SADMEssenceParser()
    : EssenceParser(),
      mSectionHeaderParsed(false),
      mSectionSize(0),
      mSections(0),
      mCurrentSectionIndex(0),
      mStreamPos(0),
      mSectionPos(0),
      mFrameLength(0),
      mCurrentSectionIdentifier(SECTION_IDENTIFIER::AUDIO_ESSENCE),
      mMetadataParsed(false),
      mMetadataHeaderParsed(false),
      mMetadataFormat(METADATA_FORMAT::XML)
{
    ResetFrameInfo();
}

SADMEssenceParser::~SADMEssenceParser()
{
}

bool SADMEssenceParser::SkipSection() const
{
    // always skip audio and fill
    if (mCurrentSectionIdentifier == SECTION_IDENTIFIER::FILL_SECTION ||
        mCurrentSectionIdentifier == SECTION_IDENTIFIER::AUDIO_ESSENCE) {
        return true;
    }
    // skip metadata only if we have metadata parsed already
    return mMetadataParsed;
}


uint32_t SADMEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    (void)data;
    return 0;
}

uint32_t SADMEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    log_info("Parse Frame Size\n");
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // done with file
    if (data_size == 0) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    try {
        ParseSection(data, data_size);

        // end of section reached
        if (mSectionPos == mSectionSize) {
            ResetSectionInfo();

            // last section reached
            if (mCurrentSectionIndex == mSections - 1) {
                uint32_t tmp = mFrameLength;
                ResetFrameInfo();
                return tmp;
            }
        }
    } catch (NotEnoughDataError &e) {
        log_info("HELP!!!!! Need more data");
        return ESSENCE_PARSER_NULL_OFFSET;
    } catch (InvalidDataError &e) {
        log_error("InvalidDataError: %s\n", e.msg());
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    // need more data
    return ESSENCE_PARSER_NULL_OFFSET;
}

void SADMEssenceParser::ParseFrameInfo(const unsigned char *data,
                                       uint32_t data_size)
{
    // Note: THIS FUNCTION EXPECTS THAT THE WHOLE DATA IS
    // AVAILABLE IN ONE GO

    // parse first section
    ParseSection(data, data_size);
    ResetSectionInfo();
    for (uint32_t i = 1; i < mSections; ++i) {
        ParseSection(data, data_size);
        ResetSectionInfo();
    }
}

void SADMEssenceParser::ParseSection(const unsigned char *data,
                                       uint32_t data_size)
{
    log_info("ParseMGAFrame (%d / %d)\n", mStreamPos, data_size);
    if (!mSections) {
        // log_info("parse number sections\n");
        mSections = ReadByte(data, data_size);
        if (mSections < 2) {
            throw InvalidDataError("Invalid s-ADM. Minimum of 2 sections required");
        }
        mFrameLength = 1;
    }

    if (!mSectionHeaderParsed) {
        const uint32_t SECTION_HEADER_LENGTH = 6;

        if ((data_size - mStreamPos) < SECTION_HEADER_LENGTH) {
            throw NotEnoughDataError();
        }
        // no need to call ReadByte here because of the conditional in the
        // if statement
        mCurrentSectionIndex = ReadByte(data, data_size);
        mCurrentSectionIdentifier = (SECTION_IDENTIFIER)(ReadByte(data, data_size));
        if (mCurrentSectionIdentifier != SECTION_IDENTIFIER::AUDIO_ESSENCE &&
            mCurrentSectionIdentifier !=
                SECTION_IDENTIFIER::AUDIO_METADATA_METADATA_PACK &&
            mCurrentSectionIdentifier !=
                SECTION_IDENTIFIER::AUDIO_METADATA_PAYLOAD &&
            mCurrentSectionIdentifier != SECTION_IDENTIFIER::FILL_SECTION) {

            throw InvalidDataError("Invalid section identifier");
        }

        log_info("got section identifier: %X\n", mCurrentSectionIdentifier);
        mSectionSize = data[mStreamPos] << 24 | data[mStreamPos + 1] << 16 |
                       data[mStreamPos + 2] << 8 | data[mStreamPos + 3];
        mStreamPos += 4;
        mSectionPos = 0;

        mFrameLength += SECTION_HEADER_LENGTH;
        mFrameLength += mSectionSize;

        mSectionHeaderParsed = true;
    }

    if (mSectionHeaderParsed) {

        // SKIP AUDIO AND FILL. Parse METADATA. But only for first frame.
        // Store all metadata in an array of structs that we can then use
        // to create the MGA Audio Metadat SubDescriptors. Because we need
        // one per metadata section
        // After first pass, we can skip
        if (SkipSection()) {
            int64_t skipBytes = BytesAvailable(data_size);
            mStreamPos += skipBytes;
            mSectionPos += skipBytes;
            return;
        }

        if (!mMetadataHeaderParsed) {
            // make sure we have enough bytes to read the payload header
            if (BytesAvailable(data_size) < 7) {
                throw NotEnoughDataError();
            }
            // 1 byte
            uint8_t payloadTag = ReadByte(data, data_size);
            BMX_ASSERT(payloadTag == 0x12);

            // 4 bytes max
            uint32_t nFields = 0;
            uint32_t payloadLength = ReadBERLengthField(data, data_size, &nFields);
            mMetadata.reserve(payloadLength);
            mMetadata.clear();

            // 1 byte
            uint8_t version = ReadByte(data, data_size);
            BMX_ASSERT(version == 0x00);

            // 1 byte
            uint8_t mMetadataFormat = ReadByte(data, data_size);
            BMX_ASSERT(mMetadataFormat == METADATA_FORMAT::XML ||
                       mMetadataFormat == METADATA_FORMAT::GZIP);

            mMetadataHeaderParsed = true;
        }

        if (mMetadataHeaderParsed) {
            uint32_t bytesToCopy = BytesAvailable(data_size);
            for (uint32_t i = 0; i < bytesToCopy; ++i) {
                mMetadata.push_back(data[mStreamPos]);
                mStreamPos++;
                mSectionPos++;
            }

            if (mSectionPos == mSectionSize) {
                ParseMetadataPayload();
                mMetadataParsed = true;
                //BMX_ASSERT(false);
            }
        }
    }
}

void SADMEssenceParser::XML_StartElement(void *user_data, const char *qname, const char **atts)
{
    SADMEssenceParser *parser = static_cast<SADMEssenceParser*>(user_data);
    (void)atts;
    (void)qname;
    (void)parser;
    //log_info(">>> %s\n", qname);
}

void SADMEssenceParser::XML_EndElement(void *user_data, const char *qname)
{
    SADMEssenceParser *parser = static_cast<SADMEssenceParser*>(user_data);
    (void)qname;
    (void)parser;
    //log_info("<<< %s\n", qname);
}

void SADMEssenceParser::ParseMetadataPayload()
{
    log_info("Parse Metadata\n");
    if (mMetadataFormat == METADATA_FORMAT::GZIP) {
        // unzip first. for now. throw error
        throw InvalidDataError("GZIP not supported yet");
    }
    XML_Parser parser = XML_ParserCreate("utf-8");
    XML_SetStartElementHandler(parser, SADMEssenceParser::XML_StartElement);
    XML_SetEndElementHandler(parser, SADMEssenceParser::XML_EndElement);
    XML_SetUserData(parser, this);

    XML_Status status = XML_Parse(parser,
                                  (const char*)mMetadata.data(),
                                  mMetadata.size(),
                                  true);
    if (status == XML_STATUS_ERROR)
    {
        XML_Error err = XML_GetErrorCode(parser);
        if (err != XML_ERROR_ABORTED)
        {
            int32_t err_index = XML_GetErrorByteIndex(parser);
            throw BMXException("XML Error: %d (%s) (err_idx: %d, char: %c)\n",
                err, XML_ErrorString(err), err_index, mMetadata[err_index]);
        }
        throw BMXException("XML ABORTED error");
    }

    XML_ParserFree(parser);
}

uint8_t SADMEssenceParser::ReadByte(const unsigned char *data, uint32_t data_size)
{
    if (BytesAvailable(data_size)) {
        uint8_t tmp = data[mStreamPos];
        mStreamPos++;
        if (mSectionHeaderParsed) {
            mSectionPos++;
        }
        return tmp;
    }
    throw NotEnoughDataError();
}

uint32_t SADMEssenceParser::ReadBERLengthField(const unsigned char *data, uint32_t data_size, uint32_t *n)
{
    // make sure we have enough data
    if (BytesAvailable(data_size) < 4) {
        throw NotEnoughDataError();
    }

    log_info("streamPos: %d\n", mStreamPos);
    log_info("data[0]: %x\n", data[mStreamPos]);

    uint8_t byte0 = ReadByte(data, data_size);

    // short-form.
    if ((byte0 & 0x80) == 0) {
        log_info("aha");
        *n = 1;
        return byte0 & 0x7F;
    }

    // long-form
    // grab bits 1 - 7
    uint32_t berLength = byte0 & 0x7F;
    log_info("berLength: %d\n", berLength);
    *n = berLength;

    uint32_t length = 0;
    for (uint32_t i = 0; i < berLength; ++i) {
        length = (length << 8) | ReadByte(data, data_size);
    }

    return length;
}

uint32_t SADMEssenceParser::BytesAvailable(uint32_t data_size) const
{
    if (mSectionSize > 0) {
        uint32_t bytesAvailable = std::min(data_size - mStreamPos,
                                           mSectionSize - mSectionPos);
        return bytesAvailable;
    }
    return data_size - mStreamPos;

}

void SADMEssenceParser::ResetSectionInfo()
{
    mSectionHeaderParsed = 0;
    mSectionPos = 0;
    mSectionSize = 0;
}

void SADMEssenceParser::ResetFrameInfo()
{
    mStreamPos = 0;
    mMetadataHeaderParsed = false;
    mCurrentSectionIdentifier = SECTION_IDENTIFIER::AUDIO_ESSENCE,
    mSectionHeaderParsed = false;
    mSectionSize =0;
    mCurrentSectionIndex = 0;
    mSections = 0;
    mFrameLength = 0;
}
