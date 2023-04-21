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
#include <widget/str_utilts.h>

LyricsShowWidget::LyricsShowWidget(QWidget* parent) 
	: WheelableWidget(false, parent)
	, pos_(0)
	, last_lyric_index_(0)
	, item_percent_(0)
	, lrc_color_(Qt::darkGray)
    , lrc_highlight_color_(Qt::black) {
    Initial();
}

void LyricsShowWidget::ResizeFontSize() {
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

	XAMP_LOG_DEBUG("Max length lrc: {}", String::ToString((*itr).lrc));
	lrc_font_.setPointSize(font_size);

	lrc_metrics = QFontMetrics(lrc_font_);
	while (lrc_metrics.horizontalAdvance(max_lrc) > size().width()) {		
		const auto max_width = lrc_metrics.horizontalAdvance(max_lrc);
		font_size -= 5;
		lrc_font_.setPointSize(font_size);
		lrc_metrics = QFontMetrics(lrc_font_);
		XAMP_LOG_DEBUG("Change font size => {}, width: {}", font_size, max_width);
	}
}

void LyricsShowWidget::resizeEvent(QResizeEvent* event) {
	ResizeFontSize();
}

void LyricsShowWidget::Initial() {
    lrc_font_ = font();
	lrc_font_.setPointSize(AppSettings::ValueAsInt(kLyricsFontSize));

	ResizeFontSize();
	SetDefaultLrc();

	SetLrcColor(AppSettings::ValueAsColor(kLyricsTextColor));
	SetLrcHighLight(AppSettings::ValueAsColor(kLyricsHighLightTextColor));

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &LyricsShowWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<LyricsShowWidget> action_map(this);
		(void)action_map.AddAction(tr("Show original lyrics"), [this]() {
			lrc_ = orilyrc_;
			LoadLrc(lrc_);

		});

		(void)action_map.AddAction(tr("Show translate lyrics"), [this]() {
			lrc_ = trlyrc_;
			LoadLrc(lrc_);
			ResizeFontSize();
		});
#if 0
		(void)action_map.AddAction(tr("Set font size(small)"), [this]() {
			AppSettings::SetValue(kLyricsFontSize, 12);
			lrc_font_.setPointSize(qTheme.GetFontSize(12));
			});

		(void)action_map.AddAction(tr("Set font size(middle)"), [this]() {
			AppSettings::SetValue(kLyricsFontSize, 16);
			lrc_font_.setPointSize(qTheme.GetFontSize(16));
			});

		(void)action_map.AddAction(tr("Set font size(big)"), [this]() {
			AppSettings::SetValue(kLyricsFontSize, 24);
			lrc_font_.setPointSize(qTheme.GetFontSize(24));
			});
		(void)action_map.AddAction(tr("Change high light color"), [this]() {
			auto text_color = AppSettings::ValueAsColor(kLyricsHighLightTextColor);
			QColorDialog dlg(text_color, this);
			(void)QObject::connect(&dlg, &QColorDialog::currentColorChanged, [this](auto color) {
				SetLrcHighLight(color);
				AppSettings::SetValue(kLyricsHighLightTextColor, color);
			});
			dlg.setStyleSheet(qSTR("background-color: %1;").arg(qTheme.BackgroundColorString()));
			dlg.exec();
			});

		(void)action_map.AddAction(tr("Change text color"), [this]() {
			auto text_color = AppSettings::ValueAsColor(kLyricsTextColor);
			QColorDialog dlg(text_color, this);
			(void)QObject::connect(&dlg, &QColorDialog::currentColorChanged, [this](auto color) {
				SetLrcColor(color);
				AppSettings::SetValue(kLyricsTextColor, color);
				});
			dlg.setStyleSheet(qSTR("background-color: %1;").arg(qTheme.BackgroundColorString()));
			dlg.exec();
			});
#endif

		action_map.exec(pt);
		});

	setAcceptDrops(true);
}

void LyricsShowWidget::SetBackgroundColor(QColor color) {
	background_color_ = color;
}

void LyricsShowWidget::SetDefaultLrc() {
	LyricEntry entry;
	entry.lrc = tr("Not found lyrics").toStdWString();
	lyric_.AddLrc(entry);
}

void LyricsShowWidget::SetCurrentTime(const int32_t time, const bool is_adding) {
	auto time2 = time;

	if (!is_adding) {
	    time2 = (-time2);
	}

	for (auto& lrc : lyric_) {
	    lrc.timestamp += std::chrono::milliseconds(time2);
	}
}

