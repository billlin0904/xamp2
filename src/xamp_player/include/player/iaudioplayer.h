//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>

#include <output_device/deviceinfo.h>
#include <output_device/iaudiodevicemanager.h>

#include <player/playstate.h>
#include <stream/iaudioprocessor.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

class XAMP_PLAYER_API XAMP_NO_VTABLE IAudioPlayer {
public:
    XAMP_BASE_CLASS(IAudioPlayer)

	virtual void Startup() = 0;

	virtual void Open(Path const& file_path, const Uuid& device_id = Uuid::kInvalidUUID) = 0;

    virtual void Open(Path const& file_path, const DeviceInfo& device_info, uint32_t target_sample_rate = 0, AlignPtr<ISampleRateConverter> converter = nullptr) = 0;

    virtual void PrepareToPlay() = 0;

    virtual void Play() = 0;

    virtual void Pause() = 0;

    virtual void Resume() = 0;

    virtual void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) = 0;

    virtual void Destroy() = 0;

    virtual void Seek(double stream_time) = 0;

    virtual void SetVolume(uint32_t volume) = 0;

    [[nodiscard]] virtual uint32_t GetVolume() const = 0;

    [[nodiscard]] virtual bool IsHardwareControlVolume() const = 0;

    [[nodiscard]] virtual bool IsMute() const = 0;

    virtual void SetMute(bool mute) = 0;

    [[nodiscard]] virtual bool IsPlaying() const noexcept = 0;

    [[nodiscard]] virtual DsdModes GetDsdModes() const noexcept = 0;

    [[nodiscard]] virtual bool IsDSDFile() const = 0;

    [[nodiscard]] virtual std::optional<uint32_t> GetDSDSpeed() const = 0;

    [[nodiscard]] virtual double GetDuration() const = 0;

    [[nodiscard]] virtual PlayerState GetState() const noexcept = 0;

    [[nodiscard]] virtual AudioFormat GetInputFormat() const noexcept = 0;

    [[nodiscard]] virtual AudioFormat GetOutputFormat() const noexcept = 0;

    virtual void SetProcessor(AlignPtr<IAudioProcessor>&& processor) = 0;

    virtual void EnableProcessor(bool enable = true) = 0;

    [[nodiscard]] virtual bool IsEnableProcessor() const = 0;

    [[nodiscard]] virtual bool IsEnableSampleRateConverter() const = 0;

    virtual void SetDevice(const DeviceInfo& device_info) = 0;

    [[nodiscard]] virtual DeviceInfo GetDevice() const = 0;

    virtual const AlignPtr<IAudioDeviceManager>& GetAudioDeviceManager() = 0;

protected:
	IAudioPlayer() = default;
};

}
