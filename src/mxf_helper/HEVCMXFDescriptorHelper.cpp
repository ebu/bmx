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

#include <bmx/mxf_helper/HEVCMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    mxfUL pc_label;
    EssenceType essence_type;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(HEVC_MAIN),                 HEVC_MAIN},
    {MXF_CMDEF_L(HEVC_MAIN_10),              HEVC_MAIN_10},
    {MXF_CMDEF_L(HEVC_MAIN_10_INTRA),        HEVC_MAIN_10_INTRA},
    {MXF_CMDEF_L(HEVC_MAIN_422_10_INTRA),    HEVC_MAIN_422_10_INTRA},
};



EssenceType HEVCMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_is_avc_ec(&ec_label, 0) &&
        !mxf_is_avc_ec(&ec_label, 1) &&
        !mxf_is_avc_ec(&alternative_ec_label, 0) &&
        !mxf_is_avc_ec(&alternative_ec_label, 1) &&
        !IsNullAvidECUL(ec_label, alternative_ec_label))
    {
        return UNKNOWN_ESSENCE_TYPE;
    }

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool HEVCMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

void HEVCMXFDescriptorHelper::MapColorPrimaries(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(pict_helper->GetFileDescriptor());
    BMX_ASSERT(cdci_descriptor);

    switch (avc_value) {
        case 1:
            cdci_descriptor->setColorPrimaries(ITU709_COLOR_PRIM);
            break;
        case 4:
            cdci_descriptor->setColorPrimaries(ITU470_PAL_COLOR_PRIM);
            break;
        case 5:
            cdci_descriptor->setColorPrimaries(ITU470_PAL_COLOR_PRIM);
            break;
        case 6:
            cdci_descriptor->setColorPrimaries(SMPTE170M_COLOR_PRIM);
            break;
        case 9:
            cdci_descriptor->setColorPrimaries(ITU2020_COLOR_PRIM);
            break;
        case 11: 
            // SMPTE P3DCI, ST 2113
            cdci_descriptor->setColorPrimaries(SMPTE_DCDM_COLOR_PRIM);
            break;
        case 12:
            cdci_descriptor->setColorPrimaries(P3D65_COLOR_PRIM);
            break;
        default:
            log_warn("ColorPrimaries with value %d not mapped to SMPTE UL\n");
            break;
    }
}

void HEVCMXFDescriptorHelper::MapTransferCharacteristic(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(pict_helper->GetFileDescriptor());
    BMX_ASSERT(cdci_descriptor);

    switch (avc_value) {
        case 1:
            cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
            break;
        case 4:
        case 5:
            cdci_descriptor->setCaptureGamma(ITUR_BT470_TRANSFER_CH);
            break;
        case 7:
            cdci_descriptor->setCaptureGamma(SMPTE240M_TRANSFER_CH);
            break;
        case 8:
            cdci_descriptor->setCaptureGamma(LINEAR_TRANSFER_CH);
            break;
        case 11:
            cdci_descriptor->setCaptureGamma(IEC6196624_XVYCC_TRANSFER_CH);
            break;
        case 12:
            cdci_descriptor->setCaptureGamma(ITU1361_TRANSFER_CH);
            break;
        case 14:
            cdci_descriptor->setCaptureGamma(ITU2020_TRANSFER_CH);
            break;
        case 15:
            cdci_descriptor->setCaptureGamma(ITU2020_TRANSFER_CH);
            break;
        case 16:
            cdci_descriptor->setCaptureGamma(SMPTE_ST2084_TRANSFER_CH);
            break;
        case 18:
            // ITU-R BT.2100-2 hybrid log gamma (HLG)
            cdci_descriptor->setCaptureGamma(HLG_OETF_TRANSFER_CH);
            break;
        default:
            log_warn("HEVC - Transfer Characteristic with value %d not mapped to SMPTE UL\n");
            break;
    }
}

