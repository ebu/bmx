/*
 * Copyright (C) 2012, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
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

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey ISXDDescriptorBase::setKey = MXF_SET_K(ISXDDescriptor);


ISXDDescriptorBase::ISXDDescriptorBase(HeaderMetadata *headerMetadata)
: GenericDataEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

ISXDDescriptorBase::ISXDDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericDataEssenceDescriptor(headerMetadata, cMetadataSet)
{}

ISXDDescriptorBase::~ISXDDescriptorBase()
{}


// getters
bool ISXDDescriptorBase::haveNamespaceURI() const
{
    return haveItem(&MXF_ITEM_K(ISXDDescriptor, NamespaceURI));
}

std::string ISXDDescriptorBase::getNamespaceURI() const
{
    return getUTF8StringItem(&MXF_ITEM_K(ISXDDescriptor, NamespaceURI));
}

bool ISXDDescriptorBase::haveRootRelementKey() const
{
    return haveItem(&MXF_ITEM_K(ISXDDescriptor, RootElementKey));
}

std::string ISXDDescriptorBase::getRootElementKey() const
{
    return getStringItem(&MXF_ITEM_K(ISXDDescriptor, RootElementKey));
}

// setters
void ISXDDescriptorBase::setNamespaceURI(std::string value)
{
    setUTF8StringItem(&MXF_ITEM_K(ISXDDescriptor, NamespaceURI), value);
}

void ISXDDescriptorBase::setRootElementKey(std::string value)
{
    setStringItem(&MXF_ITEM_K(ISXDDescriptor, RootElementKey), value);
}
