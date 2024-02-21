//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <output_device/output_device.h>

#include <output_device/deviceinfo.h>
#include <output_device/win32/mmcss_types.h>

#include <base/stl.h>
#include <base/uuid.h>

#include <atlcomcli.h>
#include <mmdeviceapi.h>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mfapi.h>
#include <strmif.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#include <avrt.h>
#include <Mferror.h>

#include <chrono>

struct IMMDevice;
struct IMFAsyncResult;
struct IAudioClient;
struct IAudioRenderClient;
struct IMMNotificationClient;
struct IMMDeviceEnumerator;

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_BEGIN

inline constexpr int32_t kWasapiReftimesPerMillisec = 10000;
inline constexpr double kWasapiReftimesPerSec = 10000000;
inline constexpr auto kMicrosecondsPerSecond =  1000000;

using ComString = CComHeapPtr<wchar_t>;

/*
* Convert reference time to seconds.
* 
* @param[in] ref: reference time
* @return seconds
*/
XAMP_ALWAYS_INLINE constexpr double Nano100ToSeconds(REFERENCE_TIME ref) noexcept {
	//  1 nano = 0.000000001 seconds
	//100 nano = 0.0000001   seconds
	//100 nano = 0.0001   milliseconds
	constexpr double ratio = 0.0000001;
	return (static_cast<double>(ref) * ratio);
}

/*
* Convert reference time to milliseconds.
* 
* @param[in] ref: reference time
* @return milliseconds
*/
XAMP_ALWAYS_INLINE constexpr double Nano100ToMillis(REFERENCE_TIME ref) noexcept {
	constexpr double ratio = 0.0001;
	return (static_cast<double>(ref) * ratio);
}

/*
* Convert reference time to frames
* 
* @param period: period
* @param samplerate: samplerate
* @return frames
*/
XAMP_ALWAYS_INLINE constexpr UINT32 ReferenceTimeToFrames(REFERENCE_TIME period, UINT32 samplerate) noexcept {
	return static_cast<UINT32>(
		1.0 * period * // hns *
		samplerate / // (frames / s) /
		1000 / // (ms / s) /
		10000 // (hns / s) /
		+ 0.5 // rounding
		);
}

/*
* Convert frames to reference time.
* 
* @param[in] frames: frames
* @param[in] samplerate: samplerate
* @return reference time
*/
XAMP_ALWAYS_INLINE constexpr REFERENCE_TIME MakeHnsPeriod(UINT32 frames, UINT32 samplerate) noexcept {
	return static_cast<REFERENCE_TIME>(10000.0 * 1000.0 / static_cast<double>(samplerate) * static_cast<double>(frames) + 0.5);
}

/*
* Convert milliseconds to reference time.
* 
* @param[in] ms: milliseconds
* @return reference time
*/
XAMP_ALWAYS_INLINE constexpr REFERENCE_TIME MsToPeriod(uint32_t ms) noexcept {
	return ms * 10000;
}

/*
* Get device info.
* 
* @param[in] device: IMMDevice
* @param[in] device_type_id: device type id
* @return DeviceInfo
*/
DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, const Uuid& device_type_id);

/*
* Create device enumerator.
* 
* @return IMMDeviceEnumerator
*/
CComPtr<IMMDeviceEnumerator> CreateDeviceEnumerator();

/*
* Get stream position in milliseconds.
* 
* @param[in] clock: IAudioClock
*/
double GetStreamPosInMilliseconds(CComPtr<IAudioClock>& clock);

AudioFormat ToAudioFormat(const WAVEFORMATEX* format);

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END

#endif
