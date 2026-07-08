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

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cmath>
#include <set>

#include <bmx/essence_parser/AVCEssenceParser.h> // for AVCGetBitBuffer
#include <bmx/essence_parser/HEVCEssenceParser.h>
#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
#include <bmx/BitBuffer.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace bmx;

void print_buf(const unsigned char *data, uint32_t data_size, uint32_t n) {
    if (n < 20) {
        n = 20;
    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    if (data_size < n) {
        for (uint32_t i = 0; i < n; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i]) << ".";
        }
    } else {
        for (uint32_t i = 0; i < n/2; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i])  << ".";
        }
        ss << ".........";
        for (uint32_t i = data_size - n/2; i < data_size; ++i) {
            ss << std::setw(2) << static_cast<unsigned>(data[i]);
            if (i < data_size - 1) {
                 ss << ".";
            }
        }
    }
    log_info("%s\n", ss.str().c_str());
}


enum NAL_TYPE {
    NAL_UNKNOWN         = -1,
    // Coded slice segment of an IDR picture
    IDR_W_RADL          = 19,       
    IDR_N_LP            = 20,         // DR slice with no leading pictures

    // Video parameter set
    VPS_NUT             = 32,
    // Sequence parameter set
    SPS_NUT             = 33,
    // Picture parameter set
    PPS_NUT             = 34,

    // Access unit delimiter
    AUD_NUT             = 35,

    // Supplemental enhancement information
    PREFIX_SEI_NUT      = 39
};


HEVCEssenceParser::HEVCEssenceParser()
{   
    mNalUnitType = NAL_TYPE::NAL_UNKNOWN;
    mLastNalUnitType = NAL_TYPE::NAL_UNKNOWN;
    mFrameNum = 0;
    ResetFrameInfo();
}

HEVCEssenceParser::~HEVCEssenceParser()
{
}

uint32_t HEVCEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // the access unit shall start with a zero_byte followed by start_code_prefix_one_3byte

    uint32_t offset = NextStartCodePrefix(data, data_size);
    if (offset == ESSENCE_PARSER_NULL_OFFSET)
        return ESSENCE_PARSER_NULL_OFFSET;

    if (offset == 0 || data[offset - 1] != 0x00) {
        log_warn("HEVC: Missing zero_byte before start_code_prefix_one_3byte at access unit start\n");
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    return offset - 1;
}

uint32_t HEVCEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{    
    // End of stream
    if (data_size == 0) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    uint32_t end_pos = ESSENCE_PARSER_NULL_OFFSET;
    bool have_frame_end = false;
    
    if (data_size == ESSENCE_PARSER_NULL_OFFSET) {
        end_pos = mOffset + 5; // add the 5 deducted from loop below to get to EOS

        have_frame_end = true;
    }

    // we want at least 5 bytes of space
    while (!have_frame_end && mOffset < data_size - 5) {
        if (data[mOffset] == 0x00 && data[mOffset+1] == 0x00 && data[mOffset+2] == 0x01) {

            // advance start code
            mOffset += 3;

            uint8_t b1 = data[mOffset];

            // advance nal header
            mOffset += 2;

            uint32_t nal_unit_type = (b1 & 0x7e) >> 1;
            mLastNalUnitType = mNalUnitType;
            mNalUnitType = nal_unit_type;

            if (mLastNalUnitType == NAL_TYPE::IDR_W_RADL
                || mLastNalUnitType == NAL_TYPE::IDR_N_LP)
            {
                end_pos = mOffset - 5; // back to before start code and header

                assert(data[end_pos] == 0x00 && data[end_pos+1] == 0x00 && data[end_pos+2] == 0x01);

                // check if we have 00.00.00.01 or 00.00.01

                have_frame_end = true;
                break;
            }
        } else {
            mOffset++;
        }
    }

    uint32_t frame_size = ESSENCE_PARSER_NULL_OFFSET; // need more data
    if (have_frame_end) {
        frame_size = end_pos;
        mFrameNum++;
        // done. start again!
        mOffset = 0;
    }

    return frame_size;
}