void HEVCMXFDescriptorHelper::MapMatrixCoefficients(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    switch (avc_value) {
        case 0:
            // identity
            pict_helper->SetCodingEquationsMod(GBR_CODING_EQ);
            break;
        case 1:
            pict_helper->SetCodingEquationsMod(ITUR_BT709_CODING_EQ);
            break;
        case 5:
            pict_helper->SetCodingEquationsMod(ITUR_BT601_CODING_EQ);
            break;
        case 6:
            pict_helper->SetCodingEquationsMod(ITUR_BT601_CODING_EQ);
            break;
        case 7:
            pict_helper->SetCodingEquationsMod(SMPTE_240M_CODING_EQ);
            break;
        case 8:
            pict_helper->SetCodingEquationsMod(Y_CG_CO_CODING_EQ);
            break;
        case 9:
            pict_helper->SetCodingEquationsMod(ITU2020_NCL_CODING_EQ);
            break;
        case 10:
            // Constant Luminance System
            //pict_helper->SetCodingEquationsMod(ITU2020_NCL_CODING_EQ);
            break;
        default:
            log_warn("HEVC - MatrixCoeff with value %d not mapped to SMPTE UL\n");
            break;
    }
}

HEVCMXFDescriptorHelper::HEVCMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
    mHEVCSubDescriptor = nullptr;
}

HEVCMXFDescriptorHelper::~HEVCMXFDescriptorHelper()
{
}

void HEVCMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                        mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = (mxf_is_avc_ec(&ec_label, 1) || mxf_is_avc_ec(&alternative_ec_label, 1));

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            break;
        }
    }

    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (i = 0; i < sub_descriptors.size(); i++) {
            mHEVCSubDescriptor = dynamic_cast<HEVCSubDescriptor*>(sub_descriptors[i]);
            if (mHEVCSubDescriptor)
                break;
        }
    }
}

void HEVCMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

FileDescriptor* HEVCMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    mHEVCSubDescriptor = new HEVCSubDescriptor(header_metadata);
    mFileDescriptor->appendSubDescriptors(mHEVCSubDescriptor);

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void HEVCMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    mxfUL coding = SUPPORTED_ESSENCE[mEssenceIndex].pc_label;
    cdci_descriptor->setPictureEssenceCoding(coding);
}

void HEVCMXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    CDCIEssenceDescriptor *cdci_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);
    BMX_CHECK(cdci_desc_in);

#define SET_PROPERTY(name)                                                \
    if (cdci_desc_in->have##name() && !cdci_descriptor->have##name())     \
        cdci_descriptor->set##name(cdci_desc_in->get##name());

    // TODO: move these to PictureMXFDescriptorHelper once this update is supported across all formats

    SET_PROPERTY(SignalStandard)
    SET_PROPERTY(FrameLayout)
    SET_PROPERTY(AspectRatio)
    SET_PROPERTY(ActiveFormatDescriptor)
    SET_PROPERTY(VideoLineMap)
    SET_PROPERTY(FieldDominance)
    SET_PROPERTY(CaptureGamma)
    SET_PROPERTY(CodingEquations)
    SET_PROPERTY(ColorPrimaries)
    if (!cdci_descriptor->haveColorSiting() && cdci_desc_in->haveColorSiting())
        SetColorSitingMod(cdci_desc_in->getColorSiting());
    SET_PROPERTY(BlackRefLevel)
    SET_PROPERTY(WhiteReflevel)
    SET_PROPERTY(ColorRange)
}

