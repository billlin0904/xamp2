//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <stdexcept>

#include <output_device/output_device.h>
#include <base/audioformat.h>
#include <base/pcm.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IOutputDevice {
public:
	static constexpr auto kWaitStreamStartTimeout = std::chrono::milliseconds(60 * 1000);

	XAMP_BASE_CLASS(IOutputDevice)
	
    virtual void setIntegerPcmFormat(const xamp::pcm::Format&) {
        throw std::runtime_error("Output does not support integer BitPerfect PCM");
    }
    virtual void setBitPerfect(bool enabled) {
        if (enabled) throw std::runtime_error("BitPerfect requires WASAPI Exclusive or integer ASIO output");
    }

    virtual void openStream(const AudioFormat & output_format) = 0;

	virtual void setAudioCallback(IAudioCallback* callback) = 0;

	[[nodiscard]] virtual bool isStreamOpen() const = 0;

	[[nodiscard]] virtual bool isStreamRunning() const = 0;

	virtual void stopStream(bool wait_for_stop_stream = true) = 0;

	virtual void closeStream() = 0;

	virtual void startStream() = 0;

	virtual void setStreamTime(double stream_time) = 0;

	[[nodiscard]] virtual double getStreamTime() const = 0;

    [[nodiscard]] virtual uint32_t getVolume() const = 0;

    virtual void setVolume(uint32_t volume) const = 0;

	virtual void setMute(bool mute) const = 0;

	[[nodiscard]] virtual bool isMuted() const = 0;

	[[nodiscard]] virtual bool isHardwareControlVolume() const = 0;

	[[nodiscard]] virtual PackedFormat getPackedFormat() const = 0;

	[[nodiscard]] virtual uint32_t getBufferSize() const = 0;
	
	virtual void abortStream() = 0;

protected:
	IOutputDevice() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
