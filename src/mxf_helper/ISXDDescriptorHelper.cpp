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

#include <bmx/mxf_helper/ISXDDescriptorHelper.h>
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
    {MXF_EC_L(ISXDFrameWrapped),     ISXD,     true}
};



EssenceType ISXDDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool ISXDDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

ISXDDescriptorHelper::ISXDDescriptorHelper()
: DataMXFDescriptorHelper()
{
    mEssenceType = IAB;
}

ISXDDescriptorHelper::~ISXDDescriptorHelper()
{
}

void ISXDDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                         mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    DataMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

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

FileDescriptor* ISXDDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new ISXDDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

uint32_t ISXDDescriptorHelper::GetSampleSize()
{
    return 0;
}

void ISXDDescriptorHelper::UpdateFileDescriptor()
{
    DataMXFDescriptorHelper::UpdateFileDescriptor();

    ISXDDescriptor *descriptor = dynamic_cast<ISXDDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    // UL_DEFINITION ESSENCE CODING
    mxfUL essenceCoding = {0x06,0x0E,0x2B,0x34,0x04,0x01,0x01,0x05,0x0E,0x09,0x06,0x06,0x00,0x00,0x00,0x00};

    descriptor->setDataEssenceCoding(essenceCoding);
}

void ISXDDescriptorHelper::UpdateFileDescriptor(mxfpp::FileDescriptor *file_desc_in)
{
    ISXDDescriptor *isxd_descriptor = dynamic_cast<ISXDDescriptor*>(mFileDescriptor);
    BMX_ASSERT(isxd_descriptor);

    ISXDDescriptor *isxd_desc_in = dynamic_cast<ISXDDescriptor*>(file_desc_in);
    BMX_CHECK(isxd_desc_in);
}

void ISXDDescriptorHelper::UpdateFileDescriptor(ISXDEssenceParser *essence_parser)
{
    (void)essence_parser;
    ISXDDescriptor *descriptor = dynamic_cast<ISXDDescriptor*>(mFileDescriptor);
    BMX_ASSERT(descriptor);

    string namespace_uri = essence_parser->GetNamespaceURI();
    string root_element_key = essence_parser->GetRootElementKey();

    descriptor->setNamespaceURI(namespace_uri);
    
    // removed because of ST2067-202.
    //descriptor->setRootElementKey(root_element_key);
}

mxfUL ISXDDescriptorHelper::ChooseEssenceContainerUL() const
{
    BMX_ASSERT(mFrameWrapped);
    mxfUL ul = MXF_EC_L(ISXDFrameWrapped);
    return ul;
}
