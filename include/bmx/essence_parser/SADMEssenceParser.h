/*
 * Copyright (C) 2023, Dolby Laboratories, Inc
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

#ifndef BMX_SADM_ESSENCE_PARSER_H_
#define BMX_SADM_ESSENCE_PARSER_H_

#include "mxf/mxf_types.h"
#include <map>
#include <vector>

#include <bmx/essence_parser/EssenceParser.h>


namespace bmx
{

class BitstreamParser;

struct SADMMetadataSectionInfo {
    uint8_t index;
    uint8_t identifier;
};

class SADMEssenceParser : public EssenceParser
{
public:
    SADMEssenceParser();
    virtual ~SADMEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

    uint32_t GetBitDepth() const { return mBitDepth; }
    mxfRational GetAudioSampleRate() const { return mSampleRate; }
    uint32_t GetCountMetadataSections() const { return mNumberMetadataSections; }
    uint32_t GetChannelCount() const { return mChannels; }

    const std::map<uint8_t, SADMMetadataSectionInfo>& GetSADMMetadataSectionInfo()
    {
        return mMetadataSectionInfo;
    }

private:
    enum SECTION_IDENTIFIER {
        AUDIO_ESSENCE = 0x00,
        AUDIO_METADATA_METADATA_PACK = 0x01,
        AUDIO_METADATA_PAYLOAD = 0x02,
        FILL_SECTION = 0xFF
    };

    enum METADATA_FORMAT {
        XML = 0x00,
        GZIP = 0x01
    };

    // resets whole frame. called in between MGAFrame
    void ResetFrameInfo();
    // resets sections info.
    void ResetSectionInfo();
    // parses a section. re-entrant.
    void ParseSection(const unsigned char *data, uint32_t data_size);
    // returns true if section can be skipped
    bool SkipSection() const;
    // returns number of bytes we still can obtain
    uint32_t BytesAvailable(uint32_t data_size) const;
    // parses metadata payload from mMetadata vector
    void ParseMetadataPayload();

    static void XML_StartElement(void *user_data, const char *qname, const char **atts);

    static void XML_EndElement(void *user_data, const char *qname);

    uint32_t ReadBERLengthField(const unsigned char *data, uint32_t data_size, uint32_t *n);

    uint8_t ReadByte(const unsigned char *data, uint32_t data_size);

    // is the section header parsed already?
    bool mSectionHeaderParsed;
    // size of the current section to be parsed
    uint32_t mSectionSize;
    // number of sections in mga frame
    uint32_t mSections;
    // index of current section
    uint32_t mCurrentSectionIndex;
    // position in overall stream. reset after each frame
    uint32_t mStreamPos;
    // position in current section. reset after end of section
    uint32_t mSectionPos;
    // length of frame. updated after each section header is parsed
    uint32_t mFrameLength;
    // identifier of current section
    SECTION_IDENTIFIER mCurrentSectionIdentifier;
    // flag to indicate whether metadata has already been parsed. This is NOT
    // reset per frame
    bool mMetadataParsed;
    // is metadata header parsed already?
    bool mMetadataHeaderParsed;
    // number Frames Parsed
    uint32_t mNumberFramesParsed;

    METADATA_FORMAT mMetadataFormat;

    // METADATA
    uint32_t mNumberMetadataSections;
    mxfRational mSampleRate;
    uint32_t mBitDepth;
    uint32_t mChannels;

    // metadata payload. gets overwritten for each section
    std::vector<unsigned char> mMetadata;

    // metadata info obtained during parsing of each metadata section
    std::map<uint8_t, SADMMetadataSectionInfo> mMetadataSectionInfo;
};

};



#endif
