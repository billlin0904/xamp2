//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/resampler.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class XAMP_PLAYER_API NullResampler final : public Resampler {
public:
    explicit NullResampler(DsdModes dsd_mode, uint32_t sample_size)
		: dsd_mode_(dsd_mode)
		, sample_size_(sample_size)
		, process_(nullptr) {
		if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
			process_ = &NullResampler::ProcessNativeDsd;
		}
		else {
			process_ = &NullResampler::ProcessPcm;
		}
	}

    void Start(uint32_t, uint32_t, uint32_t, uint32_t) override {
	}

    bool Process(const float* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) override {
		return (*this.*process_)(reinterpret_cast<const int8_t*>(sample_buffer), num_samples, buffer);
	}

	std::string_view GetDescription() const noexcept override {
		return "None";
	}

	void Flush() override {
	}
private:
#define IsBufferTooSmallThrow(expr) \
    do {\
        if (!(expr)) {\
        throw Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "Buffer overflow!");\
        }\
        return true;\
    } while(false)

    bool ProcessNativeDsd(const int8_t* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
        IsBufferTooSmallThrow(buffer.TryWrite(sample_buffer, num_samples));
	}

    bool ProcessPcm(const int8_t* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
        IsBufferTooSmallThrow(buffer.TryWrite(sample_buffer, num_samples * sample_size_));
	}

	typedef bool (NullResampler::*ProcessDispatch)(const int8_t* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer);
	DsdModes dsd_mode_;
    uint32_t sample_size_;
	ProcessDispatch process_;
};

}

