/*
 * Copyright (C) 2024, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <bmx/mxf_helper/TimedEventsMXFDescriptorHelper.h>

#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceType TimedEventsMXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(TimedEvents)))
        return UNKNOWN_ESSENCE_TYPE;

    TimedEventsTextDescriptor *te_descriptor = dynamic_cast<TimedEventsTextDescriptor*>(file_descriptor);
    if (!te_descriptor)
        return UNKNOWN_ESSENCE_TYPE;

    return TIMED_EVENTS;
}

bool TimedEventsMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == TIMED_EVENTS;
}

TimedEventsManifest* TimedEventsMXFDescriptorHelper::CreateManifest(FileDescriptor *file_descriptor)
{
    TimedEventsTextDescriptor *te_descriptor = dynamic_cast<TimedEventsTextDescriptor*>(file_descriptor);
    BMX_ASSERT(te_descriptor);

    TimedEventsManifest *manifest = new TimedEventsManifest();
    try
    {
        manifest->mMIMEType = te_descriptor->getTextMIMEType();
        manifest->mCharacterEncoding = te_descriptor->getTextCharacterEncoding();
        manifest->mEventSchemes = te_descriptor->getEventSchemeURIsAsVector();

        if (te_descriptor->haveSubDescriptors()) {
            vector<SubDescriptor*> sub_descriptors = te_descriptor->getSubDescriptors();
            for (size_t i = 0; i < sub_descriptors.size(); i++) {
                TimedEventsAncillaryResourceSubDescriptor *anc_subdescriptor =
                    dynamic_cast<TimedEventsAncillaryResourceSubDescriptor*>(sub_descriptors[i]);
                if (anc_subdescriptor) {
                    TimedEventsAncillaryResource *anc_resource = new TimedEventsAncillaryResource();
                    anc_resource->resource_id = anc_subdescriptor->getTimedEventsAncillaryResourceID();
                    anc_resource->mime_type   = anc_subdescriptor->getTimedEventsAncillaryResourceMIMEType();
                    anc_resource->stream_id   = anc_subdescriptor->getTimedEventsAncillaryResourceStreamID();
                    manifest->mAncillaryResources.push_back(anc_resource);
                }
            }
        }

        return manifest;
    }
    catch (...)
    {
        delete manifest;
        throw;
    }
}

TimedEventsMXFDescriptorHelper::TimedEventsMXFDescriptorHelper()
: TimedDataMXFDescriptorHelper()
{
    mEssenceType = TIMED_EVENTS;
}

TimedEventsMXFDescriptorHelper::~TimedEventsMXFDescriptorHelper()
{
}

void TimedEventsMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label)
{
    MXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mEssenceType = TIMED_EVENTS;
}

FileDescriptor* TimedEventsMXFDescriptorHelper::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    TimedEventsManifest *te_manifest = dynamic_cast<TimedEventsManifest*>(mManifest);
    BMX_ASSERT(te_manifest);

    mFileDescriptor = new TimedEventsTextDescriptor(header_metadata);

    size_t i;
    for (i = 0; i < te_manifest->GetAncillaryResources().size(); i++) {
        TimedEventsAncillaryResourceSubDescriptor *anc_subdescriptor = new TimedEventsAncillaryResourceSubDescriptor(header_metadata);
        TimedEventsAncillaryResource *anc_resource = dynamic_cast<TimedEventsAncillaryResource*>(te_manifest->GetAncillaryResources()[i]);
        anc_subdescriptor->setTimedEventsAncillaryResourceID(anc_resource->resource_id);
        anc_subdescriptor->setTimedEventsAncillaryResourceMIMEType(anc_resource->mime_type);
        anc_subdescriptor->setTimedEventsAncillaryResourceStreamID(anc_resource->stream_id);
        mFileDescriptor->appendSubDescriptors(anc_subdescriptor);
    }

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void TimedEventsMXFDescriptorHelper::UpdateFileDescriptor()
{
    TimedEventsManifest *te_manifest = dynamic_cast<TimedEventsManifest*>(mManifest);
    BMX_ASSERT(te_manifest);

    DataMXFDescriptorHelper::UpdateFileDescriptor();

    TimedEventsTextDescriptor *te_descriptor = dynamic_cast<TimedEventsTextDescriptor*>(mFileDescriptor);
    BMX_ASSERT(te_descriptor);

    te_descriptor->setTextMIMEType(te_manifest->GetMIMEType());
    te_descriptor->setTextCharacterEncoding(te_manifest->GetCharacterEncoding());
    te_descriptor->setEventSchemeURIs(te_manifest->GetEventSchemes());
}

mxfUL TimedEventsMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return MXF_EC_L(TimedEvents);
}
