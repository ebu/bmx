/*
 * Copyright (C) 2026, Dolby Laboratories, Inc
 * All Rights Reserved.
 *
 * Reads a BW64 file (Recommendation ITU-R BS.2088) carrying Serial ADM (S-ADM)
 * metadata in the <sxml> chunk and PCM audio in the <data> chunk, and presents
 * the data as an ST 2127-1 MGA frame bitstream so that the existing
 * SADMEssenceParser / MGA sound essence pipeline can consume it unchanged.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DAMAGES ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef BMX_BW64_SADM_ESSENCE_SOURCE_H_
#define BMX_BW64_SADM_ESSENCE_SOURCE_H_

#include <cstdio>
#include <string>
#include <vector>

#include <bmx/essence_parser/EssenceSource.h>


namespace bmx
{


class BW64SADMEssenceSource : public EssenceSource
{
public:
    BW64SADMEssenceSource();
    virtual ~BW64SADMEssenceSource();

    // Parse the BW64 header/chunks and build the MGA frame table.
    bool Open(const std::string &filename);

    // EssenceSource interface. Serves the ST 2127-1 MGA frame bitstream.
    virtual uint32_t Read(unsigned char *data, uint32_t size);
    virtual bool SeekStart();
    virtual bool Skip(int64_t offset);

    virtual bool HaveError() const   { return mErrno != 0; }
    virtual int GetErrno() const     { return mErrno; }
    virtual std::string GetStrError() const { return mStrError; }

    // Audio format from the BW64 <fmt > chunk.
    uint16_t GetChannelCount() const  { return mChannelCount; }
    uint32_t GetSamplingRate() const  { return mSamplingRate; }
    uint16_t GetBitsPerSample() const { return mBitsPerSample; }
    // Number of audio samples per S-ADM frame (from the first SubXMLChunk).
    uint32_t GetSamplesPerFrame() const { return mSamplesPerFrame; }
    uint32_t GetFrameCount() const    { return (uint32_t)mFrames.size(); }

private:
    struct MGAFrame
    {
        int64_t   audio_file_offset;                 // absolute file offset of the audio for this frame
        uint32_t  audio_size;                        // audio bytes for this frame
        std::vector<unsigned char> non_audio_bytes;  // MGA framing: [numSections][audio hdr] + [metadata section]
        uint32_t  audio_insert_pos;                  // offset within non_audio_bytes where audio is spliced in
    };

    bool SetError(const std::string &msg);
    void BuildFrames(uint16_t sxml_fmt_type,
                     const std::vector<std::pair<int64_t, uint32_t> > &sub_xml,  // (file offset, size) of each xmlData
                     const std::vector<uint32_t> &sub_samples);

private:
    FILE *mFile;
    std::string mFilename;
    int mErrno;
    std::string mStrError;

    uint16_t mChannelCount;
    uint32_t mSamplingRate;
    uint16_t mBitsPerSample;
    uint16_t mBlockAlign;
    uint32_t mSamplesPerFrame;

    int64_t mDataChunkOffset;   // absolute file offset of <data> chunk payload
    int64_t mDataChunkSize;

    std::vector<MGAFrame> mFrames;

    // Read cursor over the synthesised MGA bitstream.
    size_t  mCurFrame;
    int64_t mPosInFrame;        // byte position within the current frame (non-audio + audio)
};


};


#endif
