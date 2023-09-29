//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWidget;
class SpectrumWidget;
class QPaintEvent;

class XAMP_WIDGET_SHARED_EXPORT LrcPage : public QFrame {
	Q_OBJECT
	Q_PROPERTY(int disappearBgProg READ GetDisappearBgProgress WRITE SetDisappearBgProgress)
	Q_PROPERTY(int appearBgProg READ GetAppearBgProgress WRITE SetAppearBgProgress)
public:
	static constexpr auto kBlurAlpha = 200;
	static constexpr int kBlurBackgroundAnimationMs = 1500;

	explicit LrcPage(QWidget* parent = nullptr);

	LyricsShowWidget* lyrics();

	void SetCover(const QPixmap& cover);

	void AddCoverShadow(bool is_dark = false);

	QLabel* format();

	ScrollLabel* album();

	ScrollLabel* artist();

    ScrollLabel* title();

	QLabel* cover();

	QSize CoverSize() const;

	SpectrumWidget* spectrum();

	void SetFullScreen(bool enter);

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

    void OnThemeChanged(QColor backgroundColor, QColor color);

	void SetBackground(const QImage& cover);

	void ClearBackground();

private:
	void SetAppearBgProgress(int x);

	int GetAppearBgProgress() const;

	void SetDisappearBgProgress(int x);

	int GetDisappearBgProgress() const;

	void StartBackgroundAnimation(int durationMs);

	void paintEvent(QPaintEvent*) override;

	void resizeEvent(QResizeEvent* event) override;

	void initial();

	int current_bg_alpha_ = 255;
	int prev_bg_alpha_ = 0;
	LyricsShowWidget* lyrics_widget_;
    QLabel* cover_label_;
	QLabel* format_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
    ScrollLabel* title_;
	SpectrumWidget* spectrum_;
	QPixmap cover_;
	QImage background_image_;
	QImage prev_background_image_;
};
