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

    void setFrequency(qreal low_freq, qreal high_freq, qreal frequency);

	void reset();

	void start();

	void stop();

public slots:
    void receiveMagnitude(const std::vector<float>& mag);

private:
	void paintEvent(QPaintEvent* event) override;
};