void HEVCEssenceParser::ParseSeqParameterSet(AVCGetBitBuffer *bs)
{
    uint8_t sps_video_parameter_set_id = 0;
    bs->GetU(4, &sps_video_parameter_set_id);
    
    uint8_t sps_max_sub_layers_minus1 = 0;
    bs->GetU(3, &sps_max_sub_layers_minus1);
    
    uint8_t sps_temporal_id_nesting_flag = 0;
    bs->GetU(1, &sps_temporal_id_nesting_flag);

    ParseProfileTierLevel(bs, sps_max_sub_layers_minus1);

    uint8_t sps_seq_parameter_set_id = 0;
    bs->GetUE(&sps_seq_parameter_set_id, 31);

    uint8_t chroma_format_idc = 0;
    bs->GetUE(&chroma_format_idc, 3);
    mSeqParameterSet.chroma_format_idc = chroma_format_idc;


    // mChromaFormat
    uint8_t separate_colour_plane_flag = 0;
    // chroma sampling, 0 to 3 incl.
    if (chroma_format_idc == 3) {
        bs->GetU(1, &separate_colour_plane_flag);
    }

    uint64_t pic_width_in_luma_samples = 0;
    bs->GetUE(&pic_width_in_luma_samples);
    mSeqParameterSet.pic_width_in_luma_samples = (uint32_t)pic_width_in_luma_samples;

    uint64_t pic_height_in_luma_samples = 0;
    bs->GetUE(&pic_height_in_luma_samples);
    mSeqParameterSet.pic_height_in_luma_samples = (uint32_t)pic_height_in_luma_samples;

    uint8_t conformance_window_flag = 0;
    bs->GetU(1, &conformance_window_flag);
    if (conformance_window_flag) {
        uint64_t conf_win_left_offset = 0;
        bs->GetUE(&conf_win_left_offset);
        uint64_t conf_win_right_offset = 0;
        bs->GetUE(&conf_win_right_offset);
        uint64_t conf_win_top_offset = 0;
        bs->GetUE(&conf_win_top_offset);
        uint64_t conf_win_bottom_offset = 0;
        bs->GetUE(&conf_win_bottom_offset);
    }

    // mComponentDepth = bit_depth_luma_minus8 + 8;
    uint64_t bit_depth_luma_minus8 = 0;
    bs->GetUE(&bit_depth_luma_minus8);
    mSeqParameterSet.bit_depth_luma_minus8 = bit_depth_luma_minus8;

    uint64_t bit_depth_chroma_minus8 = 0;
    bs->GetUE(&bit_depth_chroma_minus8);

    uint64_t log2_max_pic_order_cnt_lsb_minus4 = 0;
    bs->GetUE(&log2_max_pic_order_cnt_lsb_minus4);

    uint8_t sps_sub_layer_ordering_info_present_flag = 0;
    bs->GetU(1, &sps_sub_layer_ordering_info_present_flag);
    uint32_t start = sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1;
    for (uint8_t i = start; i <= sps_max_sub_layers_minus1; ++i) {
        uint64_t sps_max_dec_pic_buffering_minus1 = 0;
        bs->GetUE(&sps_max_dec_pic_buffering_minus1);
        uint64_t sps_max_num_reorder_pics = 0;
        bs->GetUE(&sps_max_num_reorder_pics);
        uint64_t sps_max_latency_increase_plus1 = 0;
        bs->GetUE(&sps_max_latency_increase_plus1);
    }

    uint64_t log2_min_luma_coding_block_size_minus3 = 0;
    bs->GetUE(&log2_min_luma_coding_block_size_minus3);
    uint64_t log2_diff_max_min_luma_coding_block_size = 0;
    bs->GetUE(&log2_diff_max_min_luma_coding_block_size);
    uint64_t log2_min_transform_block_size_minus2 = 0;
    bs->GetUE(&log2_min_transform_block_size_minus2);
    uint64_t log2_diff_max_min_transform_block_size = 0;
    bs->GetUE(&log2_diff_max_min_transform_block_size);

    uint64_t max_transform_hierarchy_depth_inter = 0;
    bs->GetUE(&max_transform_hierarchy_depth_inter);
    uint64_t max_transform_hierarchy_depth_intra = 0;
    bs->GetUE(&max_transform_hierarchy_depth_intra);

    uint8_t scaling_list_enabled_flag = 0;
    bs->GetU(1, &scaling_list_enabled_flag);
    if (scaling_list_enabled_flag) {
        
        uint8_t sps_scaling_list_data_present_flag = 0;
        bs->GetU(1, &sps_scaling_list_data_present_flag);
        if (sps_scaling_list_data_present_flag) {
            ParseScalingListData(bs);  // You will need to define this function
        }
    }

    uint8_t amp_enabled_flag = 0;
    bs->GetU(1, &amp_enabled_flag);

    uint8_t sample_adaptive_offset_enabled_flag = 0;
    bs->GetU(1, &sample_adaptive_offset_enabled_flag);

    uint8_t pcm_enabled_flag = 0;
    bs->GetU(1, &pcm_enabled_flag);
    if (pcm_enabled_flag) {
        uint64_t pcm_sample_bit_depth_luma_minus1 = 0;
        bs->GetUE(&pcm_sample_bit_depth_luma_minus1);
        uint64_t pcm_sample_bit_depth_chroma_minus1 = 0;
        bs->GetUE(&pcm_sample_bit_depth_chroma_minus1);

        uint64_t log2_min_pcm_luma_coding_block_size_minus3 = 0;
        bs->GetUE(&log2_min_pcm_luma_coding_block_size_minus3);
        uint64_t log2_diff_max_min_pcm_luma_coding_block_size = 0;
        bs->GetUE(&log2_diff_max_min_pcm_luma_coding_block_size);

        uint8_t pcm_loop_filter_disabled_flag = 0;
        bs->GetU(1, &pcm_loop_filter_disabled_flag);
    }

    uint64_t num_short_term_ref_pic_sets = 0;
    bs->GetUE(&num_short_term_ref_pic_sets);
    for (uint64_t i = 0; i < num_short_term_ref_pic_sets; ++i) {
        ParseShortTermRefPicSet(bs, i, num_short_term_ref_pic_sets);
    }

    uint8_t long_term_ref_pics_present_flag = 0;
    bs->GetU(1, &long_term_ref_pics_present_flag);
    if (long_term_ref_pics_present_flag) {
        uint64_t num_long_term_ref_pics_sps = 0;
        bs->GetUE(&num_long_term_ref_pics_sps);
        for (uint64_t i = 0; i < num_long_term_ref_pics_sps; ++i) {
            // The number of bits used to represent lt_ref_pic_poc_lsb_sps[ i ] is equal to log2_max_pic_order_cnt_lsb_minus4 + 4.
            uint64_t bits = log2_max_pic_order_cnt_lsb_minus4 + 4;
            uint8_t lt_ref_pic_poc_lsb_sps = 0;   //u(v)
            bs->GetU(bits, &lt_ref_pic_poc_lsb_sps);
            uint8_t used_by_curr_pic_lt_sps_flag = 0;
            bs->GetU(1, &used_by_curr_pic_lt_sps_flag);
        }
    }

    uint8_t sps_temporal_mvp_enabled_flag = 0;
    bs->GetU(1, &sps_temporal_mvp_enabled_flag);

    uint8_t strong_intra_smoothing_enabled_flag = 0;
    bs->GetU(1, &strong_intra_smoothing_enabled_flag);

    uint8_t vui_parameters_present_flag = 0;
    bs->GetU(1, &vui_parameters_present_flag);
    if (vui_parameters_present_flag) {
        ParseVUIParameters(bs, sps_max_sub_layers_minus1);
    }

    uint8_t sps_extension_flag = 0;
    bs->GetU(1, &sps_extension_flag);
}

