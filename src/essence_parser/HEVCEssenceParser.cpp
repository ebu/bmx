/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cmath>
#include <set>

#include <bmx/essence_parser/AVCEssenceParser.h> // for AVCGetBitBuffer
#include <bmx/essence_parser/HEVCEssenceParser.h>
#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
#include <bmx/BitBuffer.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

#define ESSENCE_PARSER_NEED_MORE_DATA ESSENCE_PARSER_NULL_OFFSET

HEVCEssenceParser::HEVCEssenceParser() : 
    mOffset(0)
{   
}

HEVCEssenceParser::~HEVCEssenceParser()
{
}

uint32_t HEVCEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // the access unit shall start with a zero_byte followed by start_code_prefix_one_3byte

    uint32_t offset = NextStartCodePrefix(data, data_size);
    if (offset == ESSENCE_PARSER_NULL_OFFSET)
        return ESSENCE_PARSER_NULL_OFFSET;

    if (offset == 0 || data[offset - 1] != 0x00) {
        log_warn("HEVC: Missing zero_byte before start_code_prefix_one_3byte at access unit start\n");
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    log_info("YO: %d\n", offset);

    return offset - 1;
}

uint32_t HEVCEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    log_info("ParseFrameSize: %lu\n", data_size);
    BMX_CHECK(data_size < ESSENCE_PARSER_NULL_OFFSET);
    //BMX_CHECK(mOffset < data_size);

    if (mOffset == 0) {
        ResetFrameSize();
    }

    return 0;
    //return ESSENCE_PARSER_NULL_OFFSET;
}

void HEVCEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    log_info("ParseFrameInfo: %lu\n", data_size);
    ResetFrameInfo();
}

uint32_t HEVCEssenceParser::NextStartCodePrefix(const unsigned char *data, uint32_t size)
{
    const unsigned char *datap3 = data + 3;
    const unsigned char *end    = data + size;

    // loop logic is based on FFmpeg's avpriv_find_start_code in libavcodec/utils.c
    while (datap3 < end) {
        if (datap3[-1] > 1)
            datap3 += 3;
        else if (datap3[-2])
            datap3 += 2;
        else if (datap3[-3] | (datap3[-1] - 1))
            datap3++;
        else
            break;
    }
    if (datap3 < end)
        return (uint32_t)(datap3 - data) - 3;
    else
        return ESSENCE_PARSER_NULL_OFFSET;
}


void HEVCEssenceParser::ResetFrameSize()
{

}

void HEVCEssenceParser::ResetFrameInfo()
{
    mOffset = 0;
}

EssenceType HEVCEssenceParser::GetEssenceType() const
{
    return HEVC_MAIN;
}

