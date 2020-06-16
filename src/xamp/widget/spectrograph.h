//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QFrame>
#include <QTimer>
#include <QThread>

#include <widget/fftprocessor.h>

class Spectrograph : public QFrame {
	Q_OBJECT
public:
	explicit Spectrograph(QWidget* parent = nullptr);

    virtual ~Spectrograph();

    void setFrequency(float low_freq, float high_freq, float frequency);

	void reset();

	void start();

	void stop();

    FFTProcessor processor;

public slots:
    void updateBar(std::vector<SpectrumData> const & spectrum_data);

private:
    size_t barIndex(float frequency) const;

    struct Bar {
        float value {0};
    };

	void paintEvent(QPaintEvent* event) override;

    float low_freq_;
    float high_freq_;
    float frequency_;
    std::vector<Bar> bars_;    
    QTimer timer_;
    QThread thread_;
};
