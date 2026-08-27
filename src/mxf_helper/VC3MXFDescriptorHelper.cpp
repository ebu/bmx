/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <algorithm>
#include <cstring>

#include <bmx/mxf_helper/VC3MXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;

/*
 * MXF AVID labels for DNxHR
 */
#define MXF_AVID_BASE_L(b8, b9, b10, b11, b12, b13, b14, b15, b16) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, b8, b9, b10, b11, b12, b13, b14, b15, b16}

#define MXF_AVID_DNXHR_EC_L                     MXF_AVID_BASE_L(0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x11, 0x02, 0x00)
#define MXF_DNXHR_CMDEF_L(byte14)               MXF_AVID_BASE_L(0x0d, 0x04, 0x01, 0x02, 0x02, 0x71, byte14, 0x00, 0x00)

static const mxfUL MXF_CMDEF_L(VC3_DNXHR_444) = MXF_DNXHR_CMDEF_L(0x24);
static const mxfUL MXF_CMDEF_L(VC3_DNXHR_HQX) = MXF_DNXHR_CMDEF_L(0x25);
static const mxfUL MXF_CMDEF_L(VC3_DNXHR_HQ)  = MXF_DNXHR_CMDEF_L(0x26);
static const mxfUL MXF_CMDEF_L(VC3_DNXHR_SQ)  = MXF_DNXHR_CMDEF_L(0x27);
static const mxfUL MXF_CMDEF_L(VC3_DNXHR_LB)  = MXF_DNXHR_CMDEF_L(0x28);


typedef struct
{
    mxfUL pc_label;
    EssenceType essence_type;
    int32_t resolution_id;
    uint32_t component_depth;
    uint32_t horiz_subsampling;
    uint32_t frame_size;
    uint32_t stored_width;
    uint32_t stored_height;
    uint32_t display_width;
    uint32_t display_height;
    mxfVideoLineMap video_line_map;
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL avid_pc_label;
    mxfUL avid_ec_label;
} SupportedEssence;

enum { VARI = 0 };

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(VC3_1080P_1235),  VC3_1080P_1235, 1235,   10,  2,  917504,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1235ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1237),  VC3_1080P_1237, 1237,   8,   2,  606208,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1237ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1238),  VC3_1080P_1238, 1238,   8,   2,  917504,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1238ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1241),  VC3_1080I_1241, 1241,   10,  2,  917504,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1241ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1242),  VC3_1080I_1242, 1242,   8,   2,  606208,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1242ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1243),  VC3_1080I_1243, 1243,   8,   2,  917504,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1243ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1244),  VC3_1080I_1244, 1244,   8,   2,  606208,  1440,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1244ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1250),   VC3_720P_1250,  1250,   10,  2,  458752,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1250ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1251),   VC3_720P_1251,  1251,   8,   2,  458752,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1251ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1252),   VC3_720P_1252,  1252,   8,   2,  303104,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1252ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1253),  VC3_1080P_1253, 1253,   8,   2,  188416,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1253ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1258),   VC3_720P_1258,  1258,   8,   2,  212992,  960,    720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1258ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1259),  VC3_1080P_1259, 1259,   8,   2,  417792,  1440,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1259ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1260),  VC3_1080I_1260, 1260,   8,   2,  417792,  1440,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1260ClipWrapped)},
    {MXF_CMDEF_L(VC3_DNXHR_444),   VC3_DNXHR_444,  1270,   12,  1,  VARI,    VARI,   VARI,   VARI,   VARI,   {1, 0},     MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(VC3_DNXHR_444), MXF_AVID_DNXHR_EC_L},
    {MXF_CMDEF_L(VC3_DNXHR_HQX),   VC3_DNXHR_HQX,  1271,   12,  2,  VARI,    VARI,   VARI,   VARI,   VARI,   {1, 0},     MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(VC3_DNXHR_HQX), MXF_AVID_DNXHR_EC_L},
    {MXF_CMDEF_L(VC3_DNXHR_HQ),    VC3_DNXHR_HQ,   1272,   8,   2,  VARI,    VARI,   VARI,   VARI,   VARI,   {1, 0},     MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(VC3_DNXHR_HQ), MXF_AVID_DNXHR_EC_L},
    {MXF_CMDEF_L(VC3_DNXHR_SQ),    VC3_DNXHR_SQ,   1273,   8,   2,  VARI,    VARI,   VARI,   VARI,   VARI,   {1, 0},     MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(VC3_DNXHR_SQ), MXF_AVID_DNXHR_EC_L},
    {MXF_CMDEF_L(VC3_DNXHR_LB),    VC3_DNXHR_LB,   1274,   8,   2,  VARI,    VARI,   VARI,   VARI,   VARI,   {1, 0},     MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(VC3_DNXHR_LB), MXF_AVID_DNXHR_EC_L},
};

typedef struct
{
    int32_t resolution_id;
    Rational packet_scale;
} CompressionParams;

