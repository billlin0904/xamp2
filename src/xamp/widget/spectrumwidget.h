//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <stream/stream.h>

#include <QFrame>

using xamp::stream::ComplexValarray;

class SpectrumWidget : public QFrame {
	Q_OBJECT
public:
	explicit SpectrumWidget(QWidget* parent = nullptr);

	void setParams(const double& low_freq, const double& high_freq);

	void reset();

public slots:
	void onFFTResultChanged(ComplexValarray const& result);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	double low_freq_{ 0 };
	double high_freq_{ 0 };
	ComplexValarray fft_result_;
	std::vector<float> freq_data_;
	std::vector<float> bars_;
};
