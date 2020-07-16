//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWideget;

class LrcPage : public QFrame {
	Q_OBJECT
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWideget* lyricsWidget();

    QLabel* cover();

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

public slots:
    void OnThemeColorChanged(QColor theme_color, QColor color);

private:
	void initial();

	LyricsShowWideget* lyrics_widget_;
    QLabel* cover_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
};
