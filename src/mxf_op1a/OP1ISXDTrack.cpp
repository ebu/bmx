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

#include <bmx/mxf_op1a/OP1AISXDTrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/apps/AppMCALabelHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>
#include <bmx/essence_parser/ISXDEssenceParser.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


OP1AISXDTrack::OP1AISXDTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate, EssenceType essence_type)
: OP1ATrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    BMX_ASSERT(essence_type == ISXD);

    const int elecount = 0x01;
    const int elenum = 0x00;

    mEssenceElementKey = MXF_ISXD_EE_K(elecount, MXF_ISXD_FRAME_WRAPPED_EE_TYPE, elenum);
    mTrackNumber = MXF_ISXD_TRACK_NUM(elecount, MXF_ISXD_FRAME_WRAPPED_EE_TYPE, elenum);

    mISXDDescriptorHelper = dynamic_cast<ISXDDescriptorHelper *>(mDescriptorHelper);
    BMX_ASSERT(mISXDDescriptorHelper);

    mPosition = 0;

    mCbe =  false;
}

OP1AISXDTrack::~OP1AISXDTrack()
{
}

void OP1AISXDTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterISXDTrackElement(mTrackIndex, mEssenceElementKey);

    mIndexTable->RegisterISXDTrackElement(mTrackIndex, mCbe);
}


void OP1AISXDTrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (mPosition == 0) {
        //log_info("Write samples int\n");
        ISXDEssenceParser essence_parser;
        essence_parser.ParseFrameInfo(data, size);
        //log_info("Update FD\n");
        mISXDDescriptorHelper->UpdateFileDescriptor(&essence_parser);
    }

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    if (!mCbe) {
        mIndexTable->AddIndexEntry(mTrackIndex, mPosition, 0, 0, 0, true, false);
    }

    mPosition++;
}
