//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/vinylwidget.h>
#include <widget/scrolllabel.h>

class Spectrograph;
class LyricsShowWideget;

class LrcPage : public QFrame {
	Q_OBJECT
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWideget* lyricsWidget();

    VinylWidget* cover();

	QLabel* album();

	QLabel* artist();

    ScrollLabel* title();

	Spectrograph* spectrum();

	void setTextColor(QColor color);

private:
	void initial();

	LyricsShowWideget* lyrics_widget_;
    VinylWidget* vinyl_;
	QLabel* album_;
	QLabel* artist_;
    ScrollLabel* title_;
    Spectrograph* spectrograph_;
};
