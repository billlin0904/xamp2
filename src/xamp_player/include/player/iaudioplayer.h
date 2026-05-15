//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/playstate.h>
#include <player/player.h>

#include <base/base.h>
#include <base/fs.h>
#include <base/memory.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>
#include <base/uuid.h>
#include <base/archivefile.h>
#include <stream/filestream.h>
#include <stream/iaudioprocessor.h>

#include <optional>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

/*
* IAudioPlayer is an interface that plays audio files. It is responsible for reading audio files,
* 
*/
class XAMP_PLAYER_API XAMP_NO_VTABLE IAudioPlayer {
public:
    XAMP_BASE_CLASS(IAudioPlayer)

	virtual void SetStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) = 0;

    virtual void Destroy() = 0;

	virtual void Open(Path const& file_path,
                    const Uuid& device_id = Uuid::kNullUuid,
                    float rate = 0.0f,
                    bool use_mqa_decode = false) = 0;

    virtual void Open(Path const& file_path,
                      const DeviceInfo& device_info,
                      uint32_t target_sample_rate = 0,
                      DsdModes output_mode = DsdModes::DSD_MODE_AUTO,
                      float rate = 0.0f,
                      bool use_mqa_decode = false) = 0;

    virtual void OpenArchiveEntry(ArchiveEntry archive_entry,
                    const DeviceInfo& device_info,
                    uint32_t target_sample_rate = 0,
                    DsdModes output_mode = DsdModes::DSD_MODE_AUTO,
                    float rate = 0.0f,
                    bool use_mqa_decode = false) = 0;

    virtual void Open(ScopedPtr<FileStream> file_stream,
                      const DeviceInfo& device_info,
                      uint32_t target_sample_rate = 0,
                      DsdModes output_mode = DsdModes::DSD_MODE_AUTO) = 0;

    virtual void PrepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT,
        uint32_t device_sample_rate = 0) = 0;

    virtual void BufferStream(double stream_time = 0.0,
        const std::optional<double>& offset = std::nullopt,
        const std::optional<double>& duration = std::nullopt) = 0;

    virtual void Play() = 0;

    virtual void Pause() = 0;

    virtual void Resume() = 0;
    
    virtual void Stop(bool signal_to_stop = true,
        bool shutdown_device = false,
        bool wait_for_stop_stream = true) = 0;    

    virtual void Seek(double stream_time) = 0;

    virtual void SetVolume(uint32_t volume) = 0;

    [[nodiscard]] virtual uint32_t GetVolume() const = 0;

    [[nodiscard]] virtual bool IsHardwareControlVolume() const = 0;

    [[nodiscard]] virtual bool IsMute() const = 0;

    virtual void SetMute(bool mute) = 0;

    virtual void EnableFadeOut(bool enable) = 0;

    [[nodiscard]] virtual bool IsPlaying() const = 0;

    [[nodiscard]] virtual DsdModes GetDsdModes() const = 0;

    [[nodiscard]] virtual bool IsDsdFile() const = 0;

    [[nodiscard]] virtual std::optional<uint32_t> GetDsdSpeed() const = 0;

    [[nodiscard]] virtual double GetDuration() const = 0;

    [[nodiscard]] virtual PlayerState GetState() const = 0;

    [[nodiscard]] virtual AudioFormat GetInputFormat() const = 0;

    [[nodiscard]] virtual AudioFormat GetOutputFormat() const = 0;

    [[nodiscard]] virtual uint32_t GetBitRate() const = 0;

    virtual const ScopedPtr<IAudioDeviceManager>& GetAudioDeviceManager() = 0;

    virtual ScopedPtr<IDSPManager>& GetDspManager() = 0;

    virtual Property& GetDspConfig() = 0;

    virtual void SetDelayCallback(std::function<void(uint32_t)>&& delay_callback) = 0;

    virtual void SeFileCacheMode(bool enable) = 0;

    virtual bool IsMQA() const = 0;
protected:
	IAudioPlayer() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
