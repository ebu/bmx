/*
 * Copyright (C) 2010, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Markus Pfundstein
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

#include "bmx/BMXTypes.h"
#include "mxf/mxf_types.h"
#include <cmath>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/mxf_helper/SADMDescriptorHelper.h>
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
    {MXF_EC_L(SADMFrameWrapped),     SADM,     true}
};



EssenceType SADMDescriptorHelper::IsSupported(FileDescriptor *file_descriptor,
                                              mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool SADMDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

SADMDescriptorHelper::SADMDescriptorHelper()
: SoundMXFDescriptorHelper()
{
    mEssenceType = SADM;
    mEssenceCompressionLabel = g_Null_UL;
}

SADMDescriptorHelper::~SADMDescriptorHelper()
{
}

void SADMDescriptorHelper::Initialize(FileDescriptor *file_descriptor,
                                      uint16_t mxf_version,
                                      mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    SoundMXFDescriptorHelper::Initialize(file_descriptor,
                                         mxf_version,
                                         alternative_ec_label);

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

void SADMDescriptorHelper::SetSoundEssenceCompression(mxfUL label)
{
    mEssenceCompressionLabel = label;
}

void SADMDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    // note: this is edit rate. Set in Op1ATrack constructor
    mSampleRate = sample_rate;
}

FileDescriptor* SADMDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new SADMDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void SADMDescriptorHelper::UpdateFileDescriptor()
{
    SoundMXFDescriptorHelper::UpdateFileDescriptor();

    SADMDescriptor *descriptor = dynamic_cast<SADMDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    descriptor->setSoundEssenceCompression(mEssenceCompressionLabel);
    descriptor->setSampleRate(mSampleRate);
    descriptor->setQuantizationBits(0);
    descriptor->setChannelCount(0);
    descriptor->setAudioSamplingRate(ZERO_RATIONAL);
}

void SADMDescriptorHelper::UpdateFileDescriptor(mxfpp::FileDescriptor *file_desc_in)
{
    SADMDescriptor *descriptor = dynamic_cast<SADMDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    SADMDescriptor *desc_in = dynamic_cast<SADMDescriptor*>(file_desc_in);
    BMX_CHECK(desc_in);
}

void SADMDescriptorHelper::UpdateAverageBytesPerSecond(uint32_t avgBytes)
{
    SADMDescriptor *descriptor = dynamic_cast<SADMDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    descriptor->setMGASoundEssenceAverageBytesPerSecond(avgBytes);
}

void SADMDescriptorHelper::UpdateFileDescriptor(SADMEssenceParser *essence_parser)
{
    SADMDescriptor *descriptor = dynamic_cast<SADMDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    // add stuff from first parse
    uint32_t bitDepth = essence_parser->GetBitDepth();
    descriptor->setQuantizationBits(essence_parser->GetBitDepth());
    descriptor->setAudioSamplingRate(essence_parser->GetAudioSampleRate());
    // I assume we have one channel PER audio and block align refers to this
    // (and not to the whole array of files where each file is one channel)
    descriptor->setMGASoundEssenceBlockAlign(1.0 * std::floor((float)(bitDepth + 7.0) / 8.0));
    descriptor->setMGASoundEssenceSequenceOffset(0);

    int nSubDescriptors = essence_parser->GetCountMetadataSections();
    printf("\nadd n subs: %d\n", nSubDescriptors);
}

uint32_t SADMDescriptorHelper::GetSampleSize()
{
    return 0;
}

mxfUL SADMDescriptorHelper::ChooseEssenceContainerUL() const
{
    BMX_ASSERT(mFrameWrapped);
    mxfUL ul = MXF_EC_L(SADMFrameWrapped);
    return ul;
}
