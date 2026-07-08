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

#ifndef BMX_IAB_ESSENCE_PARSER_H_
#define BMX_IAB_ESSENCE_PARSER_H_

#include <map>
#include <vector>

#include <bmx/essence_parser/EssenceParser.h>


namespace bmx
{

// 10.2.2 (2bits)
enum IABSampleRate {
    IAB_SAMPLERATE_48000    = 0x00,
    IAB_SAMPLERATE_96000    = 0x01,

    IAB_SAMPLERATE_UNDEFINED = 0xFF
};

// 10.2.3
enum IABBitDepth {
    IAB_BITDEPTH_16bit      = 0x00,
    IAB_BITDEPTH_24bit      = 0x01,

    IAB_BITDEPTH_UNDEFINED  = 0xFF
};

// 10.2.4 (4bits)
enum IABFrameRate {
    IAB_FRAMERATE_24         = 0x00,
    IAB_FRAMERATE_25         = 0x01,
    IAB_FRAMERATE_30         = 0x02,
    IAB_FRAMERATE_48         = 0x03,
    IAB_FRAMERATE_50         = 0x04,
    IAB_FRAMERATE_60         = 0x05,
    IAB_FRAMERATE_96         = 0x06,
    IAB_FRAMERATE_100        = 0x07,
    IAB_FRAMERATE_120        = 0x08,
    IAB_FRAMERATE_24000_1001 = 0x09,

    IAB_FRAMERATE_UNDEFINED  = 0xFF
};

class BitstreamParser;

class IABEssenceParser : public EssenceParser
{
public:
    IABEssenceParser();
    virtual ~IABEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    bool HasFrameRate() const { return mFrameRate != IAB_FRAMERATE_UNDEFINED; }
    
    Rational GetFrameRate() const;
    Rational GetSampleRate() const;
    uint32_t GetBitDepth() const;
    uint32_t GetChannelCount() const { return mChannelCount; }

private:
    void ResetFrameInfo();
    void ReadPreamble();
    void ReadIAElement();
    void ReadIAFrame();
    void ReadBedDefinition();

private:
    BitstreamParser *mBitstreamParser;

    uint32_t mFrameLength;
    bool mFrameHeaderParsed;

    IABFrameRate mFrameRate;
    IABSampleRate mSampleRate;
    IABBitDepth mBitDepth;
    uint8_t mVersion;

    uint32_t mChannelCount;

    bool mNecessaryFrameInfoAcquired;
};


};



#endif
