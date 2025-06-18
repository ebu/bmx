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

#include "bmx/BMXTypes.h"
#include "bmx/essence_parser/SADMEssenceParser.h"
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
    mMGAAudioMetadataSubDescriptors.clear();
    mSADMAudioMetadataSubDescriptors.clear();
    mMGASoundfieldGroupLabelSubDescriptors.clear();
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

    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (size_t i = 0; i < sub_descriptors.size(); ++i) {
            MGAAudioMetadataSubDescriptor *mga = dynamic_cast<MGAAudioMetadataSubDescriptor*>(sub_descriptors[i]);
            if (mga) {
                printf("SADMDescriptorHelper::Initialize. Push MGASubDesc\n");
                mMGAAudioMetadataSubDescriptors.push_back(mga);
                continue;
            }
            SADMAudioMetadataSubDescriptor *sadm = dynamic_cast<SADMAudioMetadataSubDescriptor*>(sub_descriptors[i]);
            if (sadm) {
                printf("SADMDescriptorHelper::Initialize. Push SADMSubDesc\n");
                mSADMAudioMetadataSubDescriptors.push_back(sadm);
                continue;
            }
            MGASoundfieldGroupLabelSubDescriptor *sfgl = dynamic_cast<MGASoundfieldGroupLabelSubDescriptor*>(sub_descriptors[i]);
            if (sfgl) {
                mMGASoundfieldGroupLabelSubDescriptors.push_back(sfgl);
            }
        }
    }
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
    descriptor->setChannelCount(essence_parser->GetChannelCount());
    descriptor->setMGASoundEssenceBlockAlign(essence_parser->GetChannelCount() * std::floor((float)(bitDepth + 7.0) / 8.0));
    descriptor->setMGASoundEssenceSequenceOffset(0);

    uint32_t avgBytes = essence_parser->GetChannelCount() * essence_parser->GetAudioSampleRate().numerator * bitDepth / 8.0;

    descriptor->setMGASoundEssenceAverageBytesPerSecond(avgBytes);

    for (auto it = essence_parser->GetSADMMetadataSectionInfo().cbegin();
         it != essence_parser->GetSADMMetadataSectionInfo().cend();
         ++it)
    {
        const SADMMetadataSectionInfo& info = it->second;
        if (info.identifier != 0x02) {
            throw BMXException("Header metadata identifier 0x01 not supported yet.");
        }
        HeaderMetadata *hm = mFileDescriptor->getHeaderMetadata();
        MGAAudioMetadataSubDescriptor *mga = new MGAAudioMetadataSubDescriptor(hm);

        // that correct?
        mxfUUID linkId = generate_uuid();
        mga->setMGALinkID(linkId);
        mga->setMGAAudioMetadataIndex(info.index);
        mga->setMGAAudioMetadataIdentifier(info.identifier);

        // ST 2127-10:2022 , 10.2
        if (info.identifier == 0x02) {
            mxfUL sadmMetadataPayloadUL = {
                0x06,0x0e,0x2b,0x34,
                0x04,0x01,0x01,0x0d,
                0x04,0x04,0x02,0x12,
                0x00,0x00,0x00,0x00
            };
            std::vector<mxfUL> payloadULArray = { sadmMetadataPayloadUL };        
            mga->setMGAAudioMetadataPayloadULArray(payloadULArray);
        }

        SADMAudioMetadataSubDescriptor *sadm = new SADMAudioMetadataSubDescriptor(hm);

        sadm->setSADMMetadataSectionLinkID(linkId);
        // TODO: add profile levels

        MGASoundfieldGroupLabelSubDescriptor *sfgl = new MGASoundfieldGroupLabelSubDescriptor(hm);
        // MGA related stuff
        sfgl->setMGAMetadataSectionLinkID(linkId);
        // generic soundfield stuff (ST2127-1:202 - Table 12)
        mxfUL dictId = {
            0x06,0x0e,0x2b,0x34,
            0x04,0x01,0x01,0x0d,
            0x03,0x02,0x02,0x22,
            0x00,0x00,0x00,0x00
        };
        sfgl->setMCALabelDictionaryID(dictId);
        sfgl->setMCALinkID(generate_uuid());
        sfgl->setMCATagSymbol("MGASf");
        sfgl->setMCATagName("MGA Soundfield");

        mFileDescriptor->appendSubDescriptors(mga);
        mMGAAudioMetadataSubDescriptors.push_back(mga);
        mFileDescriptor->appendSubDescriptors(sadm);
        mSADMAudioMetadataSubDescriptors.push_back(sadm);
        mFileDescriptor->appendSubDescriptors(sfgl);
        mMGASoundfieldGroupLabelSubDescriptors.push_back(sfgl);
    }
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
