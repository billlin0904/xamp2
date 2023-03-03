//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/lrcparser.h>
#include <widget/wheelablewidget.h>

class QDropEvent;

class LyricsShowWidget : public WheelableWidget {
	Q_OBJECT

public:
	explicit LyricsShowWidget(QWidget *parent = nullptr);

	Q_DISABLE_COPY(LyricsShowWidget)

	bool LoadLrcFile(const QString &file_path);

	void SetCurrentTime(int32_t time, bool is_adding = true);

	void PaintItem(QPainter* painter, int32_t index, QRect &rect) override;

	void PaintItemMask(QPainter* painter) override;

	void PaintBackground(QPainter* painter) override;

	int32_t ItemHeight() const override;

	int32_t ItemCount() const override;

	void SetBackgroundColor(QColor color);

public slots:
	void stop();	

	void SetLrc(const QString &lrc, const QString& trlyc);

	void SetLrcTime(int32_t length);

	void SetLrcFont(const QFont &font);

	void SetLrcHighLight(const QColor &color);

	void SetLrcColor(const QColor& color);

	void AddFullLrc(const QString& lrc, std::chrono::milliseconds duration);

private:
	void dragEnterEvent(QDragEnterEvent* event) override;

	void dragMoveEvent(QDragMoveEvent* event) override;

	void dragLeaveEvent(QDragLeaveEvent* event) override;

	void dropEvent(QDropEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void SetDefaultLrc();

    void initial();

	void LoadLrc(const QString& lrc);

	void ResizeFontSize();

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
