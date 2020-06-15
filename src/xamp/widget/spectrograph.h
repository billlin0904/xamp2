//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <QFrame>
#include <QTimer>

#include <player/fft.h>

class Spectrograph : public QFrame {
	Q_OBJECT
public:
	explicit Spectrograph(QWidget* parent = nullptr);

    void setFrequency(float low_freq, float high_freq, float frequency);

	void reset();

	void start();

	void stop();

public slots:
    void spectrumDataChanged(std::vector<float> const &samples);

private:
    void updateBar();

    size_t barIndex(float frequency) const;

    struct Bar {
        float value {0};
    };

    struct SpectrumData {
        float frequency{0};
        float magnitude{0};
        float phase{0};
    };

	void paintEvent(QPaintEvent* event) override;

    float low_freq_;
    float high_freq_;
    float frequency_;
    int32_t bar_selected_;
    std::vector<Bar> bars_;
    std::vector<SpectrumData> spectrum_data_;
    xamp::base::AlignPtr<xamp::player::FFT> fft_;
    QTimer timer_;
};

