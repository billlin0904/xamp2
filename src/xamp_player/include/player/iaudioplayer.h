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
#include <stream/eqsettings.h>

#include <optional>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

/*
* IAudioPlayer is an interface that plays audio files. It is responsible for reading audio files,
* 
*/
class XAMP_PLAYER_API XAMP_NO_VTABLE IAudioPlayer {
public:
    XAMP_BASE_CLASS(IAudioPlayer)

	virtual void setStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) = 0;

    virtual void destroy() = 0;

    virtual void openArchiveEntry(ArchiveEntry archive_entry,
                    const DeviceInfo& device_info,
                    uint32_t target_sample_rate = 0,
                    DsdModes output_mode = DsdModes::DSD_MODE_AUTO) = 0;

    virtual void open(ScopedPtr<FileStream> file_stream,
                      const DeviceInfo& device_info,
                      uint32_t target_sample_rate = 0,
                      DsdModes output_mode = DsdModes::DSD_MODE_AUTO) = 0;

    virtual void prepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT,
        uint32_t device_sample_rate = 0) = 0;

    virtual void bufferStream(double stream_time = 0.0,
        const std::optional<double>& offset = std::nullopt,
        const std::optional<double>& duration = std::nullopt) = 0;

    virtual void play() = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;
    
    virtual void stop(bool signal_to_stop = true,
        bool shutdown_device = false,
        bool wait_for_stop_stream = true) = 0;    

    virtual void seek(double stream_time) = 0;

    virtual void setParametricEq(bool enabled, const EqSettings& settings) = 0;

    virtual void setVolume(uint32_t volume) = 0;

    [[nodiscard]] virtual uint32_t getVolume() const = 0;

    [[nodiscard]] virtual bool isHardwareControlVolume() const = 0;

    [[nodiscard]] virtual bool isMute() const = 0;

    virtual void setMute(bool mute) = 0;

    [[nodiscard]] virtual bool isPlaying() const = 0;

    [[nodiscard]] virtual DsdModes getDsdModes() const = 0;

    [[nodiscard]] virtual bool isDsdFile() const = 0;

    [[nodiscard]] virtual std::optional<uint32_t> getDsdSpeed() const = 0;

    [[nodiscard]] virtual double getDuration() const = 0;

    [[nodiscard]] virtual PlayerState getState() const = 0;

    [[nodiscard]] virtual AudioFormat getInputFormat() const = 0;

    [[nodiscard]] virtual AudioFormat getOutputFormat() const = 0;

    [[nodiscard]] virtual uint32_t getBitRate() const = 0;

    virtual const ScopedPtr<IAudioDeviceManager>& getAudioDeviceManager() = 0;

    virtual ScopedPtr<IDSPManager>& getDspManager() = 0;

    virtual Property& getDspConfig() = 0;

protected:
	IAudioPlayer() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
