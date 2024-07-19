//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <whisper.h>
#include <vector>

class WhisperService : public QObject {
    Q_OBJECT
public:
    explicit WhisperService(QObject *parent = nullptr);

    ~WhisperService() override;

    void loadModel(const QString &file_path);

    void unloadModel();

signals:
    void resultReady(const QString& result);

public slots:
    void readSamples(const std::vector<float> &samples);

private:
    class Whisper;
    QScopedPointer<Whisper> whisper_;
};
