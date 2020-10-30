//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QModelIndex>
#include <deque>

#include <widget/widget_shared.h>

#include <base/threadpool.h>
#include <output_device/devicestatelistener.h>
#include <player/playbackstateadapter.h>

class PlayerStateAdapter final 
    : public QObject
    , public xamp::player::PlaybackStateAdapter {
    Q_OBJECT
public:
    explicit PlayerStateAdapter(QObject *parent = nullptr);

    void addPlaybackQueue(const QModelIndex& next_index, const std::wstring& file_ext, const std::wstring& file_path);

    void clearPlaybackQueue();

    size_t getPlaybackQueueSize() const;

signals:
    void sampleTimeChanged(double stream_time, double sample_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(xamp::output_device::DeviceState state);

    void volumeChanged(float vol);

    void sampleDataChanged(std::vector<float> const &samples);

    void gaplessPlayback(const QModelIndex &next_index);

protected:
	void OnSampleTime(double stream_time, double sample_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(xamp::output_device::DeviceState state) override;

    void OnVolumeChanged(float vol) override;

    void OnSampleDataChanged(const float *samples, size_t size) override;

    void OnGaplessPlayback();

    std::optional<AlignPtr<FileStream>> GetNextGaplessStream();

    BoundedQueue<QModelIndex> next_playindex_;
    BoundedQueue<AlignPtr<FileStream>> next_streams_;
};
