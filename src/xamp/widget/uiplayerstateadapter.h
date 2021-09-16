//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QModelIndex>

#include <base/spsc_queue.h>
#include <stream/filestream.h>
#include <output_device/devicestatelistener.h>
#include <player/playbackstateadapter.h>
#include <player/samplerateconverter.h>
#include <player/playstate.h>

using xamp::base::Errors;
using xamp::base::AlignPtr;
using xamp::base::Exception;
using xamp::base::SpscQueue;

using xamp::player::PlayerState;
using xamp::player::SampleRateConverter;
using xamp::player::PlaybackStateAdapter;

using xamp::output_device::DeviceState;
using xamp::stream::FileStream;

class UIPlayerStateAdapter final
    : public QObject
    , public PlaybackStateAdapter {
    Q_OBJECT
public:
    explicit UIPlayerStateAdapter(QObject *parent = nullptr);

    void OnSampleTime(double stream_time) override;

    void OnStateChanged(PlayerState play_state) override;

    void OnError(const Exception &ex) override;

    void OnDeviceChanged(DeviceState state) override;

    void OnVolumeChanged(float vol) override;

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(PlayerState play_state);

    void playbackError(Errors error, const QString &message);

    void deviceChanged(DeviceState state);

    void volumeChanged(float vol);
};
