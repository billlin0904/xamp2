//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <player/fft.h>

struct SpectrumData {
    float frequency{ 0 };
    float magnitude{ 0 };
    float phase{ 0 };
    float lufs{ 0 };
};

class FFTProcessor : public QObject {
	Q_OBJECT
public:
	explicit FFTProcessor(QObject* parent = nullptr);

    void setFrequency(float frequency);

public slots:
    void OnSampleDataChanged(std::vector<float> const& samples);

signals:
    void spectrumDataChanged(std::vector<SpectrumData> const& data);

private:
    float frequency_;
    xamp::base::AlignPtr<xamp::player::FFT> fft_;
    std::vector<SpectrumData> spectrum_data_;
};