void HEVCEssenceParser::ParseScalingListData(AVCGetBitBuffer *bs)
{
    // we skip everything in this section. just need to make sure we parse it in order to advance the bitstream
    uint8_t scaling_list_pred_mode_flag;
    uint64_t scaling_list_pred_matrix_id_delta;
    int32_t scaling_list_dc_coef_minus8, scaling_list_delta_coef;

    for (int sizeId = 0; sizeId < 4; sizeId++) {
        int numMatrices = (sizeId == 3) ? 2 : 6;
        for (int matrixId = 0; matrixId < numMatrices; matrixId++) {
            bs->GetU(1, &scaling_list_pred_mode_flag);

            if (!scaling_list_pred_mode_flag) {
                bs->GetUE(&scaling_list_pred_matrix_id_delta);
            } else {
                int coefNum = std::min(64, 1 << (4 + (sizeId << 1)));
                if (sizeId > 1) {
                    bs->GetSE(&scaling_list_dc_coef_minus8);
                }

                for (int i = 0; i < coefNum; i++) {
                    bs->GetSE(&scaling_list_delta_coef);
                }
            }
        }
    }
}

void HEVCEssenceParser::ParseShortTermRefPicSet(AVCGetBitBuffer* bs, uint8_t stRpsIdx, uint8_t num_short_term_ref_pic_sets) {
    // we skip everything in this section. just need to make sure we parse it in order to advance the bitstream
    uint8_t inter_ref_pic_set_prediction_flag = 0;
    if (stRpsIdx != 0) {
        bs->GetU(1, &inter_ref_pic_set_prediction_flag);
    }

    if (inter_ref_pic_set_prediction_flag) {
        if (stRpsIdx == num_short_term_ref_pic_sets) {
            uint64_t delta_idx_minus1;
            bs->GetUE(&delta_idx_minus1);
        }

        uint8_t delta_rps_sign;
        bs->GetU(1, &delta_rps_sign);
        uint64_t abs_delta_rps_minus1;
        bs->GetUE(&abs_delta_rps_minus1);

        for (uint8_t j = 0; j <= 0; j++) { // Assuming NumDeltaPocs is set externally
            uint8_t used_by_curr_pic_flag;
            bs->GetU(1, &used_by_curr_pic_flag);
            if (!used_by_curr_pic_flag) {
                uint8_t use_delta_flag;
                bs->GetU(1, &use_delta_flag);
            }
        }
    } else {
        uint64_t num_negative_pics;
        bs->GetUE(&num_negative_pics);
        uint64_t num_positive_pics;
        bs->GetUE(&num_positive_pics);

        for (uint8_t i = 0; i < num_negative_pics; i++) {
            uint64_t delta_poc_s0_minus1;
            bs->GetUE(&delta_poc_s0_minus1);
            uint8_t used_by_curr_pic_s0_flag;
            bs->GetU(1, &used_by_curr_pic_s0_flag);
        }

        for (uint8_t i = 0; i < num_positive_pics; i++) {
            uint64_t delta_poc_s1_minus1;
            bs->GetUE(&delta_poc_s1_minus1);
            uint8_t used_by_curr_pic_s1_flag;
            bs->GetU(1, &used_by_curr_pic_s1_flag);
        }
    }
}