void LyricsShowWidget::PaintItem(QPainter* painter, const int32_t index, QRect& rect) {
	const int32_t ih = ItemHeight() * 1.2 / 10;
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
	const auto text = QString::fromStdWString(lyric_.LineAt(index).lrc);

	painter->drawText((rect.width() - metrics.width(text)) / 2,
		rect.y() + (rect.height() - metrics.height()) / 2,
		metrics.width(text),
		rect.height(),
		Qt::AlignLeft, text);
}

void LyricsShowWidget::PaintBackground(QPainter* painter) {
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::PaintItemMask(QPainter* painter) {
	if (item_offset_ == 0) {
		painter->setFont(current_mask_font_);
		painter->setPen(lrc_highlight_color_);

		const QFontMetrics metrics(current_mask_font_);
		painter->drawText((current_roll_rect_.width() - metrics.width(real_current_text_)) / 2,
			current_roll_rect_.y() + (current_roll_rect_.height() - metrics.height()) / 2,
			mask_length_,
			current_roll_rect_.height(),
			Qt::AlignLeft,
			real_current_text_);
	}
}

int32_t LyricsShowWidget::ItemHeight() const {
	const QFontMetrics metrics(lrc_font_);
    return static_cast<int32_t>(metrics.height() * 1.5);
}

int32_t LyricsShowWidget::ItemCount() const {
	return lyric_.GetSize();
}

void LyricsShowWidget::stop() {
	mask_length_ = -1000;
	last_lyric_index_ = -1;
	current_roll_rect_ = QRect(0, 0, 0, 0);
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
			LoadLrcFile(url.toLocalFile());
			break;
		}
		event->acceptProposedAction();
	}
}

bool LyricsShowWidget::LoadLrcFile(const QString &file_path) {
	const QFileInfo file_info(file_path);

	const auto lrc_path = file_info.path()
		+ qTEXT("/")
		+ file_info.completeBaseName()
		+ qTEXT(".lrc");

	stop();
	if (!lyric_.ParseFile(lrc_path.toStdWString())) {
		SetDefaultLrc();
		return false;
	}
	ResizeFontSize();
	update();
	return true;
}

void LyricsShowWidget::AddFullLrc(const QString& lrc, std::chrono::milliseconds duration) {
    auto i = 0;
	const auto lyrics = lrc.split(qTEXT("\n"));
	const auto min_duration = duration / lyrics.count();

	for (const auto &ly : lyrics) {
		LyricEntry l;
		l.index = i++;
		l.lrc = ly.toStdWString();
		l.timestamp = min_duration * i;
		lyric_.AddLrc(l);
	}
}

void LyricsShowWidget::LoadLrc(const QString& lrc) {
	std::wistringstream stream{ lrc.toStdWString() };
	if (!lyric_.Parse(stream)) {
		SetDefaultLrc();
	}
	ResizeFontSize();
	SetLrcTime(0);
	update();
}

void LyricsShowWidget::SetLrc(const QString &lrc, const QString& trlyrc) {
	orilyrc_ = lrc;
	trlyrc_ = trlyrc;
	lrc_ = orilyrc_;
	stop();
	LoadLrc(lrc_);
}

void LyricsShowWidget::SetLrcTime(int32_t stream_time) {
	stream_time = stream_time + kScrollTime;
	pos_ = stream_time;

	if (is_scrolled_ || !lyric_.GetSize()) {
		update();
		return;
	}

    const auto &ly = lyric_.GetLyrics(std::chrono::milliseconds(stream_time));

	if (item_ != ly.index && item_offset_ == 0) {
		mask_length_ = -1000;
		current_roll_rect_ = QRect(0, 0, 0, 0);
		real_current_text_ = QString::fromStdWString(ly.lrc);
		item_offset_ = -1;
		ScrollTo(ly.index);
	}

	if (item_offset_ != 0) {
		update();
		return;
	}

    const auto &post_ly = lyric_.GetLyrics(std::chrono::milliseconds(pos_));

	const auto text = QString::fromStdWString(post_ly.lrc);
	const auto interval = post_ly.index;
	const auto precent = static_cast<float>(post_ly.index) / static_cast<float>(lyric_.GetSize());

	QFontMetrics metrics(current_mask_font_);

	if (item_percent_ == precent) {
		const auto count = static_cast<double>(interval) / 25.0;
		const float lrc_mask_mini_step = metrics.width(text) / count;
		mask_length_ += lrc_mask_mini_step;
	}
	else {
		mask_length_ = metrics.width(real_current_text_) * precent;
	}

	item_percent_ = precent;
	update();
}

void LyricsShowWidget::SetLrcFont(const QFont & font) {
	lrc_font_ = font;
	current_mask_font_ = font;
	update();
}

void LyricsShowWidget::SetLrcHighLight(const QColor & color) {
	lrc_highlight_color_ = color;
	update();
}

void LyricsShowWidget::SetLrcColor(const QColor& color) {
	lrc_color_ = color;
	update();
}
