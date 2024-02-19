//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

#include <base/memory.h>
#include <base/audioformat.h>
#include <stream/stream.h>
#include <stream/stft.h>

#include <player/playstate.h>

class XAMP_WIDGET_SHARED_EXPORT UIPlayerStateAdapter final
    : public QObject
    , public IPlaybackStateAdapter {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    void setBandSize(size_t band_size);

    void OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) override;

    size_t fftSize() const;

    void OnSampleTime(double stream_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(DeviceState state) override;

    void OnVolumeChanged(float vol) override;

    void OnSamplesChanged(const float* samples, size_t num_buffer_frames) override;

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(DeviceState state);

    void volumeChanged(float vol);

    void fftResultChanged(ComplexValarray const& result);

private:
    bool enable_spectrum_;
    size_t band_size_;
    size_t fft_size_;
    double desired_band_width_;
    AlignPtr<STFT> stft_;
};
