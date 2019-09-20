//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API Device {
public:
	virtual ~Device() = default;

	XAMP_DISABLE_COPY(Device)

	virtual void OpenStream(const AudioFormat& output_format) = 0;

	virtual void SetAudioCallback(AudioCallback* callback) = 0;

	virtual bool IsStreamOpen() const = 0;

	virtual bool IsStreamRunning() const = 0;

	virtual void StopStream() = 0;

	virtual void CloseStream() = 0;

	virtual void StartStream() = 0;

	virtual void SetStreamTime(double stream_time) = 0;

	virtual double GetStreamTime() const = 0;

	virtual int32_t GetVolume() const = 0;

	virtual void SetVolume(int32_t volume) const = 0;

	virtual void SetMute(bool mute) const = 0;

	virtual void DisplayControlPanel() = 0;

	virtual InterleavedFormat GetInterleavedFormat() const = 0;

	virtual int32_t GetBufferSize() const = 0;

protected:
	Device() = default;
};

}

