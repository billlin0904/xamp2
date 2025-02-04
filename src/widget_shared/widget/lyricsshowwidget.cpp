#include <widget/lyricsshowwidget.h>

#include <sstream>
#include <QPainter>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QDir>

#include <widget/widget_shared.h>
#include <widget/lrcparser.h>
#include <widget/webvttparser.h>
#include <widget/krcparser.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/util/str_util.h>

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
	if (!lyric_) {
		return;
	}

	auto font_size = 16;
	QFontMetrics lrc_metrics(lrc_font_);
    const auto itr 
		= std::max_element(lyric_->begin(), lyric_->end(),
	    [&lrc_metrics](const auto &a, const auto &b) {
		    return lrc_metrics.horizontalAdvance(QString::fromStdWString(a.lrc))
		        < lrc_metrics.horizontalAdvance(QString::fromStdWString(b.lrc));
	    });
	if (itr == lyric_->end()) {
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
	lyric_ = MakeAlign<ILrcParser, LrcParser>();

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

		(void)action_map.addAction(tr("Copy lyrics"), [this]() {
			QApplication::clipboard()->setText(parsedLyrics());
			});

		action_map.exec(pt);
		});

	setAcceptDrops(true);
}

void LyricsShowWidget::setBackgroundColor(QColor color) {
	background_color_ = color;
}

QString LyricsShowWidget::parsedLyrics() const {
	std::wostringstream ostr;
	for (auto itr = lyric_->cbegin(); itr != lyric_->cend(); ++itr) {
		auto s = itr->lrc;
		auto pos = itr->lrc.find(L"\r");
		if (pos != std::wstring::npos) {
			s.erase(pos, 1);
		}
		pos = itr->lrc.find(L"\n");
		if (pos != std::wstring::npos) {
			s.erase(pos, 1);
		}
		if (!s.empty()) {
			ostr << s << L"\r\n";
		}		
	}
	return QString::fromStdWString(ostr.str());
}

void LyricsShowWidget::setDefaultLrc() {
	if (!lyric_) {
		return;
	}
	LyricEntry entry;
	entry.lrc = tr("Not found lyrics").toStdWString();
	lyric_->addLrc(entry);
}

void LyricsShowWidget::setCurrentTime(const int32_t time, const bool is_adding) {
	auto time2 = time;

	if (!is_adding) {
	    time2 = (-time2);
	}

	for (auto& lrc : *lyric_) {
	    lrc.timestamp += std::chrono::milliseconds(time2);
	}
}

void LyricsShowWidget::paintItem(QPainter* painter, const int32_t index, QRect& rect) {
	if (!lyric_) {
		return;
	}

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
			painter->setFont(current_mask_font_);
			painter->setPen(lrc_highlight_color_);
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
	const auto text = QString::fromStdWString(lyric_->lineAt(index).lrc);

	if (furiganas_.empty()) {
		painter->drawText((rect.width() - metrics.horizontalAdvance(text)) / 2,
			rect.y() + (rect.height() - metrics.height()) / 2,
			text);
		return;
	}

	const auto furigana_result = furiganas_[index];
	if (furigana_result.empty()) {
		painter->drawText((rect.width() - metrics.horizontalAdvance(text)) / 2,
			rect.y() + (rect.height() - metrics.height()) / 2,
			text);
		return;
	}

	auto x = (rect.width() - metrics.horizontalAdvance(text)) / 2;

	QFont kanji_font = painter->font();
	QFont furigana_font = kanji_font;
	furigana_font.setPointSize(kanji_font.pointSize() * 0.5);

	QFontMetrics furigana_metrics(furigana_font);

	for (const auto& entity : furigana_result) {
		auto kanji_width = metrics.horizontalAdvance(QString::fromStdWString(entity.text));
		auto furigana_length = entity.furigana.size();

		if (furigana_length > 0) {
			auto furigana_char_width = kanji_width / furigana_length;
			auto furigana_y = rect.y() + (rect.height() - metrics.height()) / 2 - furigana_metrics.height();

			for (auto i = 0; i < furigana_length; ++i) {
				auto furigana_char = QString::fromStdWString(entity.furigana).mid(i, 1);
				auto furigana_x = x + i * furigana_char_width;

				painter->setFont(furigana_font);
				painter->drawText(furigana_x, furigana_y, furigana_char);
			}
		}

		painter->setFont(kanji_font);
		painter->drawText(x, rect.y() + (rect.height() - metrics.height() + 20) / 2, QString::fromStdWString(entity.text));
		x += kanji_width;
	}
}