void HEVCEssenceParser::ParseVUIParameters(AVCGetBitBuffer *bs, uint8_t max_sub_layers_minus1)
{
    uint8_t aspect_ratio_info_present_flag = 0;
    bs->GetU(1, &aspect_ratio_info_present_flag);
    if (aspect_ratio_info_present_flag) {
        mSeqParameterSet.have_aspect_ratio_info = true;
        uint8_t aspect_ratio_idc = 0;
        bs->GetU(8, &aspect_ratio_idc);
        mSeqParameterSet.aspect_ratio_idc = aspect_ratio_idc;

        if (aspect_ratio_idc == HEVC_ASPECT_RATIO_IDC_EXTENDED_SAR) {
            uint64_t sar_width = 0;
            uint64_t sar_height = 0;
            bs->GetU(16, &sar_width);
            bs->GetU(16, &sar_height);
            mSeqParameterSet.sar_width = sar_width;
            mSeqParameterSet.sar_height = sar_height;
        }
    }

    uint8_t overscan_info_present_flag = 0;
    bs->GetU(1, &overscan_info_present_flag);
    if (overscan_info_present_flag) {
        uint8_t overscan_appropriate_flag = 0;
        bs->GetU(1, &overscan_appropriate_flag);
    }

    uint8_t video_signal_type_present_flag = 0;
    bs->GetU(1, &video_signal_type_present_flag);
    if (video_signal_type_present_flag) {
        mSeqParameterSet.have_video_format = true;
        uint8_t video_format = 0;
        bs->GetU(3, &video_format);
        // log_info("video_format: %d\n", video_format);
        mSeqParameterSet.video_format = video_format;

        uint8_t video_full_range_flag = 0;
        bs->GetU(1, &video_full_range_flag);
        uint8_t colour_description_present_flag = 0;
        bs->GetU(1, &colour_description_present_flag);
        if (colour_description_present_flag) {
            mSeqParameterSet.have_color_description = true;
            uint8_t colour_primaries = 0;
            bs->GetU(8, &colour_primaries);
            mSeqParameterSet.color_primaries = colour_primaries;
            
            uint8_t transfer_characteristics = 0;
            bs->GetU(8, &transfer_characteristics);
            mSeqParameterSet.transfer_characteristics = transfer_characteristics;

            uint8_t matrix_coeffs = 0;
            bs->GetU(8, &matrix_coeffs);
            mSeqParameterSet.matrix_coeffs = matrix_coeffs;
        }
    }

    uint8_t chroma_loc_info_present_flag = 0;
    bs->GetU(1, &chroma_loc_info_present_flag);
    if (chroma_loc_info_present_flag) {
        uint64_t chroma_sample_loc_type_top_field = 0;
        bs->GetUE(&chroma_sample_loc_type_top_field);
        uint64_t chroma_sample_loc_type_bottom_field = 0;
        bs->GetUE(&chroma_sample_loc_type_bottom_field);
    }

    uint8_t neutral_chroma_indication_flag = 0;
    bs->GetU(1, &neutral_chroma_indication_flag);
    uint8_t field_seq_flag = 0;
    bs->GetU(1, &field_seq_flag);
    uint8_t frame_field_info_present_flag = 0;
    bs->GetU(1, &frame_field_info_present_flag);

    uint8_t default_display_window_flag = 0;
    bs->GetU(1, &default_display_window_flag);
    if (default_display_window_flag) {
        uint64_t def_disp_win_left_offset = 0;
        bs->GetUE(&def_disp_win_left_offset);
        uint64_t def_disp_win_right_offset = 0;
        bs->GetUE(&def_disp_win_right_offset);
        uint64_t def_disp_win_top_offset = 0;
        bs->GetUE(&def_disp_win_top_offset);
        uint64_t def_disp_win_bottom_offset = 0;
        bs->GetUE(&def_disp_win_bottom_offset);
    }

    uint8_t vui_timing_info_present_flag = 0;
    bs->GetU(1, &vui_timing_info_present_flag);
    if (vui_timing_info_present_flag) {
        uint64_t vui_num_units_in_tick = 0;
        uint64_t vui_time_scale = 0;
        bs->GetU(32, &vui_num_units_in_tick);
        bs->GetU(32, &vui_time_scale);
        uint8_t vui_poc_proportional_to_timing_flag = 0;
        bs->GetU(1, &vui_poc_proportional_to_timing_flag);
        if (vui_poc_proportional_to_timing_flag) {
            uint64_t vui_num_ticks_poc_diff_one_minus1 = 0;
            bs->GetUE(&vui_num_ticks_poc_diff_one_minus1);
        }
    }

    uint8_t vui_hrd_parameters_present_flag = 0;
    bs->GetU(1, &vui_hrd_parameters_present_flag);
    if (vui_hrd_parameters_present_flag) {
        ParseHRDParameters(bs, 1, max_sub_layers_minus1);
    }

    uint8_t bitstream_restriction_flag = 0;
    bs->GetU(1, &bitstream_restriction_flag);
    if (bitstream_restriction_flag) {
        uint8_t tiles_fixed_structure_flag = 0;
        bs->GetU(1, &tiles_fixed_structure_flag);
        uint8_t motion_vectors_over_pic_boundaries_flag = 0;
        bs->GetU(1, &motion_vectors_over_pic_boundaries_flag);
        uint8_t restricted_ref_pic_lists_flag = 0;
        bs->GetU(1, &restricted_ref_pic_lists_flag);
        uint64_t min_spatial_segmentation_idc = 0;
        bs->GetUE(&min_spatial_segmentation_idc);
        uint64_t max_bytes_per_pic_denom = 0;
        bs->GetUE(&max_bytes_per_pic_denom);
        uint64_t max_bits_per_min_cu_denom = 0;
        bs->GetUE(&max_bits_per_min_cu_denom);
        uint64_t log2_max_mv_length_horizontal = 0;
        bs->GetUE(&log2_max_mv_length_horizontal);
        uint64_t log2_max_mv_length_vertical = 0;
        bs->GetUE(&log2_max_mv_length_vertical);
    }
}

