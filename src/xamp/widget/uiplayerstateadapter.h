//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QModelIndex>

#include <base/spsc_queue.h>
#include <output_device/devicestatelistener.h>
#include <stream/filestream.h>
#include <player/playbackstateadapter.h>
#include <player/playstate.h>

using xamp::player::PlayerState;
using xamp::base::Errors;
using xamp::base::AlignPtr;
using xamp::base::Exception;
using xamp::base::SpscQueue;
using xamp::stream::FileStream;
using PlaybackStateAdapterBase = xamp::player::PlaybackStateAdapter;

class UIPlayerStateAdapter final
    : public QObject
    , public PlaybackStateAdapterBase {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    size_t GetPlayQueueSize() const override;

    AlignPtr<FileStream> PopPlayQueue() override;

    void OnSampleTime(double stream_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(xamp::output_device::DeviceState state) override;

    void OnVolumeChanged(float vol) override;

    void OnSampleDataChanged(const float *samples, size_t size) override;

    void OnGaplessPlayback() override;

    void addPlayQueue(const std::wstring &file_ext,
                      const std::wstring &file_path,
                      const QModelIndex &index);

    QModelIndex popIndexQueue();

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(xamp::output_device::DeviceState state);

    void volumeChanged(float vol);

    void sampleDataChanged(std::vector<float> const &samples);

    void gaplessPlayback(const QModelIndex &index);

protected:
    SpscQueue<AlignPtr<FileStream>> play_queue_;
    SpscQueue<QModelIndex> index_queue_;
};
