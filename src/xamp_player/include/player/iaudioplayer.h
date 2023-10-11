//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/playstate.h>
#include <player/player.h>

#include <base/base.h>
#include <base/fs.h>
#include <base/align_ptr.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>
#include <base/uuid.h>

#include <stream/iaudioprocessor.h>

#include <optional>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

/*
* IAudioPlayer is a interface that plays audio files. It is responsible for reading audio files,
* 
*/
class XAMP_PLAYER_API XAMP_NO_VTABLE IAudioPlayer {
public:
    XAMP_BASE_CLASS(IAudioPlayer)

    /*
    * Startup the player.
    * 
    * @param[in] adapter The playback state adapter.
    */
	virtual void Startup(const std::weak_ptr<IPlaybackStateAdapter>& adapter) = 0;

    /*
    * Startup the player.
    *
    * @param[in] adapter The playback state adapter.
    */
    virtual void Destroy() = 0;

    /*
    * Open a file.
    *
    * @param[in] file_path The file path.
    * @param[in] device_id The device id.
    */
	virtual void Open(Path const& file_path, const Uuid& device_id = Uuid::kNullUuid) = 0;

    /*
    * Open a file.
    *
    * @param[in] file_path The file path.
    * @param[in] device_info The device info.
    * @param[in] target_sample_rate The target sample rate.
    */
    virtual void Open(Path const& file_path,
        const DeviceInfo& device_info, 
        uint32_t target_sample_rate = 0, 
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) = 0;

    /*
    * Prepare to play.
    *
    * @param[in] byte_format The byte format.
    * @param[in] device_sample_rate The device sample rate.
    * @param[in] dsd_mode The dsd mode.
    * @param[in] dsd_speed The dsd speed.
    */
    virtual void PrepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT, uint32_t device_sample_rate = 0) = 0;

    /*
    * Set read sample size.
    *
    * @param[in] num_samples The num samples.
    */
    virtual void SetReadSampleSize(uint32_t num_samples) = 0;

    /*
    * Buffer stream.
    *
    * @param[in] stream_time The stream time.
    */
    virtual void BufferStream(double stream_time = 0.0) = 0;

    /*
    * Play.
    *
    */
    virtual void Play() = 0;

    /*
    * Pause.
    *
    */
    virtual void Pause() = 0;

    /*
    * Resume.
    *
    */
    virtual void Resume() = 0;
    
    /*
    * Stop.
    *
    * @param[in] signal_to_stop The signal to stop.
    * @param[in] shutdown_device The shutdown device.
    * @param[in] wait_for_stop_stream The wait for stop stream.
    */
    virtual void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) = 0;    

    /*
    * Seek.
    *
    * @param[in] stream_time The stream time.
    */
    virtual void Seek(double stream_time) = 0;

    /*
    * Set volume.
    *
    * @param[in] volume The volume.
    */
    virtual void SetVolume(uint32_t volume) = 0;

    /*
    * Get volume.
    *
    * @return The volume.
    */
    [[nodiscard]] virtual uint32_t GetVolume() const = 0;

    /*
    * Is hardware control volume.
    *
    * @return True if hardware control volume.
    */
    [[nodiscard]] virtual bool IsHardwareControlVolume() const = 0;

    /*
    * Is mute.
    *
    * @return True if mute.
    */
    [[nodiscard]] virtual bool IsMute() const = 0;

    /*
    * Set mute.
    *
    * @param[in] mute The mute.
    */
    virtual void SetMute(bool mute) = 0;

    /*
    * Enable fade out.
    *
    * @param[in] enable The enable.
    */
    virtual void EnableFadeOut(bool enable) = 0;

    /*
    * Is playing.
    *
    * @return True if playing.
    */
    [[nodiscard]] virtual bool IsPlaying() const noexcept = 0;

    /*
    * Get dsd modes.
    *
    * @return The dsd modes.
    */
    [[nodiscard]] virtual DsdModes GetDsdModes() const noexcept = 0;

    /*
    * Is dsd file.
    *
    * @return True if dsd file.
    */
    [[nodiscard]] virtual bool IsDsdFile() const = 0;

    /*
    * Get dsd speed.
    *
    * @return The dsd speed.
    */
    [[nodiscard]] virtual std::optional<uint32_t> GetDsdSpeed() const = 0;

    /*
    * Get media duration.
    *
    * @return The media duration.
    */
    [[nodiscard]] virtual double GetDuration() const = 0;

    /*
    * Get player state.
    *
    * @return The player state.
    */
    [[nodiscard]] virtual PlayerState GetState() const noexcept = 0;

    /*
    * Get input format.
    *
    * @return The input format.
    */
    [[nodiscard]] virtual AudioFormat GetInputFormat() const noexcept = 0;

    /*
    * Get output format.
    *
    * @return The output format.
    */
    [[nodiscard]] virtual AudioFormat GetOutputFormat() const noexcept = 0;

    /*
    * Get audio device manager.
    *
    * @return The audio device manager.
    */
    virtual const AlignPtr<IAudioDeviceManager>& GetAudioDeviceManager() = 0;

    /*
    * Get dsp manager.
    *
    * @return The dsp manager.
    */
    virtual AlignPtr<IDSPManager>& GetDspManager() = 0;

    /*
    * Get dsp config.
    *
    * @return The dsp config.
    */
    virtual AnyMap& GetDspConfig() = 0;
protected:
    /*
    * Constructor.
    */
	IAudioPlayer() = default;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
