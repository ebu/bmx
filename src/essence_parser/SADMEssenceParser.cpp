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

#include <bmx/essence_parser/SADMEssenceParser.h>
#include <bmx/BMXException.h>
#include "EssenceParserUtils.h"
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

SADMEssenceParser::SADMEssenceParser()
    : EssenceParser(),
      mStreamPos(0)
{
    ResetFrameInfo();
}

SADMEssenceParser::~SADMEssenceParser()
{
}


uint32_t SADMEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    (void)data;
    return 0;
}

uint32_t SADMEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    // log_info("----------\n");
    // log_info("s-ADM Parse: %d\n", data_size);
    // log_info("\nmSections\t%d\nmCurrentSectionIndex\t%d\nmSkipSection\t%d\ncurrentDataSize\t%d\n",
    //mSections,
    //         mCurrentSectionIndex, mSkipSection,
    //         data_size - mStreamPos);
    // log_info("data_size\t%d\n, mStreamPos\t%d\n", data_size, mStreamPos);
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

        // done with file
    if (data_size == 0) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    // no sections yet means we need to parse the initial header
    if (!mSections) {
        // log_info("parse number sections\n");
        mSections = data[mStreamPos];
        mStreamPos += 1;
        if (mSections < 2) {
            log_error("Invalid s-ADM. Minimum of 2 sections required. Got: %d", mSections);
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }
        mFrameLength = 1;
    }

    do {
        if (!mSectionHeaderParsed && (data_size - mStreamPos) >= 6) {
            mCurrentSectionIndex = data[mStreamPos];
            mStreamPos += 1;
            uint32_t identifier = data[mStreamPos];
            mStreamPos += 1;
            // log_info("got section identifier: %X\n", identifier);
            mSectionSize = data[mStreamPos] << 24 | data[mStreamPos + 1] << 16 | data[mStreamPos + 2] << 8 | data[mStreamPos + 3];
            mStreamPos += 4;
            // log_info("got section size: %d\n", mSectionSize);
            mSectionHeaderParsed = true;
            mSectionPos = 0;

            mFrameLength += 6;
            mFrameLength += mSectionSize;
        }

        if (mSectionHeaderParsed) {
            // SKIP AUDIO AND FILL. Parse METADATA. But only for first frame.
            // Store all metadata in an array of structs that we can then use
            // to create the MGA Audio Metadat SubDescriptors. Because we need
            // one per metadata section
            // After first pass, we can skip

            // log_info("\nmStreamPos\t%d\nmSectionPos\t%d\nmSectionSize\t%d\ndata_size\t%d\n", mStreamPos, mSectionPos, mSectionSize, data_size);
            uint32_t skipBytes = std::min(data_size - mStreamPos,
                                          mSectionSize - mSectionPos);
            // log_info("skipBytes: %d\n", skipBytes);
            mStreamPos += skipBytes;
            mSectionPos += skipBytes;
        }

        if (mSectionPos > mSectionSize) {
            log_error("Something went wrong during parsing section\n");
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }

        if (mSectionPos == mSectionSize) {
            // log_info("!!! Section %d/%d done (%d)\n", mCurrentSectionIndex, mSections-1, mStreamPos);
            // log_info("mFrameLength: %d\n", mFrameLength);
            mSectionHeaderParsed = 0;
            mSectionPos = 0;
            mSectionSize = 0;
            if (mCurrentSectionIndex == mSections - 1) {
                // log_info("!!! LAST Section (%d) done\n", mCurrentSectionIndex);

                // log_info("%x\n", data[mStreamPos]);
                uint32_t tmp = mFrameLength;
                ResetFrameInfo();
                return tmp;
            }
        }
    } while (!mSectionHeaderParsed && (data_size - mStreamPos) >= 6);

    /*
    static int counter = 0;
    counter++;
    if (counter == 10) {
        return 0;
    }
    */

    /*
    // done with frame.
    if (data_size >= mFrameLength) {
        // log_info("out
        // reset for next frame
        mFrameHeaderParsed = false;
        return mFrameLength;
    }
    */

    return ESSENCE_PARSER_NULL_OFFSET;
}

void SADMEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    (void)data;
    (void)data_size;
    ResetFrameInfo();
}

void SADMEssenceParser::ResetFrameInfo()
{
    mStreamPos = 0;
    mSectionHeaderParsed = false;
    mSkipSection = false;
    mSectionSize =0;
    mCurrentSectionIndex = 0;
    mSections = 0;
    mFrameLength = 0;
}
