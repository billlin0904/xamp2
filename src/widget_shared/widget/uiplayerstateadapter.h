//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <vector>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

#include <base/audioformat.h>

#include <player/iplaybackstateadapter.h>
#include <player/playstate.h>

class XAMP_WIDGET_SHARED_API UIPlayerStateAdapter final
    : public QObject
    , public IPlaybackStateAdapter {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    void outputFormatChanged(const AudioFormat output_format, size_t buffer_size) override;

    int32_t sampleRate() const;

    size_t outputBufferSize() const;

    void onSampleTime(double stream_time) override;

    void onStateChanged(PlayerState play_state) override;

    void onError(const std::exception&ex) override;

    void onDeviceChanged(DeviceState state, const std::string& device_id) override;

    void onVolumeChanged(int32_t vol) override;

    void onSamplesChanged(const float* samples, size_t num_samples) override;

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(const QString &message);

    void deviceChanged(DeviceState state, const QString& message);

    void volumeChanged(float vol);

    void outputFormatChanged(int32_t sample_rate, size_t buffer_size);

    void samplesChanged(std::vector<float> samples, size_t num_samples);

private:
    int32_t sample_rate_;
    size_t output_buffer_size_;
};
