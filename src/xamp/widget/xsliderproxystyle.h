//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QVariant>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleOptionComplex>

#include <widget/str_utilts.h>

// https://www.cnblogs.com/zhiyiYo/p/15244149.html
class XSliderProxyStyle : public QProxyStyle {
public:
	XSliderProxyStyle() {
		config_[Q_TEXT("groove.height")] = 3;
		config_[Q_TEXT("sub-page.color")] = QColor(255, 255, 255);
		config_[Q_TEXT("add-page.color")] = QColor(255, 255, 255, 64);
		config_[Q_TEXT("handle.color")] = QColor(255, 255, 255);
		config_[Q_TEXT("handle.ring-width")] = 4;
		config_[Q_TEXT("handle.hollow-radius")] = 6;
		config_[Q_TEXT("handle.margin")] = 6;
		const auto width = config_[Q_TEXT("handle.ring-width")].toInt() +
			config_[Q_TEXT("handle.ring-width")].toInt() +
			config_[Q_TEXT("handle.hollow-radius")].toInt();
		config_[Q_TEXT("handle.size")] = QSize(2 * width, 2 * width);
	}

	QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const override {
		auto* slider_opt = qstyleoption_cast<const QStyleOptionSlider*>(opt);
		if (cc != CC_Slider or slider_opt->orientation != Qt::Horizontal or sc == SC_SliderTickmarks) {
			return QProxyStyle::subControlRect(cc, opt, sc, widget);
		}

		auto rect = opt->rect;
		if (sc == QProxyStyle::SC_SliderGroove) {
			auto h = config_[Q_TEXT("groove.height")].toInt();
			const auto groove_rect = QRectF(0, (rect.height() - h) / 2, rect.width(), h);
			return groove_rect.toRect();
		} else if (sc == QProxyStyle::SC_SliderHandle) {
			auto size = config_[Q_TEXT("handle.size")].toSize();
			auto x = sliderPositionFromValue(
				slider_opt->minimum, slider_opt->maximum, slider_opt->sliderPosition, rect.width());
			x *= (rect.width() - size.width()) / rect.width();
			auto slider_rect = QRectF(x, 0, size.width(), size.height());
			return slider_rect.toRect();
		}

		return QProxyStyle::subControlRect(cc, opt, sc, widget);
	}

	void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
		const QWidget* widget) const override {
		auto* slider_opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
		if (slider_opt->orientation != Qt::Horizontal) {
			return QProxyStyle::drawComplexControl(control, option, painter, widget);
		}

		auto grooveRect = subControlRect(control, option, SC_SliderGroove, widget);
		auto handleRect = subControlRect(control, option, SC_SliderHandle, widget);
		painter->setRenderHints(QPainter::Antialiasing);
		painter->setPen(Qt::NoPen);
		painter->save();
		painter->translate(grooveRect.topLeft());

		auto w = handleRect.x() - grooveRect.x();
		auto h = config_[Q_TEXT("groove.height")].toInt();
		//painter->setBrush(config_[Q_TEXT("sub-page.color")].toColor());
		painter->drawRect(0, 0, w, h);

		auto x = w + config_[Q_TEXT("handle.size")].toSize().width();
		//painter->setBrush(config_[Q_TEXT("add-page.color")].toColor());
		painter->drawRect(x, 0, grooveRect.width() - w, h);
		painter->restore();

		auto ringWidth = config_[Q_TEXT("handle.ring-width")].toInt();
		auto hollowRadius = config_[Q_TEXT("handle.hollow-radius")].toInt();
		auto radius = ringWidth + hollowRadius;

		QPainterPath path;
		path.moveTo(0, 0);
		auto center = handleRect.center() + QPoint(1, 1);
		path.addEllipse(center, radius, radius);
		path.addEllipse(center, hollowRadius, hollowRadius);

		QColor handleColor;

		if (slider_opt->activeSubControls != SC_SliderHandle) {
			handleColor.setAlpha(153);
		} else {
			handleColor.setAlpha(255);
		}
		painter->setBrush(handleColor);
		painter->drawPath(path);

		auto* ww = dynamic_cast<const QAbstractSlider*>(widget);
		if (ww->isSliderDown()) {
			handleColor.setAlpha(255);
			painter->setBrush(handleColor);
			painter->drawEllipse(handleRect);
		}
		return QProxyStyle::drawComplexControl(control, option, painter, widget);
	}
private:
	QMap<QString, QVariant> config_;
};
