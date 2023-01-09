//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <QObject>

union WaveformData {
    struct {
        uint8_t low;
        uint8_t mid;
        uint8_t high;
        uint8_t all;
    } filtered;

    int32_t val;

    WaveformData(const int32_t i) {
        val = i;
    }
};

class Waveform {
public:
    Waveform(int32_t sample_rate,
        int32_t num_samples,
        int32_t desired_visual_sample_rate,
        int32_t max_visual_sample_rate);

    inline WaveformData& at(int32_t i) {
	    return data_[i];
    }

    inline uint8_t& low(int32_t i) {
	    return data_[i].filtered.low;
    }

    inline uint8_t& mid(int32_t i) {
	    return data_[i].filtered.mid;
    }

    inline uint8_t& high(int32_t i) {
	    return data_[i].filtered.high;
    }

    inline uint8_t& all(int32_t i) {
	    return data_[i].filtered.all;
    }

    inline int32_t textureStride() const {
	    return texture_stride_;
    }

    inline size_t textureStrideSize() const {
        return data_.size();
    }

    const WaveformData* data() const {
	    return &data_[0];
    }

private:
    void Resize(int32_t size, int32_t value);

    int32_t texture_stride_;
    int32_t num_visual_samples_;
    double visual_sample_rate_;
    double visual_ratio_;
    size_t data_size_;
    std::vector<WaveformData> data_;
};

class WaveformAnalyzer {
public:
    class WaveformRenderer;
};

