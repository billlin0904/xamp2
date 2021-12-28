//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWidget;
class QPaintEvent;

class LrcPage : public QFrame {
	Q_OBJECT
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWidget* lyricsWidget();

	void setCover(const QPixmap& cover);

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

	QLabel* cover();

	QSize coverSize() const;

public slots:
    void onThemeChanged(QColor backgroundColor, QColor color);

private:
	void initial();

	LyricsShowWidget* lyrics_widget_;
    QLabel* cover_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
	QPixmap background_image_;
};
