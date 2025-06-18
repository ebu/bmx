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

#include <bmx/mxf_op1a/OP1AIABTrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/apps/AppMCALabelHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>
#include <bmx/essence_parser/IABEssenceParser.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;

OP1AIABTrack::OP1AIABTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate, EssenceType essence_type)
: OP1ATrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    BMX_ASSERT(essence_type == IAB);

    const int elecount = 0x01;
    const int elenum = 0x00;

    mEssenceElementKey = MXF_IAB_EE_K(elecount, MXF_IAB_FRAME_WRAPPED_EE_TYPE, elenum);
    //060e2b34.01020101.0d010301.16nn10cc
    mTrackNumber = MXF_IAB_TRACK_NUM(elecount, MXF_IAB_FRAME_WRAPPED_EE_TYPE, elenum);

    mIABDescriptorHelper = dynamic_cast<IABDescriptorHelper *>(mDescriptorHelper);
    BMX_ASSERT(mIABDescriptorHelper);

    // Essence Coding (4.10, SMPTE ST 2098-2)
    mxfUL label = {0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x05, 0x0E, 0x09, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00};
    mIABDescriptorHelper->SetSoundEssenceCompression(label);
    // according to spec, channel count in IAB is never set 
    mIABDescriptorHelper->SetChannelCount(0);

    mPosition = 0;

    // IAB always has variable bitrate
    mCbe = false;
}

OP1AIABTrack::~OP1AIABTrack()
{
}

void OP1AIABTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterIABTrackElement(mTrackIndex, mEssenceElementKey);

    mIndexTable->RegisterIABTrackElement(mTrackIndex, mCbe);
}


void OP1AIABTrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    if (!mCbe) {
        mIndexTable->AddIndexEntry(mTrackIndex, mPosition, 0, 0, 0, true, false);
    }

    mPosition++;
}

void OP1AIABTrack::SetSamplingRate(Rational sample_rate)
{
    // audio sample rate (e.g. 48000/1). 
    mIABDescriptorHelper->SetSamplingRate(sample_rate);
}

void OP1AIABTrack::SetReferenceImageEditRate(Rational edit_rate)
{
    mIABDescriptorHelper->SetReferenceImageEditRate(edit_rate);
}

void OP1AIABTrack::SetQuantizationBits(uint32_t bitDepth)
{
    mIABDescriptorHelper->SetQuantizationBits(bitDepth);
}

void OP1AIABTrack::SetChannelCount(uint32_t channelCount)
{
    mIABDescriptorHelper->SetChannelCount(channelCount);
}
