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

#include <bmx/mxf_helper/IABDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    mxfUL ec_label;
    EssenceType essence_type;
    bool frame_wrapped;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    // EssenceContainerUL
    {MXF_EC_L(IABFrameWrapped),     IAB,     true}
};



EssenceType IABDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool IABDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

IABDescriptorHelper::IABDescriptorHelper()
: SoundMXFDescriptorHelper()
{
    mEssenceType = IAB;
    mEssenceCompressionLabel = g_Null_UL;
}

IABDescriptorHelper::~IABDescriptorHelper()
{
}

void IABDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                         mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    SoundMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label)) {
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
            break;
        }
    }
    BMX_ASSERT(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

void IABDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    // note: this is framerate. Set in Op1ATrack constructor
    mSampleRate = sample_rate;
}

void IABDescriptorHelper::SetSoundEssenceCompression(mxfUL label)
{
    mEssenceCompressionLabel = label;
}

FileDescriptor* IABDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new IABDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void IABDescriptorHelper::UpdateFileDescriptor()
{
    SoundMXFDescriptorHelper::UpdateFileDescriptor();

    IABDescriptor *descriptor = dynamic_cast<IABDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    descriptor->setSoundEssenceCompression(mEssenceCompressionLabel);
    // todo: Set ReferenceAudioalignmentlevel if possible
}

void IABDescriptorHelper::UpdateFileDescriptor(mxfpp::FileDescriptor *file_desc_in)
{
    IABDescriptor *iab_descriptor = dynamic_cast<IABDescriptor*>(mFileDescriptor);
    BMX_ASSERT(iab_descriptor);

    IABDescriptor *iab_desc_in = dynamic_cast<IABDescriptor*>(file_desc_in);
    BMX_CHECK(iab_desc_in);
}

uint32_t IABDescriptorHelper::GetSampleSize()
{
    return 0;
}

mxfUL IABDescriptorHelper::ChooseEssenceContainerUL() const
{
    BMX_ASSERT(mFrameWrapped);
    mxfUL ul = MXF_EC_L(IABFrameWrapped);
    return ul;
}
