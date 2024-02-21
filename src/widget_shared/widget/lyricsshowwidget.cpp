#include <widget/lyricsshowwidget.h>

#include <sstream>
#include <QPainter>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>

#include <widget/widget_shared.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/util/str_utilts.h>

LyricsShowWidget::LyricsShowWidget(QWidget* parent) 
	: WheelableWidget(false, parent)
	, pos_(0)
	, last_lyric_index_(0)
	, item_percent_(0)
	, lrc_color_(Qt::darkGray)
    , lrc_highlight_color_(Qt::black) {
    initial();
}

void LyricsShowWidget::resizeFontSize() {
	auto font_size = 16;
	QFontMetrics lrc_metrics(lrc_font_);
    const auto itr = std::max_element(lyric_.begin(), lyric_.end(),
	                                          [&lrc_metrics](const auto &a, const auto &b) {
		                                          return lrc_metrics.horizontalAdvance(QString::fromStdWString(a.lrc))
			                                          < lrc_metrics.horizontalAdvance(QString::fromStdWString(b.lrc));
	                                          });
	if (itr == lyric_.end()) {
		lrc_font_.setPointSize(font_size);
		return;
	}

	const auto max_lrc = QString::fromStdWString((*itr).lrc);
	if (max_lrc.isEmpty()) {
		lrc_font_.setPointSize(font_size);
		return;
	}

	lrc_font_.setPointSize(font_size);

	lrc_metrics = QFontMetrics(lrc_font_);
	while (lrc_metrics.horizontalAdvance(max_lrc) > size().width() - 30) {		
		const auto max_width = lrc_metrics.horizontalAdvance(max_lrc);
		font_size -= 5;
		lrc_font_.setPointSize(font_size);
		lrc_metrics = QFontMetrics(lrc_font_);
		XAMP_LOG_DEBUG("Change font size => {}, width: {}", font_size, max_width);
	}
}

void LyricsShowWidget::resizeEvent(QResizeEvent* event) {
	resizeFontSize();
}

void LyricsShowWidget::initial() {
    lrc_font_ = font();
	lrc_font_.setPointSize(qAppSettings.valueAsInt(kLyricsFontSize));

	resizeFontSize();
	setDefaultLrc();

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &LyricsShowWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<LyricsShowWidget> action_map(this);
		(void)action_map.addAction(tr("Show original lyrics"), [this]() {
			lrc_ = orilyrc_;
			loadLrc(lrc_);

		});

		(void)action_map.addAction(tr("Show translate lyrics"), [this]() {
			lrc_ = trlyrc_;
			loadLrc(lrc_);
			resizeFontSize();
		});

		action_map.exec(pt);
		});

	setAcceptDrops(true);
}

void LyricsShowWidget::setBackgroundColor(QColor color) {
	background_color_ = color;
}

void LyricsShowWidget::setDefaultLrc() {
	LyricEntry entry;
	entry.lrc = tr("Not found lyrics").toStdWString();
	lyric_.addLrc(entry);
}

void LyricsShowWidget::setCurrentTime(const int32_t time, const bool is_adding) {
	auto time2 = time;

	if (!is_adding) {
	    time2 = (-time2);
	}

	for (auto& lrc : lyric_) {
	    lrc.timestamp += std::chrono::milliseconds(time2);
	}
}

void LyricsShowWidget::paintItem(QPainter* painter, const int32_t index, QRect& rect) {
	const int32_t ih = itemHeight() * 1.2 / 10;
	const int32_t ch = item_offset_ * 1.2 / 10;

    painter->setPen(lrc_color_);
    painter->setFont(lrc_font_);

	if (index == item_) {
		if (item_offset_ == 0) {
			auto font = lrc_font_;
            font.setBold(true);
			font.setPointSize(font.pointSize() + ih);
			painter->setFont(font);            
			current_roll_rect_ = rect;
			current_mask_font_ = painter->font();
		} else {
			auto font = lrc_font_;
            font.setBold(true);
			font.setPointSize(font.pointSize() + ih - ch);
			painter->setFont(font);
		}
	}

	if (index == item_ + 1) {
		auto font = lrc_font_;
        font.setBold(true);
		font.setPointSize(font.pointSize() + ch);
		painter->setFont(font);
	}

	const QFontMetrics metrics(painter->font());
	const auto text = QString::fromStdWString(lyric_.lineAt(index).lrc);

	painter->drawText((rect.width() - metrics.horizontalAdvance(text)) / 2,
		rect.y() + (rect.height() - metrics.height()) / 2,
		metrics.horizontalAdvance(text),
		rect.height(),
		Qt::AlignLeft, text);
}

