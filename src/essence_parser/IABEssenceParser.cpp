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

#include <bmx/essence_parser/IABEssenceParser.h>
#include <bmx/BMXException.h>
#include "EssenceParserUtils.h"
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

// 10.2.3
namespace bmx {

#define IAB_PREAMBLE_TAG 0x01
#define IAB_IAFRAME_TAG  0x02

// 10.1.1
enum IABElementId {
    IAB_ELEMENT_IA_FRAME            = 0x08,
    IAB_ELEMENT_BED_DEFINITION      = 0x10,
    IAB_ELEMENT_BED_REMAP           = 0x20,
    IAB_OBJECT_DEFINITION           = 0x40,
    IAB_OBJECT_ZONE_DEFINITION19    = 0x80,
    AUTHORING_TOOL_INFO             = 0x100,
    USER_DATA                       = 0x101,
    AUDIO_DATA_DLC                  = 0x200,
    AUDIO_DATA_PCM                  = 0x400
};

inline void IsValidIABElement(IABElementId id)
{
    bool valid = (id == IAB_ELEMENT_IA_FRAME 
                  || id == IAB_ELEMENT_BED_DEFINITION
                  || id == IAB_ELEMENT_BED_REMAP
                  || id == IAB_OBJECT_DEFINITION
                  || id == IAB_OBJECT_ZONE_DEFINITION19
                  || id == AUTHORING_TOOL_INFO
                  || id == USER_DATA
                  || id == AUDIO_DATA_DLC
                  || id == AUDIO_DATA_PCM);

    if (!valid) {
        log_error("Invalid IAB Element ID: %04x\n", id);
    }
    BMX_ASSERT(valid);
}

};

class InvalidData : public exception
{
public:
    InvalidData(const char *message)
    : msg_(message) {}

    InvalidData(const std::string &message)
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

static uint32_t read_plex(BitstreamParser *parser, uint32_t num_bits)
{
    uint32_t oVal = 0;
    uint32_t bitsToRead = num_bits;
    uint32_t value = 0;

    while (bitsToRead <= 32)
    {
        value = parser->read(bitsToRead);
        uint64_t maxValue = 1;
        maxValue = (maxValue << bitsToRead) - 1;

        if (value < maxValue)
        {
            oVal = value;
            return oVal;
        }
        else
        {
            bitsToRead = bitsToRead << 1;
        }
    }
    return -1;
}

IABEssenceParser::IABEssenceParser()
: EssenceParser(), mBitstreamParser(new BitstreamParser())
{
    ResetFrameInfo();
}

IABEssenceParser::~IABEssenceParser()
{
    delete mBitstreamParser;
}

Rational IABEssenceParser::GetSampleRate() const
{
    BMX_ASSERT(mSampleRate != IAB_SAMPLERATE_UNDEFINED);

    switch (mSampleRate)
    {
        case IAB_SAMPLERATE_48000:
        return SAMPLING_RATE_48K;
        break;
        case IAB_SAMPLERATE_96000:
        return SAMPLING_RATE_96K;
        break;
        default:
        BMX_ASSERT(false);
    }
}

uint32_t IABEssenceParser::GetBitDepth() const
{
    switch (mBitDepth)
    {
        case IAB_BITDEPTH_16bit:
        return 16;
        break;
        case IAB_BITDEPTH_24bit:
        return 24;
        break;
        default:
        BMX_ASSERT(false);
    }
}

Rational IABEssenceParser::GetFrameRate() const
{
    BMX_ASSERT(mFrameRate != IAB_FRAMERATE_UNDEFINED);

    switch (mFrameRate)
    {
        case IAB_FRAMERATE_24000_1001:
        return FRAME_RATE_23976;
        break;
        case IAB_FRAMERATE_24:
        return FRAME_RATE_24;
        break;
        case IAB_FRAMERATE_25:
        return FRAME_RATE_25;
        break;
        case IAB_FRAMERATE_30:
        return FRAME_RATE_30;
        break;
        case IAB_FRAMERATE_48:
        return FRAME_RATE_48;
        break;
        case IAB_FRAMERATE_50:
        return FRAME_RATE_50;
        break;
        case IAB_FRAMERATE_60:
        return FRAME_RATE_60;
        break;
        case IAB_FRAMERATE_96:
        return FRAME_RATE_96;
        break;
        case IAB_FRAMERATE_100:
        return FRAME_RATE_100;
        break;
        case IAB_FRAMERATE_120:
        return FRAME_RATE_120;
        break;
        default:
        BMX_ASSERT(false);
    }
}

uint32_t IABEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    (void)data;
    return 0;
}

uint32_t IABEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    //log_info("IAB Parse: %d\n", data_size);
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // done with file
    if (data_size == 0) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    try
    {
        if (mFrameHeaderParsed == false) {
            ParseFrameInfo(data, data_size);
            mFrameHeaderParsed = true;
        }

        // done with frame.
        if (data_size >= mFrameLength) {
            // reset for next frame
            mFrameHeaderParsed = false;
            return mFrameLength;
        }

        return ESSENCE_PARSER_NULL_OFFSET;
    }
    catch (const InvalidData& ex)
    {
        log_error("Error parsing IAB: %s\n", ex.msg());
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
}


void IABEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    ResetFrameInfo();

    log_debug("Parse New IAB Frame\n");
    // for early break
    mNecessaryFrameInfoAcquired = false;

    mBitstreamParser->start(data, data_size);

    ReadPreamble();

    uint32_t tag = mBitstreamParser->read(8);

    if (tag != IAB_IAFRAME_TAG) {
        throw InvalidData("Invalid Frame Tag");
    }

    uint32_t frame_length = mBitstreamParser->read(32);
    log_debug("frame length: %d\n", frame_length);

    uint32_t offset = mBitstreamParser->getOffset();

    BMX_ASSERT(offset % 8 == 0);

    // Preamble + Frame Length
    mFrameLength = offset/8 + frame_length;

    ReadIAElement();
}

void IABEssenceParser::ReadPreamble()
{
    log_debug("Read Preamble\n");
    uint8_t preambleTag = mBitstreamParser->read(8);
    if (preambleTag != IAB_PREAMBLE_TAG) {
        throw InvalidData("Invalid Preamble Tag");
    }

    uint32_t preambleLength = mBitstreamParser->read(32);

    //printf("preamble length: %d\n", preambleLength);

    // skip preamble
    mBitstreamParser->skip(preambleLength * 8);
}

void IABEssenceParser::ReadIAElement()
{
    log_debug("ReadIAFrame()\n");
    // skip rest of stream if we have all info we need
    if (mNecessaryFrameInfoAcquired) {
        log_debug("Done\n");
        return;
    }

    IABElementId elementId = (IABElementId)read_plex(mBitstreamParser, 8);


    IsValidIABElement(elementId);

    uint32_t elementLength = read_plex(mBitstreamParser, 8);
    (void)elementLength;

    log_debug("elementId: %04x, elementLength: %d\n", elementId, elementLength);

    if (elementId == IAB_ELEMENT_IA_FRAME) 
    {
        // to get framerate, samplerate, bit depth
        ReadIAFrame();
    }
    else if (elementId == IAB_ELEMENT_BED_DEFINITION)
    {
        // to get channels
        //mNecessaryFrameInfoAcquired = true;
        ReadBedDefinition();
    }
    else 
    {
        // break. even if we dont have enough info yet. hopefully
        // this doesnt happen on first frame :-)
        mNecessaryFrameInfoAcquired = true;
    }
}

void IABEssenceParser::ReadIAFrame()
{
    log_debug("ReadIAFrame()\n");
    // version (8)
    // sample rate (2)
    // bit depth (2)
    // frame rate (4)
    uint8_t version = mBitstreamParser->read(8);
    
    BMX_ASSERT(version == 0x01);

    uint8_t samplerate = mBitstreamParser->read(2);
    uint8_t bitDepth = mBitstreamParser->read(2);
    uint8_t framerate = mBitstreamParser->read(4);

    
    // maxRendered (plex 8)
    uint8_t maxRendered = read_plex(mBitstreamParser, 8);

    // sub elements (8)
    uint32_t nSubElements = read_plex(mBitstreamParser, 8);

    log_debug("IAFrame: Version: %d, SampleRate: %d, BitDepth: %d, Framerate: %d, MaxRendered: %d, nSubElements: %d\n", version, samplerate, bitDepth, framerate, maxRendered, nSubElements);

    mVersion = version;
    mSampleRate = static_cast<IABSampleRate>(samplerate);
    mBitDepth = static_cast<IABBitDepth>(bitDepth);
    mFrameRate = static_cast<IABFrameRate>(framerate);

    for (uint32_t i =0 ; i < nSubElements; ++i) {

        ReadIAElement();
    }
}

void IABEssenceParser::ReadBedDefinition()
{
    log_debug("ReadBedDefinition()\n");
    // metaId (8)
    read_plex(mBitstreamParser, 8);
    
    // conditionalBed (1)
    uint8_t cond_bed = mBitstreamParser->read(1);
    if (cond_bed) {
        // bed use case (8)
        mBitstreamParser->read(8);
    }

    // channel count (4)
    uint32_t channelCount = read_plex(mBitstreamParser, 4);

    log_debug("Channel Count: %d\n", channelCount);

    BMX_ASSERT(channelCount != 0);
    mChannelCount = channelCount;

    // done. no more data needed
    log_debug("!!!! Set done flag\n");
    mNecessaryFrameInfoAcquired = true;
}

void IABEssenceParser::ResetFrameInfo()
{
    mBitstreamParser->reset();
    mNecessaryFrameInfoAcquired = false;
    mFrameLength = 0;
    mFrameHeaderParsed = false;
    mFrameRate = IAB_FRAMERATE_UNDEFINED;
    mSampleRate = IAB_SAMPLERATE_UNDEFINED;
    mBitDepth = IAB_BITDEPTH_UNDEFINED;
    mChannelCount = 0;
    mVersion = 0;
}
