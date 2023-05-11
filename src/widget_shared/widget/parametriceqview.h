//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/eqsettings.h>
#include <widget/widget_shared.h>
#include <widget/qcustomplot.h>
#include <widget/widget_shared_global.h>

class ParametricEqView : public QCustomPlot {
	Q_OBJECT
public:
	explicit ParametricEqView(QWidget* parent = nullptr);

	void InitialAxisTicker(const EqSettings &settings);

	void SetBand(float frequency, float value);

	void SetBand(EQFilterTypes type, int frequency, float value, float q);

	void ClearBand();

	void SetSpectrumData(int frequency, float value);

signals:
	void DataChanged(int frequency, float value);

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private:
	int32_t drag_number_ = -1;
	int32_t dragable_graph_number_ = 1;
	int32_t max_distance_to_add_point_ = 20;
	int32_t sample_rate_ = 44100;
	QCPGraph* mLowPassGraph;
	QCPGraph* mHighPassGraph;
	QCPGraph* mHighBandPassGraph;
	QCPGraph* mHighBandPassQGraph;
	QCPGraph* mNotchGraph;
	QCPGraph* mAllPassGraph;
	QCPGraph* mAllPeakingEqGraph;
	QCPGraph* mLowShelfGraph;
	QCPGraph* mLowHighShelfGraph;
};
