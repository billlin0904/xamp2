//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <QPainterPath>

class SmoothCurveGenerator2 {
public:
	QPainterPath GenerateSmoothCurve(const std::vector<QPointF>& points);

private:
	static void CalculateFirstControlPoints(std::vector<double>& result, const std::vector<double>& rhs, int n);

	static void CalculateControlPoints(const std::vector<QPointF>& knots,
		std::vector<QPointF>* first_control_points,
		std::vector<QPointF>* second_control_points);

private:
	std::vector<QPointF> first_control_points_;
	std::vector<QPointF> second_control_points_;
};

inline QPainterPath SmoothCurveGenerator2::GenerateSmoothCurve(const std::vector<QPointF>& points) {
	QPainterPath path;
	int len = points.size();

	if (len < 2) {
		return path;
	}

	CalculateControlPoints(points, &first_control_points_, &second_control_points_);

	path.moveTo(points[0].x(), points[0].y());

	// Using bezier curve to generate a smooth curve.
	for (int i = 0; i < len - 1; ++i) {
		path.cubicTo(first_control_points_[i], second_control_points_[i], points[i + 1]);
	}

	first_control_points_.clear();
	second_control_points_.clear();
	return path;
}

inline void SmoothCurveGenerator2::CalculateFirstControlPoints(std::vector<double>& result, const std::vector<double> & rhs, int n) {
	result.resize(n);
	std::vector<double> tmp(n);
	double b = 2.0;
	result[0] = rhs[0] / b;

	// Decomposition and forward substitution.
	for (int i = 1; i < n; i++) {
		tmp[i] = 1 / b;
		b = (i < n - 1 ? 4.0 : 3.5) - tmp[i];
		result[i] = (rhs[i] - result[i - 1]) / b;
	}

	for (int i = 1; i < n; i++) {
		result[n - i - 1] -= tmp[n - i] * result[n - i]; // Backsubstitution.
	}
}

inline void SmoothCurveGenerator2::CalculateControlPoints(const std::vector<QPointF>& knots,
	std::vector<QPointF>* first_control_points,
	std::vector<QPointF>* second_control_points) {
	int n = knots.size() - 1;

	first_control_points->reserve(n);
	second_control_points->reserve(n);

	for (int i = 0; i < n; ++i) {
		first_control_points->push_back(QPointF());
		second_control_points->push_back(QPointF());
	}

	if (n == 1) {
		// Special case: Bezier curve should be a straight line.
		// P1 = (2P0 + P3) / 3
		(*first_control_points)[0].rx() = (2 * knots[0].x() + knots[1].x()) / 3;
		(*first_control_points)[0].ry() = (2 * knots[0].y() + knots[1].y()) / 3;

		// P2 = 2P1 ¡V P0
		(*second_control_points)[0].rx() = 2 * (*first_control_points)[0].x() - knots[0].x();
		(*second_control_points)[0].ry() = 2 * (*first_control_points)[0].y() - knots[0].y();

		return;
	}

	// Calculate first Bezier control points
	std::vector<double> xs;
	std::vector<double> ys;
	std::vector<double> rhsx(n); // Right hand side vector
	std::vector<double> rhsy(n); // Right hand side vector

	// Set right hand side values
	for (int i = 1; i < n - 1; ++i) {
		rhsx[i] = 4 * knots[i].x() + 2 * knots[i + 1].x();
		rhsy[i] = 4 * knots[i].y() + 2 * knots[i + 1].y();
	}
	rhsx[0] = knots[0].x() + 2 * knots[1].x();
	rhsx[n - 1] = (8 * knots[n - 1].x() + knots[n].x()) / 2.0;
	rhsy[0] = knots[0].y() + 2 * knots[1].y();
	rhsy[n - 1] = (8 * knots[n - 1].y() + knots[n].y()) / 2.0;

	// Calculate first control points coordinates
	CalculateFirstControlPoints(xs, rhsx, n);
	CalculateFirstControlPoints(ys, rhsy, n);

	// Fill output control points.
	for (int i = 0; i < n; ++i) {
		(*first_control_points)[i].rx() = xs[i];
		(*first_control_points)[i].ry() = ys[i];

		if (i < n - 1) {
			(*second_control_points)[i].rx() = 2 * knots[i + 1].x() - xs[i + 1];
			(*second_control_points)[i].ry() = 2 * knots[i + 1].y() - ys[i + 1];
		}
		else {
			(*second_control_points)[i].rx() = (knots[n].x() + xs[n - 1]) / 2;
			(*second_control_points)[i].ry() = (knots[n].y() + ys[n - 1]) / 2;
		}
	}
}
