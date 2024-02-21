//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QProxyStyle>
#include <widget/widget_shared_global.h>

class IconSizeStyle : public QProxyStyle {
	Q_OBJECT
public:
	explicit IconSizeStyle(int32_t size)
		: size_(size) {		
	}

    int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override {
        if (metric == PM_SmallIconSize) {
            return size_;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
private:
    int32_t size_;
};
