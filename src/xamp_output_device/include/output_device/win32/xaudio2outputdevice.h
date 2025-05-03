//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>

#include <base/logger.h>
#include <base/dataconverter.h>
#include <base/buffer.h>
#include <base/task.h>
#include <base/platfrom_handle.h>
#include <base/threadpoolexecutor.h>

#include <xaudio2.h>
#include <atomic>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(XAudio2OutputDevice);

/*
* XAudio2OutputDevice is the XAudio2 device.
*
*/
class XAudio2OutputDevice final : public IOutputDevice {
public:
	/*
	* Constructor.
	*
	* @param device: device
	*/
	XAudio2OutputDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::wstring &device_id);

	/*
	* Destructor.
	*/
	virtual ~XAudio2OutputDevice() override;

	/*
	* Open stream.
	*
	* @param output_format: output format
	* @return void
	*/
	void OpenStream(const AudioFormat& output_format) override;

	/*
	* Set audio callback.
	*
	* @param callback: audio callback
	*/
	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	/*
	* Is stream open.
	*
	* return bool
	*/
	bool IsStreamOpen() const noexcept override;

	/*
	* Is stream running.
	*
	* @return bool
	*/
	bool IsStreamRunning() const noexcept override;

	/*
	* Stop stream.
	*
	* @param[in] wait_for_stop_stream: wait for stop stream
	*/
	void StopStream(bool wait_for_stop_stream = true) override;

	/*
	* Close stream.
	*
	*/
	void CloseStream() override;

	/*
	* Start stream.
	*
	*/
	void StartStream() override;

	/*
	* Set stream time.
	*
	* @param stream_time: stream time
	*/
	void SetStreamTime(double stream_time) noexcept override;

	/*
	* Get stream time.
	*
	*/
	double GetStreamTime() const noexcept override;

	/*
	* Get volume.
	*
	* @return uint32_t
	*/
	uint32_t GetVolume() const override;

	/*
	* Set volume.
	* @param volume: volume (1~100)
	*/
	void SetVolume(uint32_t volume) const override;

	/*
	* Set mute.
	*
	* @param mute: mute (true/false)
	*/
	void SetMute(bool mute) const override;

	/*
	* Is muted.
	*
	* @return bool
	*/
	bool IsMuted() const override;

	/*
	* Get packed format.
	*
	* @return PackedFormat
	*/
	PackedFormat GetPackedFormat() const noexcept override;

	/*
	* Get device buffer size.
	*
	* @return uint32_t
	*/
	uint32_t GetBufferSize() const noexcept override;

	/*
	* Is hardware control volume.
	*
	* @return bool
	*/
	bool IsHardwareControlVolume() const override;

	/*
	* Abort stream.
	*/
	void AbortStream() noexcept override;

private:
	/*
	* Report error
	*
	* @param hr: HRESULT
	*/
	void ReportError(HRESULT hr) noexcept;

	HRESULT FillSamples(bool& end_of_stream);

	class XAudio2EngineContext;
	class XAudio2VoiceContext;

	bool is_running_;
	bool is_playing_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Task<void> render_task_;
	AudioFormat output_format_;
	Buffer<float> buffer_;
	std::wstring device_id_;
	WinHandle thread_start_;
	WinHandle thread_exit_;
	WinHandle close_request_;
	IXAudio2MasteringVoice* mastering_voice_;
	mutable IXAudio2SourceVoice* source_voice_;
	ScopedPtr<XAudio2EngineContext> engine_context_;
	ScopedPtr<XAudio2VoiceContext> voice_context_;
	CComPtr<IXAudio2> xaudio2_;
	LoggerPtr logger_;
	FastMutex mutex_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
