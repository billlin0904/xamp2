//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWidget;
class SpectrumWidget;
class QPaintEvent;

class LrcPage : public QFrame {
	Q_OBJECT
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWidget* lyrics();

	void setCover(const QPixmap& cover);

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

	QLabel* cover();

	QSize coverSize() const;

	SpectrumWidget* spectrum();

	void setBackgroundColor(QColor backgroundColor);

public slots:
    void onThemeChanged(QColor backgroundColor, QColor color);

	void setBackground(const QImage& cover);

	void clearBackground();

private:
	void paintEvent(QPaintEvent*) override;

	void initial();

	LyricsShowWidget* lyrics_widget_;
    QLabel* cover_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
	SpectrumWidget* spectrum_;
	QImage background_image_;
};
