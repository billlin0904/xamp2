//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <player/playbackstateadapter.h>

class PlayerStateAdapter 
    : public QObject
    , public xamp::player::PlaybackStateAdapter {
    Q_OBJECT
public:
    explicit PlayerStateAdapter(QObject *parent = nullptr);

signals:
    void sampleTimeChanged(double stream_time);

    void stateChanged(xamp::player::PlayerState play_state);

    void playbackError(xamp::base::Errors error, const QString &message);

protected:
	void OnSampleTime(double stream_time) override;

    void OnStateChanged(xamp::player::PlayerState play_state) override;

    void OnError(const xamp::base::Exception &ex) override;
};