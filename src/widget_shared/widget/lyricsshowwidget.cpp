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
	constexpr int kMinFontSize = 8;

	while (lrc_metrics.horizontalAdvance(max_lrc) > size().width() - 30) {
		font_size -= 5;
		if (font_size < kMinFontSize) {
			font_size = kMinFontSize;
			break;
		}
		lrc_font_.setPointSize(font_size);
		lrc_metrics = QFontMetrics(lrc_font_);
	}
}

void LyricsShowWidget::resizeEvent(QResizeEvent* event) {
	resizeFontSize();
	WheelableWidget::resizeEvent(event);
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
	// （1）基本檢查
	if (!lyric_) {
		return;
	}
	// index 超出範圍就不畫
	const int32_t total_count = lyric_->getSize();
	if (index < 0 || index >= total_count) {
		return;
	}

	// （2）字型微調邏輯 (維持你的原本邏輯，也可用浮點來計算)
	// ----------------------------------------------------------------------
	// 先從基底字型出發
	QFont baseFont = lrc_font_;
	double baseFontSize = baseFont.pointSizeF();
	if (baseFontSize <= 0.0) {
		baseFontSize = 16.0;
		baseFont.setPointSizeF(baseFontSize);
	}

	// 根據 item_ / item_offset_ 來做大小或加粗
	// 這裡示範一個簡單寫法，可依你的需要再優化
	double scaleFactor = 0.12; // 相當於舊程式中的 1.2 / 10
	double ih = itemHeight() * scaleFactor;
	double ch = static_cast<double>(item_offset_) * scaleFactor;

	double newFontSize = baseFontSize;
	bool isCurrentLine = (index == item_);
	bool isNextLine = (index == item_ + 1);

	if (isCurrentLine) {
		if (item_offset_ == 0) {
			newFontSize = baseFontSize + ih;
		}
		else {
			newFontSize = baseFontSize + ih - ch;
		}
	}
	else if (isNextLine) {
		// 做較小幅度增大
		newFontSize = baseFontSize + ch;
	}
	// 防止字型縮得太小或過大
	const double minFontSize = 8.0;
	const double maxFontSize = 72.0;
	if (newFontSize < minFontSize) {
		newFontSize = minFontSize;
	}
	else if (newFontSize > maxFontSize) {
		newFontSize = maxFontSize;
	}

	// 決定要不要 bold、要不要高亮顏色
	QFont newFont = baseFont;
	newFont.setPointSizeF(newFontSize);
	painter->setFont(newFont);

	QColor penColor = lrc_color_;
	if (isCurrentLine && item_offset_ == 0) {
		// 正好在中間行、不偏移時，整行可視為高亮
		penColor = lrc_highlight_color_;
	}
	painter->setPen(penColor);

	// （3）取得這行對應的 LyricEntry 與逐字資訊
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// （4）若沒有逐字資訊，維持原本一次畫完整行
	if (words.empty()) {
		// 如果你原本有 Furigana 邏輯，可在這裡直接處理
		// --- 無 Furigana 或 furiganas_.empty() 時 ---
		const QFontMetrics metrics(painter->font());
		const auto text = QString::fromStdWString(entry.lrc);

		if (furiganas_.empty() || furiganas_[index].empty()) {
			// 直接畫一整段
			painter->drawText(
				(rect.width() - metrics.horizontalAdvance(text)) / 2,
				rect.y() + (rect.height() - metrics.height()) / 2,
				text
			);
			return;
		}
		else {
			// 與你先前相同的 Furigana 繪製
			// (此時不做「逐字 partial highlight」，因為 words 為 empty)
			auto x = (rect.width() - metrics.horizontalAdvance(text)) / 2;
			QFont kanji_font = newFont;
			QFont furigana_font = newFont;
			furigana_font.setPointSizeF(newFont.pointSizeF() * 0.5);

			QFontMetrics furigana_metrics(furigana_font);
			auto furigana_result = furiganas_[index];

			for (const auto& entity : furigana_result) {
				auto kanji_text = QString::fromStdWString(entity.text);
				auto kanji_width = metrics.horizontalAdvance(kanji_text);
				auto furigana_length = entity.furigana.size();

				// 先畫 Furigana
				if (furigana_length > 0) {
					painter->setFont(furigana_font);
					double furigana_char_width = static_cast<double>(kanji_width) / furigana_length;
					int furigana_y = rect.y()
						+ (rect.height() - metrics.height()) / 2
						- furigana_metrics.height();
					for (int i = 0; i < furigana_length; ++i) {
						auto furigana_char = QString::fromStdWString(entity.furigana).mid(i, 1);
						painter->drawText(x + i * furigana_char_width, furigana_y, furigana_char);
					}
				}
				// 再畫漢字
				painter->setFont(kanji_font);
				painter->drawText(
					x,
					rect.y() + (rect.height() - metrics.height() + 20) / 2,
					kanji_text
				);
				x += kanji_width;
			}
			return;
		}
	}

	// （5）若該行有逐字資訊，進行「卡拉 OK 式逐字繪製」
	// -----------------------------------------------------------
	// 計算該行已經唱了多少毫秒
	// 假設 pos_ 為「全局播放時間(毫秒)」
	qint64 globalTime = pos_; // 你的程式可能是 int32_t，也可能是 long long
	qint64 lineStart = entry.timestamp.count();
	qint64 delta = globalTime - lineStart;
	// delta < 0 => 尚未唱到這行
	// delta > (end_time - start_time) => 已經唱完

	// 算出該行所有 word 總寬度，方便置中
	QFontMetrics fm(painter->font());
	int totalWidth = 0;
	for (auto& w : words) {
		auto wordText = QString::fromStdWString(w.content);
		totalWidth += fm.horizontalAdvance(wordText);
	}

	int x = (rect.width() - totalWidth) / 2;
	// 建議用基線繪製
	int baseline = rect.y() + (rect.height() + fm.ascent()) / 2;

	// 如果需要再跟 Furigana 整合，可把這裡拆得更複雜，示範以「逐字文字 content」為主
	// --------------------------------------------------------------------------
	for (auto& w : words) {
		QString wordText = QString::fromStdWString(w.content);
		int wordWidth = fm.horizontalAdvance(wordText);

		// 此字在行內的開始/結束時間
		qint64 wStart = w.offset.count();         // 相對該行開頭
		qint64 wEnd = wStart + w.length.count();

		// 算出目前播放到這個字的「完成度」0~1
		double fraction = 0.0;
		if (delta <= wStart) {
			fraction = 0.0;    // 尚未唱到
		}
		else if (delta >= wEnd) {
			fraction = 1.0;    // 已唱完
		}
		else {
			fraction = double(delta - wStart) / double(wEnd - wStart);
		}
		if (fraction < 0.0) fraction = 0.0;
		if (fraction > 1.0) fraction = 1.0;

		// 先畫「未唱完」顏色 (penColor)
		painter->setPen(penColor);
		painter->drawText(x, baseline, wordText);

		// 若 fraction > 0，代表有已唱部分，用 highLight color 疊上去
		if (fraction > 0.0) {
			painter->save();
			//painter->setPen(lrc_highlight_color_);
			painter->setPen(QColor(77, 208, 225, 200));

			// clipRect: 只顯示字體左側 fraction 區域
			int highlightWidth = static_cast<int>(wordWidth * fraction);
			painter->setClipRect(x, rect.y(), highlightWidth, rect.height());

			painter->drawText(x, baseline, wordText);
			painter->restore();
		}

		x += wordWidth;
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
