#include <widget/lyricsshowwidget.h>

#include <sstream>
#include <QPainter>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QPainterPath>

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
	// 1) 基本檢查
	if (!lyric_) {
		return;
	}
	const int32_t total_count = lyric_->getSize();
	if (index < 0 || index >= total_count) {
		return;
	}

	// 2) 字型、顏色等設定 (略)
	QFont baseFont = lrc_font_;
	double baseFontSize = baseFont.pointSizeF();
	if (baseFontSize <= 0.0) {
		baseFontSize = 16.0;
		baseFont.setPointSizeF(baseFontSize);
	}

	// (根據 item_ / item_offset_ 動態微調大小的程式，略...)
	painter->setFont(baseFont);

	// 預設筆色
	QColor penColor = lrc_color_;
	if ((index == item_) && (item_offset_ == 0)) {
		// 中間行、無 offset 時使用高亮色
		penColor = lrc_highlight_color_;
	}
	painter->setPen(penColor);

	// 3) 取得該行的 LyricEntry & words
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// 4) 若沒有 words，就用整行繪製 (和你現有程式相同)
	if (words.empty()) {
		// ...省略 (保持原有邏輯)
		// 仍可在這裡處理行級 furiganas_，如你原本的方式
		return;
	}

	// 5) 有逐字資訊 -> 「卡拉 OK」部份高亮 + Furigana
	// ----------------------------------------------------
	qint64 globalTime = pos_;
	qint64 lineStart = entry.timestamp.count();
	qint64 delta = globalTime - lineStart;

	QFontMetrics fm(painter->font());
	// 先算總寬度
	int totalWidth = 0;
	for (auto& w : words) {
		QString wText = QString::fromStdWString(w.content);
		totalWidth += fm.horizontalAdvance(wText);
	}
	// 置中 x, baseline
	int x = (rect.width() - totalWidth) / 2;
	int baseline = rect.y() + (rect.height() + fm.ascent()) / 2;

	// 準備 Furigana 用的字型
	QFont furiganaFont = baseFont;
	furiganaFont.setPointSizeF(baseFontSize * 0.5);  // 假名一般較小
	QFontMetrics furiganaFm(furiganaFont);

	// 6) 逐字繪製
	// ----------------------------------------------------
	// 假設 furiganas_[index] 與 words.size() 相同，或至少不小於 words.size()。
	auto& furiganaList = furiganas_[index];
	// ※若不對應，需自行加判斷

	for (int i = 0; i < (int)words.size(); i++) {
		const auto& w = words[i];
		QString wordText = QString::fromStdWString(w.content);
		int wordWidth = fm.horizontalAdvance(wordText);

		// 計算 fraction (已唱比)
		qint64 wStart = w.offset.count();
		qint64 wEnd = wStart + w.length.count();
		double fraction = 0.0;
		if (delta <= wStart) fraction = 0.0;
		else if (delta >= wEnd)   fraction = 1.0;
		else {
			fraction = double(delta - wStart) / double(wEnd - wStart);
		}
		fraction = std::clamp(fraction, 0.0, 1.0);

		// (A) 先畫未唱色 (penColor)
		painter->setPen(penColor);
		painter->drawText(x, baseline, wordText);

		// (B) 若 fraction > 0，部分高亮
		if (fraction > 0.0) {
			painter->save();
			painter->setPen(QColor(77, 208, 225, 255));
			int highlightWidth = static_cast<int>(wordWidth * fraction);
			painter->setClipRect(x, rect.y(), highlightWidth, rect.height());
			painter->drawText(x, baseline, wordText);
			painter->restore();
		}

		// (C) 畫 Furigana (只要完整顯示，不隨 fraction)
		// ------------------------------------------------
		// 假設 furiganaList[i] 對應這個字
		if (i < (int)furiganaList.size()) {
			const auto& fentity = furiganaList[i];  // { text, furigana }
			// 取得假名(或羅馬音)字串
			QString furiganaText = QString::fromStdWString(fentity.furigana);
			if (!furiganaText.isEmpty()) {
				painter->setFont(furiganaFont);

				// 假設希望 Furigana 置於該字上方正中
				int furiganaW = furiganaFm.horizontalAdvance(furiganaText);
				int furiganaX = x + (wordWidth - furiganaW) / 2;
				// Y = baseline - 主字的高度 - 假名本身高度
				int furiganaY = baseline - fm.height();
				// 可再調整一些偏移，視覺上更好

				// 先把筆色設回 penColor 或其他顏色
				painter->setPen(lrc_color_);
				painter->drawText(furiganaX, furiganaY, furiganaText);

				// Furigana 完成後還原字型
				painter->setFont(baseFont);
			}
		}

		x += wordWidth;  // 移動 x 到下一字
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

void LyricsShowWidget::setHighLightColor(const QColor & color) {
	lrc_highlight_color_ = color;
	update();
}

void LyricsShowWidget::setNormalColor(const QColor& color) {
	lrc_color_ = color;
	update();
}