void LyricsShowWidget::paintBackground(QPainter* painter) {
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::paintItemMask(QPainter* painter) {
	if (item_offset_ == 0) {
		painter->setFont(current_mask_font_);
		painter->setPen(lrc_highlight_color_);

		const QFontMetrics metrics(current_mask_font_);
		painter->drawText((current_roll_rect_.width() - metrics.horizontalAdvance(real_current_text_)) / 2,
			current_roll_rect_.y() + (current_roll_rect_.height() - metrics.height()) / 2,
			mask_length_,
			current_roll_rect_.height(),
			Qt::AlignLeft,
			real_current_text_);
	}
}

int32_t LyricsShowWidget::itemHeight() const {
	const QFontMetrics metrics(lrc_font_);
    return static_cast<int32_t>(metrics.height() * 1.5);
}

int32_t LyricsShowWidget::itemCount() const {
	return lyric_.getSize();
}

void LyricsShowWidget::stop() {
	mask_length_ = -1000;
	last_lyric_index_ = -1;
	current_roll_rect_ = QRect(0, 0, 0, 0);
	real_current_text_.clear();
	lyric_.clear();
	update();
}

void LyricsShowWidget::dragEnterEvent(QDragEnterEvent* event) {
	event->acceptProposedAction();
}

void LyricsShowWidget::dragMoveEvent(QDragMoveEvent* event) {
	event->acceptProposedAction();
}

void LyricsShowWidget::dragLeaveEvent(QDragLeaveEvent* event) {
	event->accept();
}

void LyricsShowWidget::dropEvent(QDropEvent* event) {
	const auto* mime_data = event->mimeData();

	if (mime_data->hasUrls()) {
        Q_FOREACH(auto const& url, mime_data->urls()) {
			loadLrcFile(url.toLocalFile());
			break;
		}
		event->acceptProposedAction();
	}
}

bool LyricsShowWidget::loadLrcFile(const QString &file_path) {
	const QFileInfo file_info(file_path);

	const auto lrc_path = file_info.path()
		+ qTEXT("/")
		+ file_info.completeBaseName()
		+ qTEXT(".lrc");

	stop();
	if (!lyric_.parseFile(lrc_path.toStdWString())) {
		setDefaultLrc();
		return false;
	}
	resizeFontSize();
	update();
	return true;
}

void LyricsShowWidget::onAddFullLrc(const QString& lrc) {
	stop();

    auto i = 0;
	const auto lyrics = lrc.split(qTEXT("\n"));

	for (const auto &ly : lyrics) {
		LyricEntry l;
		l.index = i++;
		l.lrc = ly.toStdWString();
		lyric_.addLrc(l);
	}

	stop_scroll_time_ = true;
}

void LyricsShowWidget::loadLrc(const QString& lrc) {
	std::wistringstream stream{ lrc.toStdWString() };
	if (!lyric_.parse(stream)) {
		setDefaultLrc();
	}
	else {
		stop_scroll_time_ = false;
	}
	resizeFontSize();
	onSetLrcTime(0);
	update();
}

void LyricsShowWidget::onSetLrc(const QString &lrc, const QString& trlyrc) {
	orilyrc_ = lrc;
	trlyrc_ = trlyrc;
	lrc_ = orilyrc_;
	stop();
	loadLrc(lrc_);
}

void LyricsShowWidget::onSetLrcTime(int32_t stream_time) {
	if (stop_scroll_time_) {
		return;
	}

	stream_time = stream_time + kScrollTime;
	pos_ = stream_time;

	if (is_scrolled_ || !lyric_.getSize()) {
		update();
		return;
	}

    const auto &ly = lyric_.getLyrics(std::chrono::milliseconds(stream_time));

	if (item_ != ly.index && item_offset_ == 0) {
		mask_length_ = -1000;
		current_roll_rect_ = QRect(0, 0, 0, 0);
		real_current_text_ = QString::fromStdWString(ly.lrc);
		item_offset_ = -1;
		onScrollTo(ly.index);
	}

	if (item_offset_ != 0) {
		update();
		return;
	}

    const auto &post_ly = lyric_.getLyrics(std::chrono::milliseconds(pos_));

	const auto text = QString::fromStdWString(post_ly.lrc);
	const auto interval = post_ly.index;
	const auto precent = static_cast<float>(post_ly.index) / static_cast<float>(lyric_.getSize());

	const QFontMetrics metrics(current_mask_font_);

	if (item_percent_ == precent) {
		const auto count = static_cast<double>(interval) / 25.0;
		const float lrc_mask_mini_step = metrics.horizontalAdvance(text) / count;
		mask_length_ += lrc_mask_mini_step;
	}
	else {
		mask_length_ = metrics.horizontalAdvance(real_current_text_) * precent;
	}

	item_percent_ = precent;
	update();
}

void LyricsShowWidget::onSetLrcFont(const QFont & font) {
	lrc_font_ = font;
	current_mask_font_ = font;
	update();
}

void LyricsShowWidget::onSetLrcHighLight(const QColor & color) {
	lrc_highlight_color_ = color;
	update();
}

void LyricsShowWidget::onSetLrcColor(const QColor& color) {
	lrc_color_ = color;
	update();
}
