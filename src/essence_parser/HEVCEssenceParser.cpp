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
#include <sstream>
#include <iomanip>

using namespace std;
using namespace bmx;

#define ESSENCE_PARSER_NEED_MORE_DATA ESSENCE_PARSER_NULL_OFFSET


/*

def print_nal(nal, n = 50):
    length = len(nal)
    str = []
    if n < 20: n = 20
    if length < n:
        for i in range(0, length):
            str.append('0x{:02x}'.format(nal[i]))
    else:
        for i in range(0, n//2):
            str.append('0x{:02x}'.format(nal[i]))
        str.append("........")
        for i in range(length - n//2, length):
            str.append('0x{:02x}'.format(nal[i]))
    
    print(', '.join(str))
    
    */

void print_buf(const unsigned char *data, uint32_t data_size, uint32_t n) {
    if (n < 20) {
        n = 20;
    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    if (data_size < n) {
        for (int i = 0; i < n; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i]) << ".";
        }
    } else {
        for (int i = 0; i < n/2; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i])  << ".";
        }
        ss << ".........";
        for (int i = data_size - n/2; i < data_size; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i]);
            if (i < data_size - 1) {
                 ss << ".";
            }
        }
    }
    log_info("%s\n", ss.str().c_str());
}


enum NAL_TYPE {
    NAL_UNKNOWN         = -1,
    // Coded slice segment of an IDR picture
    IDR_W_RADL          = 19,       
    IDR_N_LP            = 20,         // DR slice with no leading pictures

    // Video parameter set
    VPS_NUT             = 32,
    // Sequence parameter set
    SPS_NUT             = 33,
    // Picture parameter set
    PPS_NUT             = 34,

    // Access unit delimiter
    AUD_NUT             = 35,

    // Supplemental enhancement information
    PREFIX_SEI_NUT      = 39
};


HEVCEssenceParser::HEVCEssenceParser()
{   
    mNalUnitType = NAL_TYPE::NAL_UNKNOWN;
    mLastNalUnitType = NAL_TYPE::NAL_UNKNOWN;
    mFrameNum = 0;
    ResetFrameSize();
    ResetFrameInfo();
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
    log_info("\nParseFrameSize: %lu\n", data_size);
    log_info("mOffset: %d\n", mOffset);
    
    // End of stream
    if (data_size == 0) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
    if (mOffset == 0) {
        ResetFrameSize();
    }

    uint32_t end_pos = ESSENCE_PARSER_NULL_OFFSET;
    bool have_frame_end = false;
    
    if (data_size == ESSENCE_PARSER_NULL_OFFSET) {
        end_pos = mOffset + 5; // add the 5 deducted from loop below to get to EOS

        log_info("!!! EOS\n");
        log_info("---> GOT FULL ACCESS UNIT @ pos %d\n", end_pos);
        print_buf(data, end_pos, 50);
        have_frame_end = true;
    }

    // we want at least 5 bytes of space
    while (!have_frame_end && mOffset < data_size - 5) {
        if (data[mOffset] == 0x00 && data[mOffset+1] == 0x00 && data[mOffset+2] == 0x01) {

            // advance start code
            mOffset += 3;
            /* extract nal header */

            log_info("got start\n");
            uint8_t b1 = data[mOffset];

            // advance nal header
            mOffset += 2;

            uint32_t nal_unit_type = (b1 & 0x7e) >> 1;
            mLastNalUnitType = mNalUnitType;
            mNalUnitType = nal_unit_type;

            log_info("got unit type: %d\n", nal_unit_type);

            /*
            if ((mNalUnitType == NAL_TYPE::AUD_NUT && mFrameNum > 0) 
                || mLastNalUnitType == NAL_TYPE::IDR_W_RADL
                || mLastNalUnitType == NAL_TYPE::IDR_N_LP)
            */
            if (mLastNalUnitType == NAL_TYPE::IDR_W_RADL
                || mLastNalUnitType == NAL_TYPE::IDR_N_LP)
            {
                end_pos = mOffset - 5; // back to before start code and header

                assert(data[end_pos] == 0x00 && data[end_pos+1] == 0x00 && data[end_pos+2] == 0x01);

                // check if we have 00.00.00.01 or 00.00.01
                //if (data[end_pos -1 ] == 0x00)
                //    end_pos--;

                log_info("---> GOT FULL ACCESS UNIT @ pos %d\n", end_pos);
                print_buf(data, end_pos, 50);
                have_frame_end = true;
                break;
            }
        } else {
            mOffset++;
        }
    }
    log_info("data_size: %ld, mOffset: %ld\n", data_size, mOffset);
    // check if we have last frame
    /*
    if (!have_frame_end && data_size - mOffset <= 5) {
        end_pos = mOffset + (data_size - mOffset);
        log_info("---> GOT FULL ACCESS UNIT @ pos %d\n", end_pos);
        print_buf(data, end_pos, 50);
        have_frame_end = true;
    }
    */
    

    uint32_t frame_size = ESSENCE_PARSER_NULL_OFFSET; // need more data
    if (have_frame_end) {
        frame_size = end_pos;
        log_info("FRAME SIZE: %d\n", frame_size);
        mFrameNum++;
        log_info("Got frame: %d\n", mFrameNum);
        // done. start again!
        mOffset = 0;
    }

    return frame_size;
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

