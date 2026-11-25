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

#include <memory>
#include <regex>

#include <libMXF++/MXF.h>
#include <libMXF++/Utils.h>


using namespace std;
using namespace mxfpp;


const mxfKey TimedEventsTextDescriptorBase::setKey = MXF_SET_K(TimedEventsTextDescriptor);


TimedEventsTextDescriptorBase::TimedEventsTextDescriptorBase(HeaderMetadata *headerMetadata)
: GenericDataEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

TimedEventsTextDescriptorBase::TimedEventsTextDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericDataEssenceDescriptor(headerMetadata, cMetadataSet)
{}

TimedEventsTextDescriptorBase::~TimedEventsTextDescriptorBase()
{}

string TimedEventsTextDescriptorBase::getTextCharacterEncoding() const
{
    return getStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, TextCharacterEncoding));
}

string TimedEventsTextDescriptorBase::getTextMIMEType() const
{
    return getStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, TextMIMEType));
}

string TimedEventsTextDescriptorBase::getEventSchemeURIs() const
{
    return getStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, EventSchemeURIs));
}

vector<string> TimedEventsTextDescriptorBase::getEventSchemeURIsAsVector() const
{
    return split_string(getEventSchemeURIs(), ' ', false, true);
}

void TimedEventsTextDescriptorBase::setTextCharacterEncoding(const string &value)
{
    setStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, TextCharacterEncoding), value);
}

void TimedEventsTextDescriptorBase::setTextMIMEType(const string &value)
{
    setStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, TextMIMEType), value);
}

void TimedEventsTextDescriptorBase::setEventSchemeURIs(const string &value)
{
    setStringItem(&MXF_ITEM_K(TimedEventsTextDescriptor, EventSchemeURIs), value);
}

void TimedEventsTextDescriptorBase::setEventSchemeURIs(const vector<string> &value)
{
    // Concatenate values into a string using a space as a separator.
    // Note: SMPTE ST 2067-206 states: "An Event Scheme URI shall not include the space character"

    string str_value;
    for (size_t i = 0; i < value.size(); i++) {
        if (i > 0)
            str_value += " ";
        str_value += value[i];
    }

    setEventSchemeURIs(str_value);
}
