//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <QFrame>

class Spectrograph : public QFrame {
	Q_OBJECT
public:
	explicit Spectrograph(QWidget* parent = nullptr);

	void setFrequency(double low_freq, double high_freq, double frequency);

	void reset();

public slots:
	void onGetMagnitudeSpectrum(const std::vector<float>& spectrum);

private:
	struct Bar {		
		bool clipped{ false };
		int fall{ 0 };
		qreal value{ 0.0 };
	};

	struct Spectrum {
		qreal frequency{ 0.0 };
		qreal amplitude{ 0.0 };
	};

	int barIndex(qreal frequency) const;

	void paintEvent(QPaintEvent* event) override;

	double low_freq_;
	double high_freq_;
	double frequency_;
	std::vector<Spectrum> spectrum_;
	std::vector<Bar> bars_;
};

