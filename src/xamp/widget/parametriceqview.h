//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/qcustomplot.h>

class ParametricEqView : public QCustomPlot {
public:
	explicit ParametricEqView(QWidget* parent = nullptr);

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private:
	int min_db = -15;
	int max_db = 15;
	int drag_number = -1;
	int dragable_graph_number = 0;
	int max_distance_to_add_point = 20;
};