void HEVCMXFDescriptorHelper::UpdateFileDescriptor(HEVCEssenceParser *essence_parser)
{
    BMX_ASSERT(essence_parser);
    if (essence_parser->GetEssenceType() != HEVC_MAIN_422_10_INTRA) {
        log_error("HEVCMXFDescriptorHelper only supports HEVC Main 4:2:2 10 INTRA\n");
        BMX_ASSERT(false);
    }

    const HEVCSeqParameterSet &seq_param_set = essence_parser->GetSeqParameterSet();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    // To-DO: We can for now assume that we have 422:10bit essence ONLY!
    cdci_descriptor->setStoredWidth(seq_param_set.pic_width_in_luma_samples);
    cdci_descriptor->setStoredHeight(seq_param_set.pic_height_in_luma_samples);
    cdci_descriptor->setDisplayWidth(seq_param_set.pic_width_in_luma_samples);
    cdci_descriptor->setDisplayHeight(seq_param_set.pic_height_in_luma_samples);
    cdci_descriptor->setDisplayXOffset(0);
    cdci_descriptor->setDisplayYOffset(0);
    cdci_descriptor->setSampledWidth(seq_param_set.pic_width_in_luma_samples);
    cdci_descriptor->setSampledHeight(seq_param_set.pic_height_in_luma_samples);
    cdci_descriptor->setSampledXOffset(0);
    cdci_descriptor->setSampledYOffset(0);
    cdci_descriptor->setImageStartOffset(0);
    cdci_descriptor->setPaddingBits(0);
    cdci_descriptor->setComponentDepth(seq_param_set.bit_depth_luma_minus8 + 8);

    // 4:2:2 only
    if (seq_param_set.chroma_format_idc == 2) {
        cdci_descriptor->setHorizontalSubsampling(2);
        cdci_descriptor->setVerticalSubsampling(2);
        SetColorSitingMod(MXF_COLOR_SITING_COSITING);
    }

    switch (seq_param_set.video_format) {
        case 0: // Component
            break;
        case 1: // PAL
            break;
        case 2: // NTSC
            break;
        case 3: // SECAM
            break;
        case 4: // MAC
            break;
        case 5: // Unspecified
            //cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_ITU601);
            break;
        default:
            break;
    }

    MapColorPrimaries(seq_param_set.color_primaries, this);
    MapTransferCharacteristic(seq_param_set.transfer_characteristics, this);
    MapMatrixCoefficients(seq_param_set.matrix_coeffs, this);

    Rational aspect_ratio = ZERO_RATIONAL;
    if (seq_param_set.aspect_ratio_idc == HEVC_ASPECT_RATIO_IDC_EXTENDED_SAR) {
        aspect_ratio.numerator   = (int32_t)(seq_param_set.sar_width  * seq_param_set.pic_width_in_luma_samples);
        aspect_ratio.denominator = (int32_t)(seq_param_set.sar_height * seq_param_set.pic_height_in_luma_samples);
    } else {
        static const Rational aspect_ratios[] =
        {
            {  0,  1},
            {  1,  1},
            { 12, 11},
            { 10, 11},
            { 16, 11},
            { 40, 33},
            { 24, 11},
            { 20, 11},
            { 32, 11},
            { 80, 33},
            { 18, 11},
            { 15, 11},
            { 64, 33},
            {160, 99},
            {  4,  3},
            {  3,  2},
            {  2,  1},
        };
        if (seq_param_set.aspect_ratio_idc >= BMX_ARRAY_SIZE(aspect_ratios)) {
            log_warn("Invalid aspect_ratio_idc %u\n", seq_param_set.aspect_ratio_idc);
        } else {
            aspect_ratio = aspect_ratios[seq_param_set.aspect_ratio_idc];
        }
    }
    cdci_descriptor->setAspectRatio(reduce_rational(aspect_ratio));

    BMX_ASSERT(mHEVCSubDescriptor);

    
    // only INTRA 422 10 bit
    mHEVCSubDescriptor->setDecodingDelay(0);
    mHEVCSubDescriptor->setConstantBPictureFlag(true);
    mHEVCSubDescriptor->setCodedContentKind(1); // progressive
    mHEVCSubDescriptor->setClosedGOPIndicator(true);
    mHEVCSubDescriptor->setIdenticalGOPIndicator(true);
    mHEVCSubDescriptor->setMaximumGOPSize(1);
    mHEVCSubDescriptor->setMaximumBPictureCount(0);
    mHEVCSubDescriptor->setProfile(seq_param_set.general_profile_idc);
    mHEVCSubDescriptor->setProfileConstraint(seq_param_set.profile_constraint);
    mHEVCSubDescriptor->setLevel(seq_param_set.general_level_idc);
    mHEVCSubDescriptor->setTier(seq_param_set.general_tier_flag);
    mHEVCSubDescriptor->setMaximumRefFrames(0);
    //in-band at every access unit + constant
    uint8_t flag = 0b00000101;
    mHEVCSubDescriptor->setSequenceParameterSetFlag(flag);
    mHEVCSubDescriptor->setPictureParameterSetFlag(flag);
    mHEVCSubDescriptor->setVideoParameterSetFlag(flag);
}

mxfUL HEVCMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFrameWrapped)
        return MXF_EC_L(HEVCFrameWrapped);
    log_error("Clip wrapping not supported");
    BMX_ASSERT(false);
}

void HEVCMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType) {
            mEssenceIndex = i;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