void HEVCEssenceParser::ParseHRDParameters(AVCGetBitBuffer *bs, uint8_t common_inf_present_flag, uint8_t max_sub_layers_minus1)
{
    uint8_t nal_hrd_parameters_present_flag = 0;
    uint8_t vcl_hrd_parameters_present_flag = 0;
    uint8_t sub_pic_hrd_params_present_flag = 0;

    if (common_inf_present_flag) {
        bs->GetU(1, &nal_hrd_parameters_present_flag);
        bs->GetU(1, &vcl_hrd_parameters_present_flag);

        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            bs->GetU(1, &sub_pic_hrd_params_present_flag);

            uint8_t tick_divisor_minus2;
            bs->GetU(8, &tick_divisor_minus2);

            uint8_t du_cpb_removal_delay_increment_length_minus1;
            bs->GetU(5, &du_cpb_removal_delay_increment_length_minus1);

            uint8_t sub_pic_cpb_params_in_pic_timing_sei_flag;
            bs->GetU(1, &sub_pic_cpb_params_in_pic_timing_sei_flag);

            uint8_t dpb_output_delay_du_length_minus1;
            bs->GetU(5, &dpb_output_delay_du_length_minus1);

            uint8_t bit_rate_scale;
            bs->GetU(4, &bit_rate_scale);

            uint8_t cpb_size_scale;
            bs->GetU(4, &cpb_size_scale);

            if (sub_pic_hrd_params_present_flag) {
                uint8_t cpb_size_du_scale;
                bs->GetU(4, &cpb_size_du_scale);
            }

            uint8_t initial_cpb_removal_delay_length_minus1;
            bs->GetU(5, &initial_cpb_removal_delay_length_minus1);

            uint8_t au_cpb_removal_delay_length_minus1;
            bs->GetU(5, &au_cpb_removal_delay_length_minus1);

            uint8_t dpb_output_delay_length_minus1;
            bs->GetU(5, &dpb_output_delay_length_minus1);
        }
    }

    for (int i = 0; i <= max_sub_layers_minus1; i++) {
        uint8_t fixed_pic_rate_general_flag;
        bs->GetU(1, &fixed_pic_rate_general_flag);

        if (fixed_pic_rate_general_flag) {
            uint8_t fixed_pic_rate_within_cvs_flag;
            bs->GetU(1, &fixed_pic_rate_within_cvs_flag);
            
            if (fixed_pic_rate_within_cvs_flag) {
                uint64_t elemental_duration_in_tc_minus1;
                bs->GetUE(&elemental_duration_in_tc_minus1);
            }
        } else {
            uint8_t low_delay_hrd_flag;
            bs->GetU(1, &low_delay_hrd_flag);
            
            if (!low_delay_hrd_flag) {
                uint64_t cpb_cnt_minus1;
                bs->GetUE(&cpb_cnt_minus1);
            }
        }

        if (nal_hrd_parameters_present_flag) {
            ParseSubLayerHRDParameters(bs, sub_pic_hrd_params_present_flag, max_sub_layers_minus1);
        }
        
        if (vcl_hrd_parameters_present_flag) {
            ParseSubLayerHRDParameters(bs, sub_pic_hrd_params_present_flag, max_sub_layers_minus1);
        }
    }
}