void LyricsShowWidget::paintBackground(QPainter* painter) {
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::paintItemMask(QPainter* painter) {
}

int32_t LyricsShowWidget::itemHeight() const {
	const QFontMetrics metrics(lrc_font_);
    return static_cast<int32_t>(metrics.height() * 1.5);
}

int32_t LyricsShowWidget::itemCount() const {
	if (!lyric_) {
		return 0;
	}
	return lyric_->getSize();
}

void LyricsShowWidget::stop() {
	furiganas_.clear();
	mask_length_ = -1000;
	last_lyric_index_ = -1;
	current_roll_rect_ = QRect(0, 0, 0, 0);
	real_current_text_.clear();
	if (lyric_ != nullptr) {
		lyric_->clear();
	}
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

	stop();

	const QString file_dir = file_info.path();
	const QString base_name = file_info.completeBaseName();
	const QString suffix = file_info.completeSuffix();

	QString lrc_path = file_dir + QDir::separator() + base_name;
	std::function<ScopedPtr<ILrcParser>()> make_parser_func;

	const OrderedMap<QString, std::function<ScopedPtr<ILrcParser>()>> lrc_parser_map{
		{
			".lrc"_str, []() {
			return MakeAlign<ILrcParser, LrcParser>();
			}
		},
		{
			".vtt"_str, []() {
			return MakeAlign<ILrcParser, WebVTTParser>();
			}
		},
		{
			".krc"_str, []() {
			return MakeAlign<ILrcParser, KrcParser>();
			}
		},
	};

	for (const auto &parser_pair : lrc_parser_map) {
		// Path like "C:/filename.lrc"
		if (QFileInfo::exists(lrc_path + parser_pair.first)) {
			lrc_path = lrc_path + parser_pair.first;
			make_parser_func = parser_pair.second;
			break;
			// Path like "C:/filename.mp3.lrc"
		} else if (QFileInfo::exists(lrc_path + "."_str + suffix + parser_pair.first)) {
			lrc_path = lrc_path + "."_str + suffix + parser_pair.first;
			make_parser_func = parser_pair.second;
			break;
		}
	}

	if (!make_parser_func) {
		// Create default parser, make GUI happy!
		lyric_ = MakeAlign<ILrcParser, LrcParser>();
		setDefaultLrc();		
		return false;
	}

	lyric_ = make_parser_func();

	if (!lyric_->parseFile(lrc_path.toStdWString())) {
		setDefaultLrc();
		return false;
	}

	for (const auto& lrc : *lyric_) {
		furiganas_.push_back(furigana_.Convert(lrc.lrc));
	}

	resizeFontSize();
	update();
	return true;
}

void LyricsShowWidget::onAddFullLrc(const QString& lrc) {
	stop();

    auto i = 0;
	const auto lyrics = lrc.split("\n"_str);

	for (const auto &ly : lyrics) {
		LyricEntry l;
		l.index = i++;
		l.lrc = ly.toStdWString();
		lyric_->addLrc(l);
	}

	is_fulled_ = true;
	update();
}

void LyricsShowWidget::loadLrc(const QString& lrc) {
	furiganas_.clear();
	std::wistringstream stream{ lrc.toStdWString() };
	if (!lyric_->parse(stream)) {
		setDefaultLrc();
	}
	else {
		stop_scroll_time_ = false;
		for (const auto& lrc : *lyric_) {
			furiganas_.push_back(furigana_.Convert(lrc.lrc));
		}
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

	if (is_fulled_ || is_scrolled_ || !lyric_->getSize()) {
		update();
		return;
	}

    const auto &ly =
		lyric_->getLyrics(std::chrono::milliseconds(stream_time));

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

    const auto &post_ly = 
		lyric_->getLyrics(std::chrono::milliseconds(pos_));

	const auto text = QString::fromStdWString(post_ly.lrc);
	const auto interval = post_ly.index;
	const auto precent = static_cast<float>(post_ly.index) / static_cast<float>(lyric_->getSize());

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
