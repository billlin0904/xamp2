//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QImage>
#include <QPixmap>

class ColorPicker {
public:
	ColorPicker();

	void loadImage(const QPixmap& image);

	void loadImage(const QImage& image);

	QImage getTestImage(const QImage& image) const;

	inline QColor backgroundColor() const noexcept {
		return background_color_;
	}

	inline QColor secondaryColor() const noexcept {
		return secondary_color_;
	}

	inline QColor detailColor() const noexcept {
		return detail_color_;
	}

	inline QColor primaryColor() const noexcept {
		return primary_color_;
	}

private:
	struct QColorCompare {
		bool operator()(const QColor& a, const QColor& b) const noexcept {
			return b.rgba() > a.rgba();
		}
	};

	QColor findEdgeColor() const noexcept;

	void findTextColors() noexcept;

	using ColorSet = std::map<QColor, int32_t, QColorCompare>;
	QColor background_color_;
	QColor secondary_color_;
	QColor detail_color_;
	QColor primary_color_;
	ColorSet colors_;
};