void HEVCEssenceParser::ParseSubLayerHRDParameters(AVCGetBitBuffer *bs, uint8_t sub_pic_hrd_params_present_flag, uint8_t cpb_cnt) {
    uint64_t bit_rate_value_minus1;
    uint64_t cpb_size_value_minus1;
    uint64_t cpb_size_du_value_minus1;
    uint64_t bit_rate_du_value_minus1;
    uint8_t cbr_flag;

    for (uint32_t i = 0; i <= cpb_cnt; i++) {
        bs->GetUE(&bit_rate_value_minus1);
        if (i == 0) {
            mSeqParameterSet.bit_rate_value_minus1 = bit_rate_value_minus1;
            mSeqParameterSet.have_bit_rate_value_minus1 = true;
        }

        bs->GetUE(&cpb_size_value_minus1);
        
        if (sub_pic_hrd_params_present_flag) {
            bs->GetUE(&cpb_size_du_value_minus1);
            bs->GetUE(&bit_rate_du_value_minus1);
        }

        bs->GetU(1, &cbr_flag);
    }
}

#define PARSE_HEVC_2 0

void HEVCEssenceParser::ParseProfileTierLevel(AVCGetBitBuffer *bs, uint8_t max_sub_layers_minus1)
{
    std::vector<uint8_t> general_profile_compatibility_flag(32, 0);

    uint8_t general_profile_space = 0;
    bs->GetU(2, &general_profile_space);
    mSeqParameterSet.general_profile_space = general_profile_space;

    uint8_t general_tier_flag = 0;
    bs->GetU(1, &general_tier_flag);
    mSeqParameterSet.general_tier_flag = general_tier_flag;

    uint8_t general_profile_idc = 0;
    bs->GetU(5, &general_profile_idc);
    mSeqParameterSet.general_profile_idc = general_profile_idc;

    for (int j = 0; j < 32; ++j) {
        bs->GetU(1, &general_profile_compatibility_flag[j]);
    }

    uint8_t general_progressive_source_flag = 0;
    bs->GetU(1, &general_progressive_source_flag);
    mSeqParameterSet.general_progressive_source_flag = general_progressive_source_flag;

    uint8_t general_interlaced_source_flag = 0;
    bs->GetU(1, &general_interlaced_source_flag);
    mSeqParameterSet.general_interlaced_source_flag = general_interlaced_source_flag;

    uint8_t general_non_packed_constraint_flag = 0;
    bs->GetU(1, &general_non_packed_constraint_flag);
    mSeqParameterSet.general_non_packed_constraint_flag = general_non_packed_constraint_flag;

    uint8_t general_frame_only_constraint_flag = 0;
    bs->GetU(1, &general_frame_only_constraint_flag);
    mSeqParameterSet.general_frame_only_constraint_flag = general_frame_only_constraint_flag;

#if PARSE_HEVC_2 == 0

    if (general_profile_idc == 4 || general_profile_compatibility_flag[4] ||
        general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
        general_profile_idc == 6 || general_profile_compatibility_flag[6] ||
        general_profile_idc == 7 || general_profile_compatibility_flag[7] ||
        general_profile_idc == 8 || general_profile_compatibility_flag[8] ||
        general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
        general_profile_idc == 10 || general_profile_compatibility_flag[10] ||
        general_profile_idc == 11 || general_profile_compatibility_flag[11])
    {
        // total bits 43
        mSeqParameterSet.have_extended_contraints = true;
        uint8_t general_max_12bit_constraint_flag  = 0;
        bs->GetU(1, &general_max_12bit_constraint_flag );
        mSeqParameterSet.general_max_12bit_constraint_flag  = general_max_12bit_constraint_flag ;

        uint8_t general_max_10bit_constraint_flag  = 0;
        bs->GetU(1, &general_max_10bit_constraint_flag );
        mSeqParameterSet.general_max_10bit_constraint_flag  = general_max_10bit_constraint_flag ;

        uint8_t general_max_8bit_constraint_flag  = 0;
        bs->GetU(1, &general_max_8bit_constraint_flag );
        mSeqParameterSet.general_max_8bit_constraint_flag  = general_max_8bit_constraint_flag ;

        uint8_t general_max_422chroma_constraint_flag  = 0;
        bs->GetU(1, &general_max_422chroma_constraint_flag );
        mSeqParameterSet.general_max_422chroma_constraint_flag  = general_max_422chroma_constraint_flag ;

        uint8_t general_max_420chroma_constraint_flag  = 0;
        bs->GetU(1, &general_max_420chroma_constraint_flag );
        mSeqParameterSet.general_max_420chroma_constraint_flag  = general_max_420chroma_constraint_flag ;

        uint8_t general_max_monochrome_constraint_flag  = 0;
        bs->GetU(1, &general_max_monochrome_constraint_flag );
        mSeqParameterSet.general_max_monochrome_constraint_flag  = general_max_monochrome_constraint_flag ;

        uint8_t general_intra_constraint_flag  = 0;
        bs->GetU(1, &general_intra_constraint_flag );
        mSeqParameterSet.general_intra_constraint_flag  = general_intra_constraint_flag ;

        uint8_t general_one_picture_only_constraint_flag  = 0;
        bs->GetU(1, &general_one_picture_only_constraint_flag );
        mSeqParameterSet.general_one_picture_only_constraint_flag  = general_one_picture_only_constraint_flag;

        uint8_t general_lower_bit_rate_constraint_flag  = 0;
        bs->GetU(1, &general_lower_bit_rate_constraint_flag );
        mSeqParameterSet.general_lower_bit_rate_constraint_flag  = general_lower_bit_rate_constraint_flag;

        if(general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
           general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
           general_profile_idc == 10 || general_profile_compatibility_flag[10] ||
           general_profile_idc == 11 || general_profile_compatibility_flag[11] )
        {
            uint8_t general_max_14bit_constraint_flag  = 0;
            bs->GetU(1, &general_max_14bit_constraint_flag );
            mSeqParameterSet.general_max_14bit_constraint_flag  = general_max_14bit_constraint_flag;
            bs->SkipBits(33);
        } else {
            bs->SkipBits(34);
        }        
    } else if( general_profile_idc == 2 || general_profile_compatibility_flag[2]) {
        // total bits 43
        bs->SkipBits(7);
        uint8_t general_one_picture_only_constraint_flag  = 0;
        bs->GetU(1, &general_one_picture_only_constraint_flag );
        mSeqParameterSet.general_one_picture_only_constraint_flag = general_one_picture_only_constraint_flag;
        bs->SkipBits(35);
    } else {
        // total bits 43
        bs->SkipBits(43);
    }
    // up to 44
    bs->SkipBits(1);
#else
    bs->SkipBits(44);
#endif

    uint8_t general_level_idc = 0;
    bs->GetU(8, &general_level_idc);
    mSeqParameterSet.general_level_idc = general_level_idc;

    std::vector<uint8_t> sub_layer_profile_present_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_level_present_flag(max_sub_layers_minus1, 0);
    for (uint8_t i = 0; i < max_sub_layers_minus1; ++i) {
        bs->GetU(1, &sub_layer_profile_present_flag[i]);
        bs->GetU(1, &sub_layer_level_present_flag[i]);
    }

    if (max_sub_layers_minus1 > 0) {
        for (uint8_t i = max_sub_layers_minus1; i < 8; ++i) {
            bs->SkipBits(2);
        }
    }

    std::vector<uint8_t> sub_layer_profile_space(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_tier_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_profile_idc(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_progressive_source_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_interlaced_source_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_non_packed_constraint_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_frame_only_constraint_flag(max_sub_layers_minus1, 0);
    std::vector<uint8_t> sub_layer_level_idc(max_sub_layers_minus1, 0);

    for (uint8_t i = 0; i < max_sub_layers_minus1; ++i) {
        if (sub_layer_profile_present_flag[i]) {
            bs->GetU(2, &sub_layer_profile_space[i]);
            bs->GetU(1, &sub_layer_tier_flag[i]);
            bs->GetU(5, &sub_layer_profile_idc[i]);
            for (int j = 0; j < 32; ++j) {
                bs->SkipBits(1);
            }
            bs->GetU(1, &sub_layer_progressive_source_flag[i]);
            bs->GetU(1, &sub_layer_interlaced_source_flag[i]);
            bs->GetU(1, &sub_layer_non_packed_constraint_flag[i]);
            bs->GetU(1, &sub_layer_frame_only_constraint_flag[i]);
            bs->SkipBits(44);
        }
        if (sub_layer_level_present_flag[i]) {
            bs->GetU(8, &sub_layer_level_idc[i]);
        }
    }

}

void HEVCEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    
    ResetFrameInfo();

    bool enough = false;
    while (!enough && mOffset < data_size - 5) {
        if (data[mOffset] == 0x00 && data[mOffset+1] == 0x00 && data[mOffset+2] == 0x01) {

            // advance start code
            mOffset += 3;
            /* extract nal header */

            uint8_t b1 = data[mOffset];
            uint8_t b2 = data[mOffset+1];

            // advance nal header
            mOffset += 2;

            uint32_t nal_unit_type = (b1 & 0x7e) >> 1;

            uint8_t nuh_layer_id_part1 = b1 & 0x01;            
            uint8_t nuh_layer_id_part2 = (b2 & 0xF8) >> 3;

            /* 
            In bitstreams conforming to the Main Intra, Main 10 Intra, Main 12 Intra, Main 4:2:2 10 Intra, Main 4:2:2 12 Intra,
            Main 4:4:4 Intra, Main 4:4:4 10 Intra, Main 4:4:4 12 Intra or Main 4:4:4 16 Intra profiles, all pictures with
            nuh_layer_id equal to 0 shall be IRAP pictures and the output order indicated in the bitstream among these pictures
            shall be the same as the decoding order.
            */
            uint16_t nuh_layer_id = nuh_layer_id_part1 | nuh_layer_id_part2;
            (void)nuh_layer_id; // to-do: do something with it?
            
            if (nal_unit_type == NAL_TYPE::SPS_NUT) {
                AVCGetBitBuffer bs(data + mOffset, data_size - mOffset);
                ParseSeqParameterSet(&bs);

                enough = true;
            }
        } else {
            mOffset++;
        }
    }

    CheckConformance();
}

void HEVCEssenceParser::CheckConformance()
{
    bool err = false;
    if (mSeqParameterSet.general_intra_constraint_flag == 0) {
        log_error("HEVC Intra Only allowed but general_intra_constraint_flag != 0\n");
        err = true;
    }
    if (mSeqParameterSet.chroma_format_idc != 2) {
        log_error("Only 4:2:2 chroma format is supported as of now.\n");
        err = true;
    }
    if (!mSeqParameterSet.general_max_10bit_constraint_flag || mSeqParameterSet.general_max_8bit_constraint_flag) {
        log_error("Only 10-bit essence supported but general_max_10bit_constraint_flag != 0 or mSeqParameterSet.general_max_8bit_constraint_flag == 1\n");
        err = true;
    }
    if (!(mSeqParameterSet.bit_depth_luma_minus8 >= 0 && mSeqParameterSet.bit_depth_luma_minus8 <= 2)) {
        log_error("Only 10-bit essence allowed but bit_depth_luma_minus8 != (0..2)\n");
        err = true;
    }

    if (err) {
        throw BMXException("Error in CheckConformance()");
    }
}

uint32_t HEVCEssenceParser::NextStartCodePrefix(const unsigned char *data, uint32_t size)
{
    const unsigned char *datap3 = data + 3;
    const unsigned char *end    = data + size;

    // loop logic is based on FFmpeg's avpriv_find_start_code in libavcodec/utils.c
    while (datap3 < end) {
        if (datap3[-1] > 1)
            datap3 += 3;
        else if (datap3[-2])
            datap3 += 2;
        else if (datap3[-3] | (datap3[-1] - 1))
            datap3++;
        else
            break;
    }
    if (datap3 < end)
        return (uint32_t)(datap3 - data) - 3;
    else
        return ESSENCE_PARSER_NULL_OFFSET;
}

void HEVCEssenceParser::ResetFrameInfo()
{
    mOffset = 0;
    mSeqParameterSet = HEVCSeqParameterSet();
}

EssenceType HEVCEssenceParser::GetEssenceType() const
{
    switch (mSeqParameterSet.general_profile_idc) {
        case 4:
        return CalculateEssenceTypeFromConstraints();
    }
    log_error("Invalid/unsupported HEVC profile. general_profile_idc = %d\n", mSeqParameterSet.general_profile_idc);
    BMX_ASSERT(false);
}

EssenceType HEVCEssenceParser::CalculateEssenceTypeFromConstraints() const
{
    if (mSeqParameterSet.general_max_420chroma_constraint_flag == 0
        && (mSeqParameterSet.chroma_format_idc >= 0 && mSeqParameterSet.chroma_format_idc <= 2))
    {
        return HEVC_MAIN_422_10_INTRA;
    }
    if (mSeqParameterSet.general_max_420chroma_constraint_flag == 1
        && (mSeqParameterSet.chroma_format_idc == 0 || mSeqParameterSet.chroma_format_idc == 1))
    {
        return HEVC_MAIN_10_INTRA;
    }
    log_error("Unsupported HEVC essence type\n");
    BMX_ASSERT(false);
}

