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

#ifndef MXFPP_HEVCSUBDESCRIPTOR_BASE_H_
#define MXFPP_HEVCSUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SubDescriptor.h>


namespace mxfpp
{


class HEVCSubDescriptorBase : public SubDescriptor
{
public:
    friend class MetadataSetFactory<HEVCSubDescriptorBase>;
    static const mxfKey setKey;

public:
    HEVCSubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~HEVCSubDescriptorBase();


   // getters

   uint8_t getDecodingDelay() const;
   bool haveConstantBPictureFlag() const;
   bool getConstantBPictureFlag() const;
   bool haveCodedContentKind() const;
   uint8_t getCodedContentKind() const;
   bool haveClosedGOPIndicator() const;
   bool getClosedGOPIndicator() const;
   bool haveIdenticalGOPIndicator() const;
   bool getIdenticalGOPIndicator() const;
   bool haveMaximumGOPSize() const;
   uint16_t getMaximumGOPSize() const;
   bool haveMaximumBPictureCount() const;
   uint16_t getMaximumBPictureCount() const;
   bool haveMaximumBitrate() const;
   uint32_t getMaximumBitrate() const;
   bool haveAverageBitrate() const;
   uint32_t getAverageBitrate() const;
   bool haveProfile() const;
   uint8_t getProfile() const;
   bool haveTier() const;
   uint8_t getTier() const;
   bool haveProfileConstraint() const;
   uint16_t getProfileConstraint() const;
   bool haveLevel() const;
   uint8_t getLevel() const;
   bool haveMaximumRefFrames() const;
   uint8_t getMaximumRefFrames() const;
   bool haveSequenceParameterSetFlag() const;
   uint8_t getSequenceParameterSetFlag() const;
   bool havePictureParameterSetFlag() const;
   uint8_t getPictureParameterSetFlag() const;
   bool haveVideoParameterSetFlag() const;
   uint8_t getVideoParameterSetFlag() const;


   // setters

   void setDecodingDelay(uint8_t value);
   void setConstantBPictureFlag(bool value);
   void setCodedContentKind(uint8_t value);
   void setClosedGOPIndicator(bool value);
   void setIdenticalGOPIndicator(bool value);
   void setMaximumGOPSize(uint16_t value);
   void setMaximumBPictureCount(uint16_t value);
   void setMaximumBitrate(uint32_t value);
   void setAverageBitrate(uint32_t value);
   void setProfile(uint8_t value);
   void setTier(uint8_t value);
   void setProfileConstraint(uint16_t value);
   void setLevel(uint8_t value);
   void setMaximumRefFrames(uint8_t value);
   void setSequenceParameterSetFlag(uint8_t value);
   void setPictureParameterSetFlag(uint8_t value);
   void setVideoParameterSetFlag(uint8_t value);
   


protected:
    HEVCSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
