//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/eqsettings.h>
#include <widget/widget_shared.h>
#include <widget/qcustomplot.h>

class ParametricEqView : public QCustomPlot {
	Q_OBJECT
public:
	explicit ParametricEqView(QWidget* parent = nullptr);

	void InitialAxisTicker(const EqSettings &settings);

	void SetBand(float frequency, float value);

	void ClearBand();

	void SetSpectrumData(int frequency, float value);

signals:
	void DataChanged(int frequency, float value);

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private:
	int drag_number_ = -1;
	int dragable_graph_number_ = 1;
	int max_distance_to_add_point_ = 20;
};
