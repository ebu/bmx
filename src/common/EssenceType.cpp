/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <bmx/EssenceType.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>

using namespace bmx;


typedef struct
{
    EssenceType essence_type;
    EssenceType generic_essence_type;
    const char *str;
    const char *enum_str;
    const char *file_extension;
} EssenceTypeInfo;

static const EssenceTypeInfo ESSENCE_TYPE_INFO[] =
{
    {UNKNOWN_ESSENCE_TYPE,      UNKNOWN_ESSENCE_TYPE,   "unknown essence type",                 "Unknown",                  NULL},
    {PICTURE_ESSENCE,           PICTURE_ESSENCE,        "picture essence",                      "Picture",                  NULL},
    {SOUND_ESSENCE,             SOUND_ESSENCE,          "sound essence",                        "Sound",                    NULL},
    {DATA_ESSENCE,              DATA_ESSENCE,           "data essence",                         "Data",                     NULL},
    {D10_30,                    PICTURE_ESSENCE,        "D10 30",                               "D10_30",                   NULL},
    {D10_40,                    PICTURE_ESSENCE,        "D10 40",                               "D10_40",                   NULL}, 
    {D10_50,                    PICTURE_ESSENCE,        "D10 50",                               "D10_50",                   NULL},
    {IEC_DV25,                  PICTURE_ESSENCE,        "IEC DV25",                             "IEC_DV_25",                NULL},
    {DVBASED_DV25,              PICTURE_ESSENCE,        "DV-Based DV25",                        "DV_Based_DV_25",           NULL},
    {DV50,                      PICTURE_ESSENCE,        "DV50",                                 "DV_50",                    NULL},
    {DV100_1080I,               PICTURE_ESSENCE,        "DV100 1080i",                          "DV_100_1080i",             NULL},
    {DV100_720P,                PICTURE_ESSENCE,        "DV100 720p",                           "DV_100_720p",              NULL},
    {AVCI200_1080I,             PICTURE_ESSENCE,        "AVCI 200 1080i",                       "AVCI_200_1080i",           NULL},
    {AVCI200_1080P,             PICTURE_ESSENCE,        "AVCI 200 1080p",                       "AVCI_200_1080p",           NULL},
    {AVCI200_720P,              PICTURE_ESSENCE,        "AVCI 200 720p",                        "AVCI_200_720p",            NULL},
    {AVCI100_1080I,             PICTURE_ESSENCE,        "AVCI 100 1080i",                       "AVCI_100_1080i",           NULL},
    {AVCI100_1080P,             PICTURE_ESSENCE,        "AVCI 100 1080p",                       "AVCI_100_1080p",           NULL},
    {AVCI100_720P,              PICTURE_ESSENCE,        "AVCI 100 720p",                        "AVCI_100_720p",            NULL},
    {AVCI50_1080I,              PICTURE_ESSENCE,        "AVCI 50 1080i",                        "AVCI_50_1080i",            NULL},
    {AVCI50_1080P,              PICTURE_ESSENCE,        "AVCI 50 1080p",                        "AVCI_50_1080p",            NULL},
    {AVCI50_720P,               PICTURE_ESSENCE,        "AVCI 50 720p",                         "AVCI_50_720p",             "avc"},
    {AVC_BASELINE,              PICTURE_ESSENCE,        "AVC Baseline",                         "AVC_Baseline",             "avc"},
    {AVC_CONSTRAINED_BASELINE,  PICTURE_ESSENCE,        "AVC Constrained Baseline",             "AVC_Constrained_Baseline", "avc"},
    {AVC_MAIN,                  PICTURE_ESSENCE,        "AVC Main",                             "AVC_Main",                 "avc"},
    {AVC_EXTENDED,              PICTURE_ESSENCE,        "AVC Extended",                         "AVC_Extended",             "avc"},
    {AVC_HIGH,                  PICTURE_ESSENCE,        "AVC High",                             "AVC_High",                 "avc"},
    {AVC_HIGH_10,               PICTURE_ESSENCE,        "AVC High 10",                          "AVC_High_10",              "avc"},
    {AVC_HIGH_422,              PICTURE_ESSENCE,        "AVC High 4:2:2",                       "AVC_High_422",             "avc"},
    {AVC_HIGH_444,              PICTURE_ESSENCE,        "AVC High 4:4:4",                       "AVC_High_444",             "avc"},
    {AVC_HIGH_10_INTRA,         PICTURE_ESSENCE,        "AVC High 10 Intra",                    "AVC_High_10_Intra",        "avc"},
    {AVC_HIGH_422_INTRA,        PICTURE_ESSENCE,        "AVC High 4:2:2 Intra",                 "AVC_High_422_Intra",       "avc"},
    {AVC_HIGH_444_INTRA,        PICTURE_ESSENCE,        "AVC High 4:4:4 Intra",                 "AVC_High_444_Intra",       "avc"},
    {AVC_CAVLC_444_INTRA,       PICTURE_ESSENCE,        "AVC CAVLC 4:4:4 Intra",                "AVC_CAVLC_444_Intra",      "avc"},
    {UNC_SD,                    PICTURE_ESSENCE,        "uncompressed SD",                      "Unc_SD",                   NULL},
    {UNC_HD_1080I,              PICTURE_ESSENCE,        "uncompressed HD 1080i",                "Unc_HD_1080i",             NULL},
    {UNC_HD_1080P,              PICTURE_ESSENCE,        "uncompressed HD 1080p",                "Unc_HD_1080p",             NULL},
    {UNC_HD_720P,               PICTURE_ESSENCE,        "uncompressed HD 720p",                 "Unc_HD_720p",              NULL},
    {UNC_UHD_3840,              PICTURE_ESSENCE,        "uncompressed UHD 3840x2160",           "Unc_UHD_3840",             NULL},
    {AVID_10BIT_UNC_SD,         PICTURE_ESSENCE,        "Avid 10-bit uncompressed SD",          "Avid_10b_Unc_SD",          NULL},
    {AVID_10BIT_UNC_HD_1080I,   PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 1080i",    "Avid_10b_Unc_HD_1080i",    NULL},
    {AVID_10BIT_UNC_HD_1080P,   PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 1080p",    "Avid_10b_Unc_HD_1080p",    NULL},
    {AVID_10BIT_UNC_HD_720P,    PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 720p",     "Avid_10b_Unc_HD_720p",     NULL},
    {AVID_ALPHA_SD,             PICTURE_ESSENCE,        "Avid uncompressed Alpha SD",           "Avid_Unc_Alpha_SD",        NULL},
    {AVID_ALPHA_HD_1080I,       PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 1080i",     "Avid_Unc_Alpha_HD_1080i",  NULL},
    {AVID_ALPHA_HD_1080P,       PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 1080p",     "Avid_Unc_Alpha_HD_1080p",  NULL},
    {AVID_ALPHA_HD_720P,        PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 720p",      "Avid_Unc_Alpha_HD_720p",   NULL},
    {MPEG2LG_422P_ML_576I,      PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@ML 576i",         "MPEG_2_Long_GOP_422P_ML_576i",     NULL},
    {MPEG2LG_MP_ML_576I,        PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@ML 576i",           "MPEG_2_Long_GOP_MP_ML_576i",       NULL},
    {MPEG2LG_422P_HL_1080I,     PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 1080i",        "MPEG_2_Long_GOP_422P_HL_1080i",    NULL},
    {MPEG2LG_422P_HL_1080P,     PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 1080p",        "MPEG_2_Long_GOP_422P_HL_1080p",    NULL},
    {MPEG2LG_422P_HL_720P,      PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 720p",         "MPEG_2_Long_GOP_422P_HL_720p",     NULL},
    {MPEG2LG_MP_HL_1920_1080I,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1920x1080i",     "MPEG_2_Long_GOP_MP_HL_1920_1080i", NULL},
    {MPEG2LG_MP_HL_1920_1080P,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1920x1080p",     "MPEG_2_Long_GOP_MP_HL_1920_1080p", NULL},
    {MPEG2LG_MP_HL_1440_1080I,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1440x1080i",     "MPEG_2_Long_GOP_MP_HL_1440_1080i", NULL},
    {MPEG2LG_MP_HL_1440_1080P,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1440x1080p",     "MPEG_2_Long_GOP_MP_HL_1440_1080p", NULL},
    {MPEG2LG_MP_HL_720P,        PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 720p",           "MPEG_2_Long_GOP_MP_HL_720p",       NULL},
    {MPEG2LG_MP_H14_1080I,      PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@H14 1080i",         "MPEG_2_Long_GOP_MP_H14_1080i",     NULL},
    {MPEG2LG_MP_H14_1080P,      PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@H14 1080p",         "MPEG_2_Long_GOP_MP_H14_1080p",     NULL},
    {RDD36_422_PROXY,           PICTURE_ESSENCE,        "RDD36 4:2:2 Proxy",                    "RDD36_422_Proxy",                  "prores"},
    {RDD36_422_LT,              PICTURE_ESSENCE,        "RDD36 4:2:2 LT",                       "RDD36_422_LT",                     "prores"},
    {RDD36_422,                 PICTURE_ESSENCE,        "RDD36 4:2:2",                          "RDD36_422",                        "prores"},
    {RDD36_422_HQ,              PICTURE_ESSENCE,        "RDD36 4:2:2 HQ",                       "RDD36_422_HQ",                     "prores"},
    {RDD36_4444,                PICTURE_ESSENCE,        "RDD36 4:4:4:4",                        "RDD36_4444",                       "prores"},
    {RDD36_4444_XQ,             PICTURE_ESSENCE,        "RDD36 4:4:4:4 XQ",                     "RDD36_4444_XQ",                    "prores"},
    {JPEG2000_CDCI,             PICTURE_ESSENCE,        "JPEG 2000 CDCI",                       "JPEG2000_CDCI",                    NULL},
    {JPEG2000_RGBA,             PICTURE_ESSENCE,        "JPEG 2000 RGBA",                       "JPEG2000_RGBA",                    NULL},
    {VC2,                       PICTURE_ESSENCE,        "VC2",                                  "VC2",                              NULL},
    {VC3_1080P_1235,            PICTURE_ESSENCE,        "VC3 1080p 1235",                       "VC3_1080p_1235",                   NULL},
    {VC3_1080P_1237,            PICTURE_ESSENCE,        "VC3 1080p 1237",                       "VC3_1080p_1237",                   NULL},
    {VC3_1080P_1238,            PICTURE_ESSENCE,        "VC3 1080p 1238",                       "VC3_1080p_1238",                   NULL},
    {VC3_1080I_1241,            PICTURE_ESSENCE,        "VC3 1080i 1241",                       "VC3_1080i_1241",                   NULL},
    {VC3_1080I_1242,            PICTURE_ESSENCE,        "VC3 1080i 1242",                       "VC3_1080i_1242",                   NULL},
    {VC3_1080I_1243,            PICTURE_ESSENCE,        "VC3 1080i 1243",                       "VC3_1080i_1243",                   NULL},
    {VC3_1080I_1244,            PICTURE_ESSENCE,        "VC3 1080i 1244",                       "VC3_1080i_1244",                   NULL},
    {VC3_720P_1250,             PICTURE_ESSENCE,        "VC3 720p 1250",                        "VC3_720p_1250",                    NULL},
    {VC3_720P_1251,             PICTURE_ESSENCE,        "VC3 720p 1251",                        "VC3_720p_1251",                    NULL},
    {VC3_720P_1252,             PICTURE_ESSENCE,        "VC3 720p 1252",                        "VC3_720p_1252",                    NULL},
    {VC3_1080P_1253,            PICTURE_ESSENCE,        "VC3 1080p 1253",                       "VC3_1080p_1253",                   NULL},
    {VC3_720P_1258,             PICTURE_ESSENCE,        "VC3 720p 1258",                        "VC3_720p_1258",                    NULL},
    {VC3_1080P_1259,            PICTURE_ESSENCE,        "VC3 1080p 1259",                       "VC3_1080p_1259",                   NULL},
    {VC3_1080I_1260,            PICTURE_ESSENCE,        "VC3 1080i 1260",                       "VC3_1080i_1260",                   NULL},
    {MJPEG_2_1,                 PICTURE_ESSENCE,        "MJPEG 2:1",                            "MJPEG_2_1",                        NULL},
    {MJPEG_3_1,                 PICTURE_ESSENCE,        "MJPEG 3:1",                            "MJPEG_3_1",                        NULL},
    {MJPEG_10_1,                PICTURE_ESSENCE,        "MJPEG 10:1",                           "MJPEG_10_1",                       NULL},
    {MJPEG_20_1,                PICTURE_ESSENCE,        "MJPEG 20:1",                           "MJPEG_20_1",                       NULL},
    {MJPEG_4_1M,                PICTURE_ESSENCE,        "MJPEG 4:1m",                           "MJPEG_4_1_M",                      NULL},
    {MJPEG_10_1M,               PICTURE_ESSENCE,        "MJPEG 10:1m",                          "MJPEG_10_1_M",                     NULL},
    {MJPEG_15_1S,               PICTURE_ESSENCE,        "MJPEG 15:1s",                          "MJPEG_15_1_S",                     NULL},
    {WAVE_PCM,                  SOUND_ESSENCE,          "WAVE PCM",                             "WAVE_PCM",                         "pcm"},
    {D10_AES3_PCM,              SOUND_ESSENCE,          "D10 AES3 PCM",                         "D10_AES3_PCM",                     NULL},
    {ANC_DATA,                  DATA_ESSENCE,           "ANC data",                             "ANC_Data",                         NULL},
    {VBI_DATA,                  DATA_ESSENCE,           "VBI data",                             "VBI_Data",                         NULL},
    {TIMED_TEXT,                DATA_ESSENCE,           "Timed Text",                           "Timed_Text",                       NULL},
    {IAB,                       SOUND_ESSENCE,          "Immersive Audio Bitstream",            "IAB",                              "iab"},
    {SADM,                      SOUND_ESSENCE,          "Serial Audio Definition Model",        "MGA",                              "sadm"},
    {ISXD,                      DATA_ESSENCE,           "ISXD Data Essence",                    "ISXD",                             "isxd"},
    /* HEVC */
    {HEVC_MAIN,                 PICTURE_ESSENCE,        "HEVC Main",                            "HEVC_Main",                        "hevc"},
    {HEVC_MAIN_10,              PICTURE_ESSENCE,        "HEVC Main 10",                         "HEVC_Main_10",                     "hevc"},

    /* HEVC Extended */
    {HEVC_MAIN_10_INTRA,        PICTURE_ESSENCE,        "HEVC Main Intra 10-bit",          "HEVC_Main_10_Intra",                    "hevc"},
    {HEVC_MAIN_422_10_INTRA,    PICTURE_ESSENCE,        "HEVC Main 4:2:2 10 Intra",        "HEVC_Main_422_10_Intra",                "hevc"}
};


const char* bmx::essence_type_to_file_extension(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].file_extension != NULL ? 
            ESSENCE_TYPE_INFO[essence_type].file_extension : "raw";
}


const char* bmx::essence_type_to_string(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].str;
}

const char* bmx::essence_type_to_enum_string(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].enum_str;
}

EssenceType bmx::get_generic_essence_type(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].generic_essence_type;
}

