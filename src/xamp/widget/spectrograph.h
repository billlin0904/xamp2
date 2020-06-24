//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QFrame>
#include <QTimer>
#include <QThread>

#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>

#include <widget/fftprocessor.h>

QT_CHARTS_USE_NAMESPACE

class Spectrograph : public QWidget {
	Q_OBJECT
public:
	explicit Spectrograph(QWidget* parent = nullptr);

    void setFrequency(float low_freq, float high_freq, float frequency);

	void start();

	void stop();

    void stopThread();

    void setBackgroundColor(QColor color);

    FFTProcessor processor;

public slots:
    void updateBar(std::vector<SpectrumData> const & spectrum_data);

private:
    float low_freq_;
    float high_freq_;
    float frequency_;
    float max_lufs_;    
    int32_t minY_;
    int32_t maxY_;
    size_t lufs_count_;
    QChart *chart_;
    QChartView *chart_view_;
    QSplineSeries *lufs_series_;    
    QTimer timer_;
    QTimer reset_timer_;
    QThread thread_;
    QList<double> lufs_data_;
};
