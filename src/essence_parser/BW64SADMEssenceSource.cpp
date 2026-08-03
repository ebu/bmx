/*
 * Copyright (C) 2026, Dolby Laboratories, Inc
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <cstring>
#include <cerrno>

#include <bmx/essence_parser/BW64SADMEssenceSource.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


// S-ADM metadata payload format values (ST 2127-10 Table 2), matching the BW64
// <sxml> fmtType (BS.2088 §7): 0 = uncompressed XML, 1 = gzip.
#define SADM_FORMAT_XML   0x00
#define SADM_FORMAT_GZIP  0x01

// ST 2127-1 metadata section: single S-ADM payload beginning with a Tag.
#define SADM_PAYLOAD_TAG  0x12


static uint16_t get_uint16_le(const unsigned char *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get_uint32_le(const unsigned char *p)
{
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

static uint64_t get_uint64_le(const unsigned char *p)
{
    return (uint64_t)get_uint32_le(p) | ((uint64_t)get_uint32_le(p + 4) << 32);
}

// Encode a length using the BER definite form (ISO/IEC 8824-1), as used by the
// ST 2127-1 metadata payload.
static void append_ber_length(vector<unsigned char> &out, uint32_t length)
{
    if (length < 128) {
        out.push_back((unsigned char)length);
        return;
    }
    unsigned char bytes[4];
    int n = 0;
    uint32_t v = length;
    while (v) {
        bytes[n++] = (unsigned char)(v & 0xff);
        v >>= 8;
    }
    out.push_back((unsigned char)(0x80 | n));
    for (int i = n - 1; i >= 0; i--)
        out.push_back(bytes[i]);
}

static void append_uint32_be(vector<unsigned char> &out, uint32_t v)
{
    out.push_back((unsigned char)((v >> 24) & 0xff));
    out.push_back((unsigned char)((v >> 16) & 0xff));
    out.push_back((unsigned char)((v >> 8) & 0xff));
    out.push_back((unsigned char)(v & 0xff));
}


BW64SADMEssenceSource::BW64SADMEssenceSource()
{
    mFile = 0;
    mErrno = 0;
    mChannelCount = 0;
    mSamplingRate = 0;
    mBitsPerSample = 0;
    mBlockAlign = 0;
    mSamplesPerFrame = 0;
    mDataChunkOffset = 0;
    mDataChunkSize = 0;
    mCurFrame = 0;
    mPosInFrame = 0;
}

BW64SADMEssenceSource::~BW64SADMEssenceSource()
{
    if (mFile)
        fclose(mFile);
}

bool BW64SADMEssenceSource::SetError(const string &msg)
{
    mErrno = -1;
    mStrError = msg;
    log_error("BW64 S-ADM: %s\n", msg.c_str());
    return false;
}

bool BW64SADMEssenceSource::Open(const string &filename)
{
    mFilename = filename;
    mFile = fopen(filename.c_str(), "rb");
    if (!mFile) {
        mErrno = errno;
        mStrError = strerror(errno);
        return false;
    }

    // RIFF / RF64 / BW64 top-level header
    unsigned char hdr[12];
    if (fread(hdr, 1, 12, mFile) != 12)
        return SetError("failed to read RIFF header");
    if (memcmp(hdr, "RIFF", 4) != 0 && memcmp(hdr, "RF64", 4) != 0 && memcmp(hdr, "BW64", 4) != 0)
        return SetError("not a RIFF/RF64/BW64 file");
    if (memcmp(hdr + 8, "WAVE", 4) != 0)
        return SetError("not a WAVE file");

    int64_t ds64_data_size = -1;
    bool have_fmt = false;
    int64_t sxml_offset = -1;
    int64_t sxml_size = -1;

    // Walk the chunks.
    for (;;) {
        unsigned char ckhdr[8];
        size_t n = fread(ckhdr, 1, 8, mFile);
        if (n != 8)
            break;  // end of file
        char ckid[5] = {0};
        memcpy(ckid, ckhdr, 4);
        int64_t cksize = (int64_t)get_uint32_le(ckhdr + 4);
        int64_t payload_offset = ftello(mFile);

        if (memcmp(ckid, "ds64", 4) == 0) {
            vector<unsigned char> ds64(cksize);
            if (cksize < 24 || fread(&ds64[0], 1, (size_t)cksize, mFile) != (size_t)cksize)
                return SetError("failed to read ds64 chunk");
            ds64_data_size = (int64_t)get_uint64_le(&ds64[8]);  // dataSize (bytes 8..15)
        } else if (memcmp(ckid, "fmt ", 4) == 0) {
            unsigned char fmt[16];
            if (cksize < 16 || fread(fmt, 1, 16, mFile) != 16)
                return SetError("failed to read fmt chunk");
            // WAVEFORMAT: audioFormat(2), channels(2), sampleRate(4), byteRate(4),
            // blockAlign(2), bitsPerSample(2)
            mChannelCount  = get_uint16_le(fmt + 2);
            mSamplingRate  = get_uint32_le(fmt + 4);
            mBlockAlign    = get_uint16_le(fmt + 12);
            mBitsPerSample = get_uint16_le(fmt + 14);
            have_fmt = true;
        } else if (memcmp(ckid, "data", 4) == 0) {
            int64_t data_size = cksize;
            if (cksize == 0xffffffff) {
                if (ds64_data_size < 0)
                    return SetError("data chunk uses 64-bit size but ds64 is missing");
                data_size = ds64_data_size;
            }
            mDataChunkOffset = payload_offset;
            mDataChunkSize = data_size;
            cksize = data_size;  // for advancing
        } else if (memcmp(ckid, "sxml", 4) == 0) {
            sxml_offset = payload_offset;
            sxml_size = cksize;
        }

        // Advance to next chunk (chunks are word-aligned).
        int64_t adv = cksize + (cksize & 1);
        if (fseeko(mFile, payload_offset + adv, SEEK_SET) != 0)
            break;
    }

    if (!have_fmt)
        return SetError("missing fmt chunk");
    if (mDataChunkOffset == 0)
        return SetError("missing data chunk");
    if (sxml_offset < 0)
        return SetError("missing sxml chunk (no S-ADM metadata)");
    if (mBlockAlign == 0)
        return SetError("invalid block align in fmt chunk");

    // Read the whole sxml chunk payload into memory (metadata is small).
    vector<unsigned char> sxml((size_t)sxml_size);
    if (fseeko(mFile, sxml_offset, SEEK_SET) != 0 ||
        fread(&sxml[0], 1, (size_t)sxml_size, mFile) != (size_t)sxml_size)
    {
        return SetError("failed to read sxml chunk");
    }

    // Parse sxml header (BS.2088 §7).
    if (sxml_size < 14)
        return SetError("sxml chunk too small");
    uint16_t fmt_type = get_uint16_le(&sxml[0]);
    if (fmt_type != SADM_FORMAT_XML && fmt_type != SADM_FORMAT_GZIP)
        return SetError("unsupported sxml fmtType (only 0=uncompressed and 1=gzip supported)");
    uint32_t num_sub = get_uint32_le(&sxml[10]);

    vector<pair<int64_t, uint32_t> > sub_xml;  // (offset within file, size) of each xmlData
    vector<uint32_t> sub_samples;
    // Each SubXMLChunk is: subXMLChunkSize(4) + nSamplesSubDataChunk(4) + xmlData[subXMLChunkSize].
    // subXMLChunkSize is the size of xmlData only (it excludes the 8-byte header).
    size_t pos = 14;
    for (uint32_t i = 0; i < num_sub; i++) {
        if (pos + 8 > sxml.size())
            return SetError("truncated sxml SubXMLChunk table");
        uint32_t sub_size     = get_uint32_le(&sxml[pos]);       // size of xmlData
        uint32_t sub_samples_ = get_uint32_le(&sxml[pos + 4]);
        if (pos + 8 + (size_t)sub_size > sxml.size())
            return SetError("invalid SubXMLChunk size");
        // Offset of xmlData relative to the start of the in-memory sxml buffer.
        sub_xml.push_back(make_pair((int64_t)(pos + 8), sub_size));
        sub_samples.push_back(sub_samples_);
        pos += 8 + (size_t)sub_size;
    }
    if (sub_xml.empty())
        return SetError("sxml chunk contains no SubXMLChunks");

    mSamplesPerFrame = sub_samples[0];

    // Build the per-frame MGA framing, splicing the sxml payload straight into
    // the metadata section (preserving its compression via the format byte).
    int64_t running_audio = 0;
    for (size_t i = 0; i < sub_xml.size(); i++) {
        MGAFrame frame;
        uint32_t audio_size = sub_samples[i] * mBlockAlign;
        frame.audio_file_offset = mDataChunkOffset + running_audio;
        frame.audio_size = audio_size;
        running_audio += audio_size;

        const unsigned char *xml_data = &sxml[(size_t)sub_xml[i].first];
        uint32_t xml_size = sub_xml[i].second;

        // Metadata payload = version(0) + format + xmlData
        // payloadLen (BER-encoded) counts version+format+xmlData.
        uint32_t payload_len = 2 + xml_size;
        vector<unsigned char> ber;
        append_ber_length(ber, payload_len);
        // metadata section value length = tag(1) + BER + payload
        uint32_t meta_value_len = 1 + (uint32_t)ber.size() + payload_len;

        vector<unsigned char> &out = frame.non_audio_bytes;
        // Frame prefix: number of sections + audio section header (ILV, value spliced later)
        out.push_back(0x02);                 // number of sections
        out.push_back(0x00);                 // audio section index
        out.push_back(0x00);                 // audio section identifier (Audio Essence)
        append_uint32_be(out, audio_size);   // audio section length
        frame.audio_insert_pos = (uint32_t)out.size();  // audio spliced in here
        // Metadata section (follows the audio essence in the byte stream)
        out.push_back(0x01);                 // metadata section index
        out.push_back(0x02);                 // identifier: single payload beginning with Tag
        append_uint32_be(out, meta_value_len);
        out.push_back(SADM_PAYLOAD_TAG);     // 0x12
        out.insert(out.end(), ber.begin(), ber.end());
        out.push_back(0x00);                 // version
        out.push_back((unsigned char)fmt_type);  // format (0=xml, 1=gzip) - preserved
        out.insert(out.end(), xml_data, xml_data + xml_size);

        mFrames.push_back(frame);
    }

    if (running_audio > mDataChunkSize) {
        return SetError("sxml sample counts exceed the data chunk size");
    }

    log_info("BW64 S-ADM: %u channels, %u Hz, %u-bit, %u frames, %u samples/frame, %s S-ADM\n",
             mChannelCount, mSamplingRate, mBitsPerSample,
             (uint32_t)mFrames.size(), mSamplesPerFrame,
             fmt_type == SADM_FORMAT_GZIP ? "gzip" : "uncompressed");

    return true;
}

uint32_t BW64SADMEssenceSource::Read(unsigned char *data, uint32_t size)
{
    uint32_t total_read = 0;
    while (total_read < size && mCurFrame < mFrames.size()) {
        MGAFrame &f = mFrames[mCurFrame];
        int64_t frame_total = (int64_t)f.non_audio_bytes.size() + (int64_t)f.audio_size;

        while (mPosInFrame < frame_total && total_read < size) {
            uint32_t want = size - total_read;
            if (mPosInFrame < (int64_t)f.audio_insert_pos) {
                // Frame prefix (number of sections + audio header)
                uint32_t avail = f.audio_insert_pos - (uint32_t)mPosInFrame;
                uint32_t n = (want < avail) ? want : avail;
                memcpy(data + total_read, &f.non_audio_bytes[(size_t)mPosInFrame], n);
                total_read += n;
                mPosInFrame += n;
            } else if (mPosInFrame < (int64_t)f.audio_insert_pos + (int64_t)f.audio_size) {
                // Audio essence, streamed from the <data> chunk
                int64_t audio_pos = mPosInFrame - (int64_t)f.audio_insert_pos;
                uint32_t avail = f.audio_size - (uint32_t)audio_pos;
                uint32_t n = (want < avail) ? want : avail;
                if (fseeko(mFile, f.audio_file_offset + audio_pos, SEEK_SET) != 0) {
                    SetError("failed to seek audio data");
                    return total_read;
                }
                size_t rn = fread(data + total_read, 1, n, mFile);
                if (rn != n) {
                    SetError("failed to read audio data");
                    return total_read + (uint32_t)rn;
                }
                total_read += n;
                mPosInFrame += n;
            } else {
                // Metadata section (stored after the audio-insert point in non_audio_bytes)
                int64_t meta_off = mPosInFrame - (int64_t)f.audio_size;  // index into non_audio_bytes
                uint32_t avail = (uint32_t)(f.non_audio_bytes.size() - (size_t)meta_off);
                uint32_t n = (want < avail) ? want : avail;
                memcpy(data + total_read, &f.non_audio_bytes[(size_t)meta_off], n);
                total_read += n;
                mPosInFrame += n;
            }
        }

        if (mPosInFrame >= frame_total) {
            mCurFrame++;
            mPosInFrame = 0;
        }
    }

    return total_read;
}

bool BW64SADMEssenceSource::SeekStart()
{
    mCurFrame = 0;
    mPosInFrame = 0;
    return true;
}

bool BW64SADMEssenceSource::Skip(int64_t offset)
{
    // Read and discard 'offset' bytes of the synthesised MGA stream.
    unsigned char buffer[8192];
    while (offset > 0) {
        uint32_t chunk = (offset < (int64_t)sizeof(buffer)) ? (uint32_t)offset : (uint32_t)sizeof(buffer);
        uint32_t got = Read(buffer, chunk);
        if (got == 0)
            return false;
        offset -= got;
    }
    return true;
}
