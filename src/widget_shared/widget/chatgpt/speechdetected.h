//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

struct SpeechDetectedConfig {
    /// Longest streak of samples with no voice before speech is considered to have ended
    int   patience;
    /// Minimum streak of samples with speach for a segment to be considered as containing speech
    int   minimum_samples;
    /// Tuning coefficient - higher coefficient requires longer tuning
    float beta;
    /// Treshold coeffitient - real threashold is calculated by mean(E) + k*std(E)
    float threshold;
    /// How many samples from the beginning of audio should be used for tuning
    int   adjust_samples;
};

class SpeechDetected : public QObject {
    Q_OBJECT
public:
    explicit SpeechDetected(QObject *parent = nullptr);

    void reset();

    void adjust(const std::vector<float>& data);

    float threshold() const;

    void setAdjustInProgress(bool progress);

    void setVoiceInProgress(bool progress);

    bool getVoiceInProgress();

    void feedSamples(const std::vector<float> &data);

public slots:


signals:
    void speechDetected(std::vector<float> samples);

private:
    bool voiceInProgress = false;
    bool adjustInProgress = false;

    bool _segment_approved = false;
    int _patience_counter = 0;
    int _detected_samples_counter = 0;
    int _adjustment_counter;
    float _mean_energy = 0;
    float _std_energy = 0;
    std::vector<float> _voice_buffer;
    SpeechDetectedConfig _params;
};
