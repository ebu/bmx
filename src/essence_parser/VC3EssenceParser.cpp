/*
 * Copyright (C) 2010, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
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

#include <algorithm>

#include <bmx/essence_parser/VC3EssenceParser.h>
#include "EssenceParserUtils.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define VC3_MIN_HEADER_SIZE     0x280
#define VC3_OFFSETS_START       0x168
#define VARIABLE                0

typedef struct
{
    uint32_t compression_id;
    bool is_progressive;
    uint16_t frame_width;
    uint16_t frame_height;
    uint8_t bit_depth;
    uint32_t frame_size;
    Rational packet_scale;
} CompressionParameters;

static const CompressionParameters COMPRESSION_PARAMETERS[] =
{
    {1235,  true,   1920,     1080,     10,  917504,   ZERO_RATIONAL},
    {1237,  true,   1920,     1080,     8,   606208,   ZERO_RATIONAL},
    {1238,  true,   1920,     1080,     8,   917504,   ZERO_RATIONAL},
    {1241,  false,  1920,     1080,     10,  917504,   ZERO_RATIONAL},
    {1242,  false,  1920,     1080,     8,   606208,   ZERO_RATIONAL},
    {1243,  false,  1920,     1080,     8,   917504,   ZERO_RATIONAL},
    {1244,  false,  1920,     1080,     8,   606208,   ZERO_RATIONAL},
    {1250,  true,   1280,     720,      10,  458752,   ZERO_RATIONAL},
    {1251,  true,   1280,     720,      8,   458752,   ZERO_RATIONAL},
    {1252,  true,   1280,     720,      8,   303104,   ZERO_RATIONAL},
    {1253,  true,   1920,     1080,     8,   188416,   ZERO_RATIONAL},
    {1258,  true,   1280,     720,      8,   212992,   ZERO_RATIONAL},
    {1259,  true,   1920,     1080,     8,   417792,   ZERO_RATIONAL},
    {1260,  false,  1920,     1080,     8,   417792,   ZERO_RATIONAL},
    {1270,  false,  VARIABLE, VARIABLE, 12,  VARIABLE, {0xe000, 0xff}},  // DNxHR 444 12-bit
    {1271,  false,  VARIABLE, VARIABLE, 12,  VARIABLE, {0x7000, 0xff}},  // DNxHR HQX 12-bit
    {1272,  false,  VARIABLE, VARIABLE, 8,   VARIABLE, {0x7000, 0xff}},  // DNxHR HQ
    {1273,  false,  VARIABLE, VARIABLE, 8,   VARIABLE, {0x4a00, 0xff}},  // DNxHR SQ
    {1274,  false,  VARIABLE, VARIABLE, 8,   VARIABLE, {0x1700, 0xff}},  // DNxHR LB
};

static uint32_t get_hr_frame_size(CompressionParameters const& cp, uint32_t w, uint32_t h)
{
    BMX_CHECK(cp.frame_size == VARIABLE);
    BMX_CHECK(w > 0 && h > 0);

    uint32_t result = ((w + 15) / 16) * ((h + 15) / 16) * cp.packet_scale.numerator / cp.packet_scale.denominator;
    result = (result + 2048) & ~0xFFF;

    return max(result, (uint32_t)8192);
}

static uint32_t get_uint32(const unsigned char *data)
{
    return (((uint32_t)data[0]) << 24) |
           (((uint32_t)data[1]) << 16) |
           (((uint32_t)data[2]) << 8) |
             (uint32_t)data[3];
}

static uint16_t get_uint16(const unsigned char *data)
{
    return (((uint16_t)data[0]) << 8) |
             (uint16_t)data[1];
}

static bool vc3_is_header(const unsigned char *data, uint32_t data_size)
{
    if (data_size < VC3_MIN_HEADER_SIZE) return false;

    uint8_t version = data[4];      // 1 or 2 up to full HD, 3 for larger resolutions
    uint8_t interlaced = data[5];   // 1, 2 or 3

    if (version >= 1 && version <= 3 && interlaced >= 1 && interlaced <= 3)
    {
        uint32_t header_size = get_uint32(data);
        uint32_t offsets_size = get_uint32(data + VC3_OFFSETS_START);
        uint32_t nb_offsets = get_uint16(data + VC3_OFFSETS_START + 4);

        return (((version < 3 && header_size == VC3_MIN_HEADER_SIZE)
            || (version == 3 && header_size >= VC3_MIN_HEADER_SIZE))
            && header_size == VC3_OFFSETS_START + 4 + offsets_size
            && offsets_size == nb_offsets * 4 + 4);
    }

    return false;
}

static uint8_t vc3_get_interlaced(const unsigned char *data)
{
    return data[5] & 0x03;
}

static uint16_t vc3_get_width(const unsigned char *data)
{
    return get_uint16(data + 0x1a);
}

static uint16_t vc3_get_height(const unsigned char *data)
{
    bool interlaced = vc3_get_interlaced(data) != 1;
    return get_uint16(data + 0x18) << interlaced;
}

static uint32_t vc3_get_compression_id(const unsigned char *data)
{
    return get_uint32(data + 0x28);
}



VC3EssenceParser::VC3EssenceParser()
{
    mCompressionId = 0;
    mIsProgressive = false;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mBitDepth = 0;
    mFrameSize = 0;
}

VC3EssenceParser::~VC3EssenceParser()
{
}

uint32_t VC3EssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    for (uint32_t i = 0; i < data_size; i++) {
        if (vc3_is_header(data + i, data_size - i) &&
            vc3_get_interlaced(data + i) < 3)   // coding unit is progressive frame or field 1
        {
            return i;
        }
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

void VC3EssenceParser::ResetParseFrameSize()
{
}

uint32_t VC3EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    if (data_size < VC3_MIN_HEADER_SIZE)
        return ESSENCE_PARSER_NULL_OFFSET;

    // check header
    BMX_CHECK(vc3_is_header(data, data_size));

    uint32_t compression_id = vc3_get_compression_id(data);

    for (size_t i = 0; i < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS); i++)
    {
        if (compression_id == COMPRESSION_PARAMETERS[i].compression_id) {
            uint32_t frame_size = COMPRESSION_PARAMETERS[i].frame_size;

            if (frame_size == VARIABLE)
            {
                uint32_t w = vc3_get_width(data);
                uint32_t h = vc3_get_height(data);
                frame_size = get_hr_frame_size(COMPRESSION_PARAMETERS[i], w, h);
            }

            if (data_size >= frame_size)
                return frame_size;
            else
                return ESSENCE_PARSER_NULL_OFFSET;
        }
    }

    return ESSENCE_PARSER_NULL_FRAME_SIZE;
}

void VC3EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    BMX_CHECK(data_size >= VC3_MIN_HEADER_SIZE);
    BMX_CHECK(vc3_is_header(data, data_size));

    // compression id
    mCompressionId = vc3_get_compression_id(data);
    size_t param_index;
    for (param_index = 0; param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS); param_index++)
    {
        if (mCompressionId == COMPRESSION_PARAMETERS[param_index].compression_id)
            break;
    }
    BMX_CHECK(param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS));

    CompressionParameters const& cp = COMPRESSION_PARAMETERS[param_index];

    // Note: found that an Avid MC v3.0 file containing 1252 720p50 had FFC=01h and SST=1;
    //       SST should be 0 for progressive scan. DNxHD_Compliance_Issue_To_Licensees-1.doc
    //       states that some Avid bitstreams may have SST incorrectly set to 1080i
    //       Ignore the bitstream information and use the scan type associated with the compression id
    mIsProgressive = cp.is_progressive;

    // image geometry
    mFrameWidth = vc3_get_width(data);
    mFrameHeight = vc3_get_height(data);

    // Note: DNxHD_Compliance_Issue_To_Licensees-1.doc states that some Avid bitstreams,
    //       e.g. produced by Avid Media Composer 3.0, may have ALPF incorrectly set to 1080 for
    //       1080i sources. Ignore the bitstream information and use the frame height associated
    //       with the compression id
    if (cp.frame_height != VARIABLE) {
        mFrameHeight = cp.frame_height;
    }

    uint32_t sbd_bits = get_bits(data, data_size, 33 * 8, 3);
    BMX_CHECK(sbd_bits == 2 || sbd_bits == 1);
    mBitDepth = (sbd_bits == 2 ? 10 : 8);

    if (cp.frame_size == VARIABLE) {
        mFrameSize = get_hr_frame_size(cp, mFrameWidth, mFrameHeight);
    } else {
        mFrameSize = cp.frame_size;
    }

    BMX_CHECK(cp.frame_width == VARIABLE || mFrameWidth == cp.frame_width);
    BMX_CHECK(mBitDepth >= 8 && mBitDepth <= cp.bit_depth);
}
