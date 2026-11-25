/*
 * Copyright (C) 2024, British Broadcasting Corporation
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

#ifndef BMX_TIMED_DATA_MANIFEST_H_
#define BMX_TIMED_DATA_MANIFEST_H_

#include <string>
#include <vector>

#include <bmx/BMXTypes.h>


namespace bmx
{


class TimedDataAncillaryResource
{
public:
    TimedDataAncillaryResource();
    TimedDataAncillaryResource(const TimedDataAncillaryResource &from);
    virtual ~TimedDataAncillaryResource();

    virtual TimedDataAncillaryResource* Clone() const = 0;

public:
    std::string filename;
    std::string mime_type;
    uint32_t stream_id;
};


class TimedDataManifest
{
public:
    TimedDataManifest();
    TimedDataManifest(const TimedDataManifest &from);
    virtual ~TimedDataManifest();

    std::string GetFilename() const { return mFilename; }
    int64_t GetStart() const { return mStart; }

    const std::vector<TimedDataAncillaryResource*>& GetAncillaryResources() const { return mAncillaryResources; }

public:
    virtual void Reset();

    virtual TimedDataManifest* Clone() const = 0;

public:
    std::string mFilename;
    int64_t mStart;
    std::vector<TimedDataAncillaryResource*> mAncillaryResources;
};


};

#endif