static const CompressionParams COMPRESSION_PARAMS[] =
{
    {1270, {0xe000, 0xff}},  // DNxHR 444 12-bit
    {1271, {0x7000, 0xff}},  // DNxHR HQX 12-bit
    {1272, {0x7000, 0xff}},  // DNxHR HQ
    {1273, {0x4a00, 0xff}},  // DNxHR SQ
    {1274, {0x1700, 0xff}},  // DNxHR LB
};


static uint32_t get_hr_frame_size(int32_t resolution_id, uint32_t w, uint32_t h)
{
    size_t param_index;
    for (param_index = 0; param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMS); param_index++)
    {
        if (COMPRESSION_PARAMS[param_index].resolution_id == resolution_id)
            break;
    }
    BMX_CHECK(param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMS));

    Rational packet_scale = COMPRESSION_PARAMS[param_index].packet_scale;
    uint32_t result = ((w + 15) / 16) * ((h + 15) / 16) * packet_scale.numerator / packet_scale.denominator;
    result = (result + 2048) & ~0xFFF;
    return max(result, (uint32_t)8192);
}


EssenceType VC3MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3FrameWrapped)) &&
        !CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3ClipWrapped)))
    {
        size_t index;
        if (IsAvidDNxHD(file_descriptor, alternative_ec_label, &index))
            return SUPPORTED_ESSENCE[index].essence_type;

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

bool VC3MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

bool VC3MXFDescriptorHelper::IsAvidDNxHD(FileDescriptor *file_descriptor, mxfUL alternative_ec_label, size_t *index)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].avid_ec_label))
            break;
    }
    if (i >= BMX_ARRAY_SIZE(SUPPORTED_ESSENCE))
        return false;

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return false;
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();

    bool result = mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].avid_pc_label);
    if (result)
        *index = i;

    return result;
}

VC3MXFDescriptorHelper::VC3MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
    BMX_OPT_PROP_DEFAULT(mFrameWidth, 0);
    BMX_OPT_PROP_DEFAULT(mFrameHeight, 0);
}

VC3MXFDescriptorHelper::~VC3MXFDescriptorHelper()
{
}

void VC3MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                        mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    if (IsAvidDNxHD(file_descriptor, alternative_ec_label, &mEssenceIndex)) {
        mFrameWrapped = false;
        mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;
        mAvidResolutionId = SUPPORTED_ESSENCE[mEssenceIndex].resolution_id;
    } else {
        mxfUL ec_label = file_descriptor->getEssenceContainer();
        mFrameWrapped = CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3FrameWrapped));

        GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
        mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
            if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label)) {
                mEssenceIndex = i;
                mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
                mAvidResolutionId = SUPPORTED_ESSENCE[i].resolution_id;
                break;
            }
        }
    }

    if (SUPPORTED_ESSENCE[mEssenceIndex].stored_width == VARI) {
        GenericPictureEssenceDescriptor *picture_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
        BMX_ASSERT(picture_descriptor);
        BMX_CHECK(picture_descriptor->haveDisplayWidth() && picture_descriptor->haveDisplayHeight());

        BMX_OPT_PROP_SET(mFrameWidth, picture_descriptor->getDisplayWidth());
        BMX_OPT_PROP_SET(mFrameHeight, picture_descriptor->getDisplayHeight());
    }
}

void VC3MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == essence_type) {
            mEssenceIndex = i;
            // DNxHR: do NOT write Avid ResolutionID (golden doesn't have it)
            if (SUPPORTED_ESSENCE[i].frame_size == VARI)
                mAvidResolutionId = 0;
            else
                mAvidResolutionId = SUPPORTED_ESSENCE[i].resolution_id;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);
}

void VC3MXFDescriptorHelper::SetFrameWidth(uint32_t frame_width)
{
    BMX_OPT_PROP_SET(mFrameWidth, frame_width);
}

void VC3MXFDescriptorHelper::SetFrameHeight(uint32_t frame_height)
{
    BMX_OPT_PROP_SET(mFrameHeight, frame_height);
}

