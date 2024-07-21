//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <widget/widget_shared.h>

class SpeechDetected : public QObject {
    Q_OBJECT
public:
    explicit SpeechDetected(QObject *parent = nullptr);

    ~SpeechDetected() override;

    void reset();

    bool isSpeech(const std::vector<float>& data);
public slots:

signals:    

private:
    class SpeechDetectedImpl;
    AlignPtr<SpeechDetectedImpl> impl_;
};
