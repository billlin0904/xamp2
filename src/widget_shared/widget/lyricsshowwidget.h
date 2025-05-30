//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/ilrrcparser.h>
#include <widget/wheelablewidget.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

#include <base/memory.h>
#include <base/furigana.h>
#include <base/charset_detector.h>

#include <widget/util/str_util.h>

class QDropEvent;

class XAMP_WIDGET_SHARED_EXPORT LyricsShowWidget : public WheelableWidget {
	Q_OBJECT

public:
	static constexpr auto kKaraokeHighLightColor = QColor(247, 247, 59, 255);
	static constexpr auto kNormalColor = QColor(69, 69, 69, 255);
	static constexpr auto kHighLightColor = QColor(164, 216, 216, 255);

	explicit LyricsShowWidget(QWidget *parent = nullptr);

	Q_DISABLE_COPY(LyricsShowWidget)

	bool loadFile(const QString &file_path);

	void loadFromParser(const QSharedPointer<ILrcParser> &parser);

	bool isValid() const;

	void paintItem(QPainter* painter, int32_t index, QRect &rect) override;

	void paintItemMask(QPainter* painter) override;

	void paintBackground(QPainter* painter) override;

	int32_t itemHeight() const override;

	int32_t itemCount() const override;

	void setBackgroundColor(QColor color);

	QString parsedLyrics() const;

	QColor normalColor() const {
		return lrc_color_;
	}

	QColor highlightColor() const {
		return lrc_highlight_color_;
	}

	QColor karaokeHighlightColor() const {
		return karaoke_highlight_color_;
	}

public slots:
	void stop();

	void setLrc(const QString& lrc, const QString& trlyrc = kEmptyString);

	void setLrcTime(int32_t stream_time);

	void setLrcFont(const QFont &font);

	void setHighLightColor(const QColor &color);

	void setNormalColor(const QColor& color);

	void setKaraokeHighlightColor(const QColor& color);

	void setFullLrc(const QString& lrc, double duration);

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

	QRect itemBoundingRect(int index, int offset) const;

	bool is_lrc_valid_{ false };
	bool stop_scroll_time_{ false };
	bool is_fulled_{ false };
	int32_t pos_{ 0 };
    int32_t last_lyric_index_{ 0 };
	float item_percent_{ 0 };
	float old_fraction_ = 0.f;
	float highlight_fraction_ = 0.f;
	QRect last_highlight_rect_;
	QRect currentHighlightRect_;
	QSize base_size_;
	QFont lrc_font_;
	QFont current_mask_font_;
	QColor lrc_color_;
	QColor lrc_highlight_color_;
	QColor karaoke_highlight_color_;
	QColor background_color_;
	QString lrc_;
	QString orilyrc_;
	QString trlyrc_;
	QSharedPointer<ILrcParser> lyric_;
	Furigana furigana_;
	OpenCCConvert convert_;
	LanguageDetector language_detector_;
	std::vector<std::vector<FuriganaEntity>> furiganas_;
};
