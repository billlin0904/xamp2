//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <QString>
#include <QUrl>
#include <QWidget>

class ITopWindow : public QWidget {
public:
    virtual ~ITopWindow() override = default;

    virtual void setTaskbarProgress(int32_t percent) = 0;

    virtual void resetTaskbarProgress() = 0;

    virtual void setTaskbarPlayingResume() = 0;

    virtual void setTaskbarPlayerPaused() = 0;

    virtual void setTaskbarPlayerPlaying() = 0;

    virtual void setTaskbarPlayerStop() = 0;

    virtual bool useNativeWindow() const noexcept = 0;
protected:
    ITopWindow() = default;
};

class IXampPlayer : public QWidget {
public:
    virtual ~IXampPlayer() override = default;

    virtual void addDropFileItem(const QUrl& url) = 0;

    virtual void deleteKeyPress() = 0;

    virtual void playPreviousClicked() = 0;

    virtual void playNextClicked() = 0;

    virtual void stopPlayedClicked() = 0;

    virtual void play() = 0;
protected:
    IXampPlayer() = default;
};

