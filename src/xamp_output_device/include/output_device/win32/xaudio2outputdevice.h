//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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
#include <base/threadpool.h>

#include <xaudio2.h>
#include <atomic>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(XAudio2OutputDevice);

class XAudio2OutputDevice final : public IOutputDevice {
public:
	XAudio2OutputDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::wstring &device_id);

	virtual ~XAudio2OutputDevice() override;

	void openStream(const AudioFormat& output_format) override;

	void setAudioCallback(IAudioCallback* callback) override;

	bool isStreamOpen() const override;

	bool isStreamRunning() const override;

	void stopStream(bool wait_for_stop_stream = true) override;

	void closeStream() override;

	void startStream() override;

	void setStreamTime(double stream_time) override;

	double getStreamTime() const override;

	uint32_t getVolume() const override;

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	bool isMuted() const override;

	PackedFormat getPackedFormat() const override;

	uint32_t getBufferSize() const override;

	bool isHardwareControlVolume() const override;

	void abortStream() override;

private:
	void reportError(HRESULT hr) ;

	HRESULT fillSamples(bool& end_of_stream);

	class XAudio2EngineContext;
	class XAudio2VoiceContext;

	std::atomic_bool is_running_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Future<void> render_task_;
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
	std::shared_ptr<IThreadPool> thread_pool_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
