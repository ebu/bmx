/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#include <memory>

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey HEVCSubDescriptorBase::setKey = MXF_SET_K(HEVCSubDescriptor);


HEVCSubDescriptorBase::HEVCSubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

HEVCSubDescriptorBase::HEVCSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

HEVCSubDescriptorBase::~HEVCSubDescriptorBase()
{}


uint8_t HEVCSubDescriptorBase::getDecodingDelay() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCDecodingDelay));
}

bool HEVCSubDescriptorBase::haveConstantBPictureFlag() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCConstantBPictureFlag));
}

bool HEVCSubDescriptorBase::getConstantBPictureFlag() const
{
    return getBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCConstantBPictureFlag));
}

bool HEVCSubDescriptorBase::haveCodedContentKind() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCCodedContentKind));
}

uint8_t HEVCSubDescriptorBase::getCodedContentKind() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCCodedContentKind));
}

bool HEVCSubDescriptorBase::haveClosedGOPIndicator() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCClosedGOPIndicator));
}

bool HEVCSubDescriptorBase::getClosedGOPIndicator() const
{
    return getBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCClosedGOPIndicator));
}

bool HEVCSubDescriptorBase::haveIdenticalGOPIndicator() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCIdenticalGOPIndicator));
}

bool HEVCSubDescriptorBase::getIdenticalGOPIndicator() const
{
    return getBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCIdenticalGOPIndicator));
}

bool HEVCSubDescriptorBase::haveMaximumGOPSize() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumGOPSize));
}

uint16_t HEVCSubDescriptorBase::getMaximumGOPSize() const
{
    return getUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumGOPSize));
}

bool HEVCSubDescriptorBase::haveMaximumBPictureCount() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBPictureCount));
}

uint16_t HEVCSubDescriptorBase::getMaximumBPictureCount() const
{
    return getUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBPictureCount));
}

bool HEVCSubDescriptorBase::haveMaximumBitrate() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBitrate));
}

uint32_t HEVCSubDescriptorBase::getMaximumBitrate() const
{
    return getUInt32Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBitrate));
}

bool HEVCSubDescriptorBase::haveAverageBitrate() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCAverageBitrate));
}

uint32_t HEVCSubDescriptorBase::getAverageBitrate() const
{
    return getUInt32Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCAverageBitrate));
}

bool HEVCSubDescriptorBase::haveProfile() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfile));
}

uint8_t HEVCSubDescriptorBase::getProfile() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfile));
}

bool HEVCSubDescriptorBase::haveProfileConstraint() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfileConstraint));
}

uint16_t HEVCSubDescriptorBase::getProfileConstraint() const
{
    return getUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfileConstraint));
}

bool HEVCSubDescriptorBase::haveLevel() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCLevel));
}

uint8_t HEVCSubDescriptorBase::getLevel() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCLevel));
}

bool HEVCSubDescriptorBase::haveTier() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCTier));
}

uint8_t HEVCSubDescriptorBase::getTier() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCTier));
}

bool HEVCSubDescriptorBase::haveMaximumRefFrames() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumRefFrames));
}

uint8_t HEVCSubDescriptorBase::getMaximumRefFrames() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumRefFrames));
}

bool HEVCSubDescriptorBase::haveSequenceParameterSetFlag() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCSequenceParameterSetFlag));
}

uint8_t HEVCSubDescriptorBase::getSequenceParameterSetFlag() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCSequenceParameterSetFlag));
}

bool HEVCSubDescriptorBase::havePictureParameterSetFlag() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCPictureParameterSetFlag));
}

uint8_t HEVCSubDescriptorBase::getPictureParameterSetFlag() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCPictureParameterSetFlag));
}

bool HEVCSubDescriptorBase::haveVideoParameterSetFlag() const
{
    return haveItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCVideoParameterSetFlag));
}

uint8_t HEVCSubDescriptorBase::getVideoParameterSetFlag() const
{
    return getUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCVideoParameterSetFlag));
}

void HEVCSubDescriptorBase::setDecodingDelay(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCDecodingDelay), value);
}

void HEVCSubDescriptorBase::setConstantBPictureFlag(bool value)
{
    setBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCConstantBPictureFlag), value);
}

void HEVCSubDescriptorBase::setCodedContentKind(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCCodedContentKind), value);
}

void HEVCSubDescriptorBase::setClosedGOPIndicator(bool value)
{
    setBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCClosedGOPIndicator), value);
}

void HEVCSubDescriptorBase::setIdenticalGOPIndicator(bool value)
{
    setBooleanItem(&MXF_ITEM_K(HEVCSubDescriptor, HEVCIdenticalGOPIndicator), value);
}

void HEVCSubDescriptorBase::setMaximumGOPSize(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumGOPSize), value);
}

void HEVCSubDescriptorBase::setMaximumBPictureCount(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBPictureCount), value);
}

void HEVCSubDescriptorBase::setMaximumBitrate(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumBitrate), value);
}

void HEVCSubDescriptorBase::setAverageBitrate(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCAverageBitrate), value);
}

void HEVCSubDescriptorBase::setProfile(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfile), value);
}

void HEVCSubDescriptorBase::setProfileConstraint(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCProfileConstraint), value);
}

void HEVCSubDescriptorBase::setLevel(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCLevel), value);
}

void HEVCSubDescriptorBase::setTier(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCTier), value);
}

void HEVCSubDescriptorBase::setMaximumRefFrames(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCMaximumRefFrames), value);
}

void HEVCSubDescriptorBase::setSequenceParameterSetFlag(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCSequenceParameterSetFlag), value);
}

void HEVCSubDescriptorBase::setPictureParameterSetFlag(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCPictureParameterSetFlag), value);
}

void HEVCSubDescriptorBase::setVideoParameterSetFlag(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(HEVCSubDescriptor, HEVCVideoParameterSetFlag), value);
}

