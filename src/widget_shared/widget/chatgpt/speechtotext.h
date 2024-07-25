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
class QAudioDevice;
class QAudioSink;

class SpeechToText : public QObject {
    Q_OBJECT
public:
    static constexpr auto kSampleRate = 16000;
    static constexpr auto kSilenceThreshold = 16;
    static constexpr auto kMinDetectedSamples = 480;
    static constexpr auto kMaxSamples = 16000 * 10;

    SpeechToText();

    ~SpeechToText() override;

    void loadModel(const QString &file_path);

    void start();

    void stop();

	void setRealtimeMonitorDevice(const QAudioDevice &output);

    void stopService();

signals:
    void resultReady(const QString& result);

    void sampleReady(const std::vector<int16_t> &samples);

	void silenceDetected();
private:
    int32_t silence_counter_{ 0 };
    QIODevice* device_{ nullptr };
    QIODevice* render_{ nullptr };
    QThread whisper_thread_;
    QScopedPointer<QAudioSource> source_;
    QScopedPointer<SpeechDetected> speech_detected_;
    QScopedPointer<WhisperService> whisper_;
	QScopedPointer<QAudioSink> sink_;
    std::vector<float> buffer_;
    std::vector<float> input_;
};

