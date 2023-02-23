//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

#include <widget/themecolor.h>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWidget;
class SpectrumWidget;
class QPaintEvent;

class LrcPage : public QFrame {
	Q_OBJECT
	Q_PROPERTY(int disappearBgProg READ GetDisappearBgProg WRITE SetDisappearBgProg)
	Q_PROPERTY(int appearBgProg READ GetAppearBgProg WRITE SetAppearBgProg)
public:
	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWidget* lyrics();

	void SetCover(const QPixmap& cover);

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

	QLabel* cover();

	QSize CoverSize() const;

	SpectrumWidget* spectrum();

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

    void OnThemeChanged(QColor backgroundColor, QColor color);

	void SetBackground(const QImage& cover);

	void ClearBackground();

private:
	void SetAppearBgProg(int x);

	int GetAppearBgProg() const;

	void SetDisappearBgProg(int x);

	int GetDisappearBgProg() const;

	void StartBackgroundAnimation(int durationMs);

	void paintEvent(QPaintEvent*) override;

	void initial();

	int current_bg_alpha_ = 255;
	int prev_bg_alpha_ = 0;
	LyricsShowWidget* lyrics_widget_;
    QLabel* cover_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
	SpectrumWidget* spectrum_;
	QImage background_image_;
	QImage prev_background_image_;
};
