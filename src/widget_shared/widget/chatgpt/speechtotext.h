//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QThread>

class QIODevice;
class QAudioSource;
class SpeechDetected;
class WhisperService;

class SpeechToText : public QObject {
    Q_OBJECT
public:
    static constexpr auto kSampleRate = 16000;

    SpeechToText();

    ~SpeechToText() override;

    void loadModel(const QString &file_path);

    void start();

    void stop();

    void stopService();

signals:
    void resultReady(const QString& result);

private:
    QIODevice *device_;
    QThread whisper_thread_;
    QScopedPointer<QAudioSource> source_;
    QScopedPointer<SpeechDetected> speech_detected_;
    QScopedPointer<WhisperService> whisper_;
};

