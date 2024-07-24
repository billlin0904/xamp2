//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QAudioInput>
#include <QByteArray>
#include <vector>

class QTimer;

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    ~WaveformWidget();

public slots:
    void readAudioData(const std::vector<int16_t> &buffer);

    void silence();
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer* timer_;
    std::vector<int16_t> buffer_;
};
