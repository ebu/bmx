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

#define __STDC_FORMAT_MACROS

#include <bmx/essence_parser/HEVCRawEssenceReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

#define READ_BLOCK_SIZE         8192
#define PARSE_FRAME_START_SIZE  8192


HEVCRawEssenceReader::HEVCRawEssenceReader(EssenceSource *essence_source)
: RawEssenceReader(essence_source)
{
    
}

HEVCRawEssenceReader::~HEVCRawEssenceReader()
{
}

uint32_t HEVCRawEssenceReader::ReadSamples(uint32_t num_samples)
{
    if (mLastSampleRead)
        return 0;

    // shift data from previous read to start of sample data
    // note that this is needed even if mFixedSampleSize > 0 because the previous read could have occurred
    // when mFixedSampleSize == 0

    ShiftSampleData(0, mSampleDataSize);
    mSampleDataSize = 0;
    mNumSamples = 0;

    if (mFixedSampleSize == 0) {
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            if (!ReadAndParseHEVC())
                break;
        }
    } else {
        throw false;
    }

    return mNumSamples;
}

bool HEVCRawEssenceReader::ReadAndParseHEVC()
{
    BMX_CHECK(mEssenceParser);

    uint32_t sample_start_offset = mSampleDataSize;
    uint32_t sample_num_read = mSampleBuffer.GetSize() - sample_start_offset;
    uint32_t num_read;

    if (!mReadFirstSample) {
        // find the start of the first sample

        sample_num_read += ReadBytes(PARSE_FRAME_START_SIZE);
        uint32_t offset = mEssenceParser->ParseFrameStart(mSampleBuffer.GetBytes() + sample_start_offset, sample_num_read);
        if (offset == ESSENCE_PARSER_NULL_OFFSET) {
            log_warn("Failed to find start of raw essence sample\n");
            mLastSampleRead = true;
            return false;
        }

        // shift start of first sample to offset 0
        if (offset > 0) {
            ShiftSampleData(sample_start_offset, sample_start_offset + offset);
            sample_num_read -= offset;
        }

        mReadFirstSample = true;
    } else {
        sample_num_read += ReadBytes(READ_BLOCK_SIZE);
    }

    uint32_t sample_size = 0;
    while (true) {
        sample_size = mEssenceParser->ParseFrameSize(mSampleBuffer.GetBytes() + sample_start_offset, sample_num_read);
        if (sample_size != ESSENCE_PARSER_NULL_OFFSET) {
            break;
        }

        BMX_CHECK_M(mMaxSampleSize == 0 || mSampleBuffer.GetSize() - sample_start_offset <= mMaxSampleSize,
                   ("Max raw sample size (%u) exceeded", mMaxSampleSize));

        num_read = ReadBytes(READ_BLOCK_SIZE);
        if (num_read == 0) {
            // read last frame
            sample_size = mEssenceParser->ParseFrameSize(mSampleBuffer.GetBytes() + sample_start_offset, ESSENCE_PARSER_NULL_OFFSET);
            if (sample_size != ESSENCE_PARSER_NULL_OFFSET) {
                break;
            }
        }

        sample_num_read += num_read;
    }

    if (sample_size == ESSENCE_PARSER_NULL_FRAME_SIZE) {
        // invalid or null sample data
        mLastSampleRead = true;
        return false;
    } else if (sample_size == ESSENCE_PARSER_NULL_OFFSET) {
        // assume remaining data is valid sample data
        mLastSampleRead = true;
        if (sample_num_read > 0) {
            mSampleDataSize = mSampleBuffer.GetSize();
            mNumSamples++;
        }
        return false;
    }

    mSampleDataSize += sample_size;
    mNumSamples++;
    return true;
}