FileDescriptor* VC3MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void VC3MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    // DNxHR: override the generic AAF-KLV EC label with the actual DNxHR EC label
    // (parent sets AvidAAFKLVEssenceContainer for all Avid flavour, but DNxHR needs its own)
    if ((mFlavour & MXFDESC_AVID_FLAVOUR) && SUPPORTED_ESSENCE[mEssenceIndex].frame_size == VARI) {
        mFileDescriptor->setEssenceContainer(SUPPORTED_ESSENCE[mEssenceIndex].avid_ec_label);
    }

    if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].avid_pc_label);
    else
        cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    // DNxHR: do NOT write SignalStandard (golden and Resolve don't have it)
    if (SUPPORTED_ESSENCE[mEssenceIndex].frame_size != VARI)
        cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    SetColorSitingMod(MXF_COLOR_SITING_REC601);
    cdci_descriptor->setComponentDepth(SUPPORTED_ESSENCE[mEssenceIndex].component_depth);
    if (SUPPORTED_ESSENCE[mEssenceIndex].component_depth == 12) {
        // DNxHR 12-bit
        cdci_descriptor->setBlackRefLevel(64);
        cdci_descriptor->setWhiteReflevel(940);
        cdci_descriptor->setColorRange(897);
    } else if (SUPPORTED_ESSENCE[mEssenceIndex].frame_size == VARI) {
        // DNxHR 8-bit (LB/SQ/HQ) — Avid uses non-standard levels
        cdci_descriptor->setBlackRefLevel(23);
        cdci_descriptor->setWhiteReflevel(149);
        cdci_descriptor->setColorRange(225);
    } else if (SUPPORTED_ESSENCE[mEssenceIndex].component_depth == 10) {
        cdci_descriptor->setBlackRefLevel(64);
        cdci_descriptor->setWhiteReflevel(940);
        cdci_descriptor->setColorRange(897);
    } else {
        cdci_descriptor->setBlackRefLevel(16);
        cdci_descriptor->setWhiteReflevel(235);
        cdci_descriptor->setColorRange(225);
    }
    if (SUPPORTED_ESSENCE[mEssenceIndex].frame_size == VARI) {
        // DNxHR: write CodingEquations as straight UL, not AUID half-swapped
        // (Avid DNxHR uses standard UL format, unlike DNxHD which uses AAF AUID)
        GenericPictureEssenceDescriptor *pic_desc = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
        pic_desc->setCodingEquations(ITUR_BT709_CODING_EQ);
        // DNxHR: add TransferCharacteristic and ColorPrimaries (both working files have them)
        pic_desc->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
        pic_desc->setColorPrimaries(ITU709_COLOR_PRIM);
    } else {
        SetCodingEquationsMod(ITUR_BT709_CODING_EQ);
    }
    if (SUPPORTED_ESSENCE[mEssenceIndex].stored_width == VARI) {
        BMX_CHECK(BMX_OPT_PROP_IS_SET(mFrameWidth) && BMX_OPT_PROP_IS_SET(mFrameHeight));
        cdci_descriptor->setStoredWidth(mFrameWidth);
        cdci_descriptor->setStoredHeight(mFrameHeight);
        cdci_descriptor->setDisplayWidth(mFrameWidth);
        cdci_descriptor->setDisplayHeight(mFrameHeight);
    } else {
        cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].stored_width);
        cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].stored_height);
        cdci_descriptor->setDisplayWidth(SUPPORTED_ESSENCE[mEssenceIndex].display_width);
        cdci_descriptor->setDisplayHeight(SUPPORTED_ESSENCE[mEssenceIndex].display_height);
    }
    cdci_descriptor->setSampledWidth(cdci_descriptor->getDisplayWidth());
    cdci_descriptor->setSampledHeight(cdci_descriptor->getDisplayHeight());
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        cdci_descriptor->setSampledXOffset(0);
        cdci_descriptor->setSampledYOffset(0);
        cdci_descriptor->setDisplayXOffset(0);
        cdci_descriptor->setDisplayYOffset(0);
    }
    cdci_descriptor->setVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map);
    cdci_descriptor->setHorizontalSubsampling(SUPPORTED_ESSENCE[mEssenceIndex].horiz_subsampling);
    cdci_descriptor->setVerticalSubsampling(1);
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        if (SUPPORTED_ESSENCE[mEssenceIndex].frame_size == VARI) {
            cdci_descriptor->setImageAlignmentOffset(4096); // Avid aligns DNxHR to 4096 bytes
            // DNxHR: set aspect ratio based on frame dimensions
            if (BMX_OPT_PROP_IS_SET(mFrameWidth) && BMX_OPT_PROP_IS_SET(mFrameHeight)) {
                mxfRational aspect_ratio;
                aspect_ratio.numerator = 256;
                aspect_ratio.denominator = 135;
                cdci_descriptor->setAspectRatio(aspect_ratio);
            }
        } else {
            cdci_descriptor->setImageAlignmentOffset(8192);
        }
    }
}

uint32_t VC3MXFDescriptorHelper::GetSampleSize()
{
    uint32_t frame_size = SUPPORTED_ESSENCE[mEssenceIndex].frame_size;

    if (frame_size == VARI) {
        BMX_CHECK(BMX_OPT_PROP_IS_SET(mFrameWidth) && BMX_OPT_PROP_IS_SET(mFrameHeight));
        frame_size = get_hr_frame_size(SUPPORTED_ESSENCE[mEssenceIndex].resolution_id, mFrameWidth, mFrameHeight);
    }

    return frame_size;
}

mxfUL VC3MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        BMX_ASSERT(!mFrameWrapped);
        return SUPPORTED_ESSENCE[mEssenceIndex].avid_ec_label;
    } else {
        if (mFrameWrapped)
            return MXF_EC_L(VC3FrameWrapped);
        else
            return MXF_EC_L(VC3ClipWrapped);
    }
}
