#include <sstream>
#include <QPainter>
#include <QDropEvent>
#include <QMimeData>
#include <QColorDialog>

#include <widget/settingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/str_utilts.h>
#include <widget/lyricsshowwidget.h>

LyricsShowWidget::LyricsShowWidget(QWidget* parent) 
	: WheelableWidget(false, parent)
	, pos_(0)
	, last_lyric_index_(0)
	, item_precent_(0)
	, lrc_color_(Qt::darkGray)
    , lrc_hightlight_color_(Qt::black) {
    initial();
}

void LyricsShowWidget::initial() {
    lrc_font_ = font();

	lrc_font_.setPointSize(AppSettings::getAsInt(kLyricsFontSize));

	setDefaultLrc();

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &LyricsShowWidget::customContextMenuRequested, [this](auto pt) {
		ActionMap<LyricsShowWidget, std::function<void()>> action_map(this);
		(void)action_map.addAction(tr("Set font size(small)"), [this]() {
			AppSettings::setValue(kLyricsFontSize, 12);
			lrc_font_.setPointSize(12);
			});

		(void)action_map.addAction(tr("Set font size(middle)"), [this]() {
			AppSettings::setValue(kLyricsFontSize, 16);
			lrc_font_.setPointSize(16);
			});

		(void)action_map.addAction(tr("Set font size(big)"), [this]() {
			lrc_font_.setPointSize(24);
			AppSettings::setValue(kLyricsFontSize, 24);
			});

		(void)action_map.addAction(tr("Change text color"), [this]() {
			auto color = QColorDialog::getColor(Qt::white, this);
			setLrcColor(color);
			setLrcHightLight(color);
			AppSettings::setValue(kLyricsTextColor, color);
			AppSettings::setValue(kLyricsHighLightTextColor, color);
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
	lyric_.AddLrc(entry);
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
			current_rollrect_ = rect;
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

	QFontMetrics metrics(painter->font());
	const auto text = QString::fromStdWString(lyric_.LineAt(index).lrc);

	painter->drawText((rect.width() - metrics.width(text)) / 2,
		rect.y() + (rect.height() - metrics.height()) / 2,
		metrics.width(text),
		rect.height(),
		Qt::AlignLeft, text);
}

void LyricsShowWidget::paintBackground(QPainter* painter) {
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::paintItemMask(QPainter* painter) {
	if (item_offset_ == 0) {
		painter->setFont(current_mask_font_);
		painter->setPen(lrc_hightlight_color_);

		QFontMetrics metrics(current_mask_font_);
		painter->drawText((current_rollrect_.width() - metrics.width(real_current_text_)) / 2,
			current_rollrect_.y() + (current_rollrect_.height() - metrics.height()) / 2,
			mask_length_,
			current_rollrect_.height(),
			Qt::AlignLeft,
			real_current_text_);
	}
}

int32_t LyricsShowWidget::itemHeight() const {
    QFontMetrics metrics(lrc_font_);
    return static_cast<int32_t>(metrics.height() * 1.5);
}

int32_t LyricsShowWidget::itemCount() const {
	return lyric_.GetSize();
}

void LyricsShowWidget::stop() {
	mask_length_ = -1000;
	last_lyric_index_ = -1;
	current_rollrect_ = QRect(0, 0, 0, 0);
	real_current_text_.clear();
	lyric_.Clear();
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
	stop();
	if (!lyric_.ParseFile(file_path.toStdWString())) {
		setDefaultLrc();
		return false;
	}
	update();
	return true;
}

void LyricsShowWidget::addFullLrc(const QString& lrc, std::chrono::milliseconds duration) {
    auto i = 0;
	const auto lyrics = lrc.split(Q_UTF8("\n"));
	const auto min_duration = duration / lyrics.count();

	for (const auto &ly : lyrics) {
		LyricEntry l;
		l.index = i++;
		l.lrc = ly.toStdWString();
		l.timestamp = min_duration * i;
		lyric_.AddLrc(l);
	}
}

void LyricsShowWidget::loadLrc(const QString& lrc) {
	std::wistringstream stream{ lrc.toStdWString() };
	if (!lyric_.Parse(stream)) {
		return;
	}
	update();
}

void LyricsShowWidget::setLrc(const QString &lrc) {
	lrc_ = lrc;
	stop();
	loadLrc(lrc_);
}

void LyricsShowWidget::setLrcTime(int32_t stream_time) {
	stream_time = stream_time + kScrollTime;
	pos_ = stream_time;

	if (is_scrolled_ || !lyric_.GetSize()) {
		update();
		return;
	}

    const auto &ly = lyric_.GetLyrics(std::chrono::milliseconds(stream_time));

	if (item_ != ly.index && item_offset_ == 0) {
		mask_length_ = -1000;
		current_rollrect_ = QRect(0, 0, 0, 0);
		real_current_text_ = QString::fromStdWString(ly.lrc);
		item_offset_ = -1;
		scrollTo(ly.index);
	}

	if (item_offset_ != 0) {
		update();
		return;
	}

    const auto &post_ly = lyric_.GetLyrics(std::chrono::milliseconds(pos_));

	const auto text = QString::fromStdWString(post_ly.lrc);
	const auto interval = post_ly.index;
	const auto precent = float(post_ly.index) / float(lyric_.GetSize());

	QFontMetrics metrics(current_mask_font_);

	if (item_precent_ == precent) {
		const auto count = double(interval) / 25.0;
		const float lrc_mask_mini_step = metrics.width(text) / count;
		mask_length_ += lrc_mask_mini_step;
	}
	else {
		mask_length_ = metrics.width(real_current_text_) * precent;
	}

	item_precent_ = precent;
	update();
}

void LyricsShowWidget::setLrcFont(const QFont & font) {
	lrc_font_ = font;
	current_mask_font_ = font;
	update();
}

void LyricsShowWidget::setLrcHightLight(const QColor & color) {
	lrc_hightlight_color_ = color;
	update();
}

void LyricsShowWidget::setLrcColor(const QColor& color) {
	lrc_color_ = color;
	update();
}
