//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IOutputDevice {
public:
	XAMP_BASE_CLASS(IOutputDevice)

    virtual void OpenStream(AudioFormat const & output_format) = 0;

	virtual void SetAudioCallback(IAudioCallback* callback) noexcept = 0;

	[[nodiscard]] virtual bool IsStreamOpen() const noexcept = 0;

	[[nodiscard]] virtual bool IsStreamRunning() const noexcept = 0;

	virtual void StopStream(bool wait_for_stop_stream = true) = 0;

	virtual void CloseStream() = 0;

	virtual void StartStream() = 0;

	virtual void SetStreamTime(double stream_time) noexcept = 0;

	[[nodiscard]] virtual double GetStreamTime() const noexcept = 0;

    [[nodiscard]] virtual uint32_t GetVolume() const = 0;

    virtual void SetVolume(uint32_t volume) const = 0;

	virtual void SetMute(bool mute) const = 0;

	[[nodiscard]] virtual bool IsMuted() const = 0;

	[[nodiscard]] virtual bool IsHardwareControlVolume() const = 0;

	virtual void DisplayControlPanel() = 0;

	[[nodiscard]] virtual PackedFormat GetPackedFormat() const noexcept = 0;

	[[nodiscard]] virtual uint32_t GetBufferSize() const noexcept = 0;

	virtual void AbortStream() noexcept = 0;

protected:
	IOutputDevice() = default;
};

}

