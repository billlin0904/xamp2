//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QModelIndex>

#include <base/spsc_queue.h>
#include <output_device/devicestatelistener.h>
#include <stream/filestream.h>
#include <player/playbackstateadapter.h>
#include <player/SampleRateConverter.h>
#include <player/playstate.h>

using xamp::base::Errors;
using xamp::base::AlignPtr;
using xamp::base::Exception;
using xamp::base::SpscQueue;

using xamp::player::PlayerState;
using xamp::player::SampleRateConverter;
using xamp::player::PlaybackStateAdapter;
using xamp::player::GaplessPlayEntry;

using xamp::output_device::DeviceInfo;
using xamp::output_device::DeviceState;
using xamp::stream::FileStream;

class UIPlayerStateAdapter final
    : public QObject
    , public PlaybackStateAdapter {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    size_t GetPlayQueueSize() const override;

    GaplessPlayEntry& PlayQueueFont() override;

    void PopPlayQueue() override;

    void OnSampleTime(double stream_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(xamp::output_device::DeviceState state) override;

    void OnVolumeChanged(float vol) override;

    void OnDisplayChanged(std::vector<float> const& display) override;

    void OnGaplessPlayback() override;    

    void addPlayQueue(AlignPtr<FileStream> &&stream, AlignPtr<SampleRateConverter> &&sample_rate_converter, const QModelIndex &index);

    QModelIndex popIndexQueue();

    QModelIndex currentIndex();

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(DeviceState state);

    void volumeChanged(float vol);

    void gaplessPlayback(const QModelIndex &index);

    void displayChanged(std::vector<float> const& display);

protected:
    static constexpr auto kPlayQueueSize = 8;
    static constexpr auto kIndexQueueSize = 8;

    void ClearPlayQueue() override;

    SpscQueue<GaplessPlayEntry> play_queue_;
    SpscQueue<QModelIndex> index_queue_;
};
