//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QAudioInput>
#include <QByteArray>
#include <vector>

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    ~WaveformWidget();

public slots:
    void readAudioData(const std::vector<int16_t> &buffer);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<int16_t> buffer_;
};
