//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWideget;
class QPaintEvent;

class LrcPage : public QFrame {
	Q_OBJECT
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWideget* lyricsWidget();

    QLabel* cover();

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

	void setBackground(const QPixmap& cover);

public slots:
    void OnThemeColorChanged(QColor theme_color, QColor color);

private:
	void setEffect(QWidget *widget, int blurRadius);

	void paintEvent(QPaintEvent*) override;

	void resizeEvent(QResizeEvent*) override;

	void initial();

	LyricsShowWideget* lyrics_widget_;
    QLabel* cover_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
	QPixmap background_image_;
};
