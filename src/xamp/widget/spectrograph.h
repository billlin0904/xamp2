//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <QFrame>
#include <QTimer>

class Spectrograph : public QFrame {
	Q_OBJECT
public:
	explicit Spectrograph(QWidget* parent = nullptr);

	void setFrequency(double low_freq, double high_freq, double frequency);

	void reset();

	void start();

	void stop();

public slots:
	void onGetMagnitudeSpectrum(const std::vector<float>& mag);

private:
	struct Bar {
		qreal value{ 0.0 };	
		qreal amplitude{ 0.0 };
	};

	struct SpectrumData {
		qreal frequency{ 0.0 };
		qreal amplitude{ 0.0 };
		qreal db{ 0.0 };
	};

	void init();

	int barIndex(qreal frequency) const;

	void paintEvent(QPaintEvent* event) override;

	double low_freq_;
	double high_freq_;
	double frequency_;
	std::vector<SpectrumData> results_;
	std::vector<Bar> bars_;
	QTimer timer_;
};

