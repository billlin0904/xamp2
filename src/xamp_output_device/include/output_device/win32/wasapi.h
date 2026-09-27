//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <cstdint>
#include <string_view>

#include <output_device/deviceinfo.h>

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <atlcomcli.h>

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_BEGIN

XAMP_ALWAYS_INLINE constexpr double nano100ToMillis(REFERENCE_TIME ref) {
	constexpr double ratio = 0.0001;
	return (static_cast<double>(ref) * ratio);
}

XAMP_ALWAYS_INLINE constexpr UINT32 referenceTimeToFrames(REFERENCE_TIME period, UINT32 samplerate) {
	return static_cast<UINT32>(
		1.0 * period * // hns *
		samplerate / // (frames / s) /
		1000 / // (ms / s) /
		10000 // (hns / s) /
		+ 0.5 // rounding
		);
}

XAMP_ALWAYS_INLINE constexpr REFERENCE_TIME makeHnsPeriod(UINT32 frames, UINT32 samplerate) {
	return static_cast<REFERENCE_TIME>(10000.0 * 1000.0 / static_cast<double>(samplerate) * static_cast<double>(frames) + 0.5);
}

DeviceInfo getDeviceInfo(CComPtr<IMMDevice>& device, const Uuid& device_type_id, std::string_view desc);

CComPtr<IMMDeviceEnumerator> createDeviceEnumerator();

double getStreamPosInMilliseconds(CComPtr<IAudioClock>& clock);

AudioFormat toAudioFormat(const WAVEFORMATEX* format);

bool isDeviceSupportExclusiveMode(const CComPtr<IMMDevice>& device, AudioFormat& default_format);

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END

#endif
