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

typedef struct {
    // offset how many bits we are in.
    uint64_t bit_offset;
} IABParserContext;

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

};

class InvalidData : public exception
{
public:
    InvalidData(const char *message)
    : msg_(message) {}

    virtual const char* what() const noexcept {
       return msg_.c_str();
    }

protected:
    std::string msg_;
};

static uint32_t read_bits(const unsigned char *data, uint32_t data_size, uint32_t &bit_offset, int num_bits)
{
    uint32_t d = get_bits(data, data_size, bit_offset, num_bits);
    bit_offset += num_bits;
    return d;
}

static uint32_t read_plex(const unsigned char *data, uint32_t data_size, uint32_t &bit_offset, int num_bits)
{
    uint32_t oVal = 0;
    uint32_t bitsToRead = num_bits;
    uint32_t value = 0;
    
    while (bitsToRead <= 32)
    {
        value = read_bits(data, data_size, bit_offset, bitsToRead);
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
: EssenceParser()
{
    ResetFrameInfo();
}

IABEssenceParser::~IABEssenceParser()
{
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

    ByteBuffer data_reader(data, data_size, true);
    try
    {
        // skip preamble. and find position where actual IAFrame payload begins
        uint8_t preambleTag = data_reader.GetUInt8();
        if (preambleTag != 0x01) {
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }
        uint32_t mPreambleLength = data_reader.GetUInt32();
        data_reader.Skip(mPreambleLength);

        uint8_t frameTag = data_reader.GetUInt8();
        if (frameTag != 0x02) {
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }
    }
    catch (const InsufficientBytes&)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    return 0;
}

uint32_t IABEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
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
            //log_info("got new frame with length: %d\n", mFrameLength);
            //log_info("sample rate: %d\n", GetSampleRate());
            //log_info("bit_depth: %d\n", GetBitDepth());
            //log_info("channel count: %d\n", GetChannelCount());
        }

        // done with frame.
        if (data_size >= mFrameLength) {
            //log_info("frame done\n");
            // reset for next frame
            mFrameHeaderParsed = false;
            return mFrameLength;
        }

        return ESSENCE_PARSER_NULL_OFFSET;
    }
    catch (const InsufficientBytes&)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
    catch (const InvalidData& ex)
    {
        log_error("Error parsing Iab: %s\n", ex.what());
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
}


void IABEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    ResetFrameInfo();
    // for early break
    mNecessaryFrameInfoAcquired = false;
    uint32_t offset = 0;

    ReadPreamble(data, data_size, offset);

    uint32_t tag = read_bits(data, data_size, offset, 8);

    if (tag != 0x02) {
        throw InvalidData("Invalid Preamble");
    }


    uint32_t frame_length = read_bits(data, data_size, offset, 32);

    BMX_ASSERT(offset % 8 == 0);
    
    // Preamble + Frame Length
    mFrameLength = offset/8 + frame_length;

    ReadIAElement(data, data_size, offset);
}

void IABEssenceParser::ReadPreamble(const unsigned char *data, uint32_t data_size, uint32_t &offset)
{
    uint8_t preambleTag = read_bits(data, data_size, offset, 8);
    if (preambleTag != 0x01) {
        throw InvalidData("Invalid Preamble Tag");
    }

    uint32_t preambleLength = read_bits(data, data_size, offset, 32);

    // skip preamble
    offset += preambleLength * 8;
}

void IABEssenceParser::ReadIAElement(const unsigned char *data, uint32_t data_size, uint32_t &offset)
{
    if (mNecessaryFrameInfoAcquired) {
        return;
    }
    IABElementId elementId = (IABElementId)read_plex(data, data_size, offset, 8);

    //log_info("element id: %04x\n", elementId);

    /**/
    uint32_t elementLength = read_plex(data, data_size, offset, 8);
    (void)elementLength;
    //log_info("element length[0]: %04x, offset: %d\n", elementLength, offset);

    if (elementId == IAB_ELEMENT_IA_FRAME) 
    {
        // to get framerate, samplerate, bit depth
        ReadIAFrame(data, data_size, offset);
    }
    else if (elementId == IAB_ELEMENT_BED_DEFINITION)
    {
        // to get channels
        ReadBedDefinition(data, data_size, offset);
    }
}

void IABEssenceParser::ReadIAFrame(const unsigned char *data, uint32_t data_size, uint32_t &offset)
{
    // version (8)
    // sample rate (2)
    // bit depth (2)
    // frame rate (4)
    // max rendered (plex 8)

    uint8_t version = read_bits(data, data_size, offset, 8);
    uint8_t samplerate = read_bits(data, data_size, offset, 2);
    uint8_t bitDepth = read_bits(data, data_size, offset, 2);
    uint8_t framerate = read_bits(data, data_size, offset, 4);
    
    // maxRendered
    read_plex(data, data_size, offset, 8);

    uint32_t nSubElements = read_plex(data, data_size, offset, 8);

    mVersion = version;
    mSampleRate = static_cast<IABSampleRate>(samplerate);
    mBitDepth = static_cast<IABBitDepth>(bitDepth);
    mFrameRate = static_cast<IABFrameRate>(framerate);

    for (uint32_t i =0 ; i < nSubElements; ++i) {
        ReadIAElement(data, data_size, offset);
    }
}

void IABEssenceParser::ReadBedDefinition(const unsigned char *data, uint32_t data_size, uint32_t &offset)
{
    // metaId
    read_plex(data, data_size, offset, 8);
    // conditionalBed
    uint8_t cond_bed = read_bits(data, data_size, offset, 1);
    if (cond_bed) {
        // bed use case.
        read_bits(data, data_size, offset, 8);
    }

    // 5 bits in
    uint32_t channelCount = read_plex(data, data_size, offset, 4);
    BMX_ASSERT(channelCount != 0);
    mChannelCount = channelCount;
    mNecessaryFrameInfoAcquired = true;
}

void IABEssenceParser::ResetFrameInfo()
{
    mFrameLength = 0;
    mFrameHeaderParsed = false;
    mFrameRate = IAB_FRAMERATE_UNDEFINED;
    mSampleRate = IAB_SAMPLERATE_UNDEFINED;
    mBitDepth = IAB_BITDEPTH_UNDEFINED;
    mChannelCount = 0;
    mVersion = 0;
}