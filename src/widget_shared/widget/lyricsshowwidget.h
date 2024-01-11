//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/lrcparser.h>
#include <widget/wheelablewidget.h>
#include <widget/widget_shared_global.h>
#include <widget/str_utilts.h>

class QDropEvent;

class XAMP_WIDGET_SHARED_EXPORT LyricsShowWidget : public WheelableWidget {
	Q_OBJECT

public:
	explicit LyricsShowWidget(QWidget *parent = nullptr);

	Q_DISABLE_COPY(LyricsShowWidget)

	bool loadLrcFile(const QString &file_path);

	void setCurrentTime(int32_t time, bool is_adding = true);

	void paintItem(QPainter* painter, int32_t index, QRect &rect) override;

	void paintItemMask(QPainter* painter) override;

	void paintBackground(QPainter* painter) override;

	int32_t itemHeight() const override;

	int32_t itemCount() const override;

	void setBackgroundColor(QColor color);

public slots:
	void stop();	

	void onSetLrc(const QString &lrc, const QString& trlyc = kEmptyString);

	void onSetLrcTime(int32_t length);

	void onSetLrcFont(const QFont &font);

	void onSetLrcHighLight(const QColor &color);

	void onSetLrcColor(const QColor& color);

	void onAddFullLrc(const QString& lrc);

private:
	void dragEnterEvent(QDragEnterEvent* event) override;

	void dragMoveEvent(QDragMoveEvent* event) override;

	void dragLeaveEvent(QDragLeaveEvent* event) override;

	void dropEvent(QDropEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void setDefaultLrc();

    void initial();

	void loadLrc(const QString& lrc);

	void resizeFontSize();

	bool stop_scroll_time_{ false };
	int32_t pos_;
    int32_t last_lyric_index_;
	float item_percent_;
	QSize base_size_;
	QFont lrc_font_;
	QFont current_mask_font_;
	QColor lrc_color_;
	QColor lrc_highlight_color_;
	QColor background_color_;
	QString lrc_;
	QString orilyrc_;
	QString trlyrc_;
	LrcParser lyric_;	
};
