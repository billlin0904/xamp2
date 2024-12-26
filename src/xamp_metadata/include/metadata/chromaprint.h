//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>
#include <vector>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API Chromaprint final {
public:
    explicit Chromaprint();

    XAMP_PIMPL(Chromaprint)

    void SetSampleRate(uint32_t sample_rate);

    int32_t Process(int16_t const* data, uint32_t size) const;

    int32_t Finish() const;

    std::vector<uint8_t> GetFingerprint() const;
private:
    class ChromaprintImpl;
    ScopedPtr<ChromaprintImpl> impl_;
};

XAMP_METADATA_NAMESPACE_END