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

#ifndef BMX_HEVC_ESSENCE_PARSER_H_
#define BMX_HEVC_ESSENCE_PARSER_H_

#include <map>
#include <vector>
#include <optional>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/EssenceType.h>


namespace bmx
{

enum HEVCVideoFormat {
    HEVC_VF_COMPONENT  = 0,
    HEVC_VF_PAL = 1,
    HEVC_VF_NTSC = 2,
    HEVC_VF_SECAM = 3,
    HEVC_VF_MAC = 4,
    HEVC_VF_UNSPECIFIED = 5,
};

#define HEVC_ASPECT_RATIO_IDC_EXTENDED_SAR 255

class HEVCSeqParameterSet {

public:
    HEVCSeqParameterSet() 
        : 
        chroma_format_idc(0),
        pic_width_in_luma_samples(0),
        pic_height_in_luma_samples(0),
        bit_depth_luma_minus8(0),
        have_aspect_ratio_info(false),
        aspect_ratio_idc(0),
        sar_width(0),
        sar_height(0),
        have_video_format(false),
        video_format(5),
        have_color_description(false),
        color_primaries(2),
        transfer_characteristics(2),
        matrix_coeffs(2),

        general_profile_idc(0),
        profile_constraint(0),
        general_profile_space(0),
        general_level_idc(0),
        general_tier_flag(0),
        general_progressive_source_flag(0),
        general_interlaced_source_flag(0),
        general_non_packed_constraint_flag(0),
        general_frame_only_constraint_flag(0),

        have_extended_contraints(false),
        general_max_14bit_constraint_flag(0),
        general_max_12bit_constraint_flag(0),
        general_max_10bit_constraint_flag(0),
        general_max_8bit_constraint_flag(0),
        general_max_422chroma_constraint_flag(0),
        general_max_420chroma_constraint_flag(0),
        general_max_monochrome_constraint_flag(0),
        general_intra_constraint_flag(0),
        general_one_picture_only_constraint_flag(0),
        general_lower_bit_rate_constraint_flag(0),

        bit_rate_scale(0),
        have_bit_rate_value_minus1(false),
        bit_rate_value_minus1(0)
    {

    };

    uint8_t chroma_format_idc;
    uint32_t pic_width_in_luma_samples;
    uint32_t pic_height_in_luma_samples;
    uint32_t bit_depth_luma_minus8;

    bool have_aspect_ratio_info;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;

    bool have_video_format;
    uint8_t video_format;

    bool have_color_description;
    uint8_t color_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coeffs;

    uint8_t general_profile_idc;
    // todo
    uint16_t profile_constraint;

    uint8_t general_profile_space;
    uint8_t general_level_idc;
    uint8_t general_tier_flag;

    // can be used for HEVC Coded Content Flag
    uint8_t general_progressive_source_flag;
    uint8_t general_interlaced_source_flag;
    uint8_t general_non_packed_constraint_flag;
    uint8_t general_frame_only_constraint_flag;

    bool have_extended_contraints;
    uint8_t general_max_14bit_constraint_flag;
    uint8_t general_max_12bit_constraint_flag;
    uint8_t general_max_10bit_constraint_flag;
    uint8_t general_max_8bit_constraint_flag;
    uint8_t general_max_422chroma_constraint_flag;
    uint8_t general_max_420chroma_constraint_flag;
    uint8_t general_max_monochrome_constraint_flag;
    uint8_t general_intra_constraint_flag;
    uint8_t general_one_picture_only_constraint_flag;
    uint8_t general_lower_bit_rate_constraint_flag;

    uint8_t bit_rate_scale;
    bool have_bit_rate_value_minus1;
    uint32_t bit_rate_value_minus1;
    
};

class AVCGetBitBuffer;
class HEVCEssenceParser : public EssenceParser
{
public:
    HEVCEssenceParser();
    
    virtual ~HEVCEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);
    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

    EssenceType GetEssenceType() const;

     bool HaveFrameRate() const            { return false; }
     Rational GetFrameRate() const         { return {0, 0}; }
    
    const HEVCSeqParameterSet& GetSeqParameterSet() const { return mSeqParameterSet; }

private:
    void ResetFrameInfo();
    uint32_t NextStartCodePrefix(const unsigned char* data, uint32_t size);
    void ParseSeqParameterSet(AVCGetBitBuffer *bs);
    void ParseVUIParameters(AVCGetBitBuffer *bs, uint8_t max_sub_layers_minus1);
    void ParseHRDParameters(AVCGetBitBuffer *bs, uint8_t common_inf_present_flag, uint8_t max_sub_layers_minus1);
    void ParseSubLayerHRDParameters(AVCGetBitBuffer *bs, uint8_t sub_pic_hrd_params_present_flag, uint8_t cpb_cnt);
    void ParseProfileTierLevel(AVCGetBitBuffer *bs, uint8_t max_sub_layers_minus1);
    void ParseScalingListData(AVCGetBitBuffer *bs);
    void ParseShortTermRefPicSet(AVCGetBitBuffer *bs, uint8_t stRpsIdx, uint8_t num_short_term_ref_pic_sets);
    void CheckConformance();
    EssenceType CalculateEssenceTypeFromConstraints() const;

private:
    // offset in CURRENT frame
    uint32_t mOffset;
    uint64_t mFrameNum;

    uint32_t mNalUnitType;
    uint32_t mLastNalUnitType;

    // frame data
    HEVCSeqParameterSet mSeqParameterSet;
};


};



#endif

