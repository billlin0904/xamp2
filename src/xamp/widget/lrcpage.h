//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
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
	Q_PROPERTY(int disappearBgProg READ getDisappearBgProg WRITE setDisappearBgProg)
	Q_PROPERTY(int appearBgProg READ getAppearBgProg WRITE setAppearBgProg)
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
	void setAppearBgProg(int x);

	int getAppearBgProg() const;

	void setDisappearBgProg(int x);

	int getDisappearBgProg() const;

	void startBackgroundAnimation(int durationMs);

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
