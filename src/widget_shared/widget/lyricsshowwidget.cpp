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
#include <QtGlobal>

#include <base/charset_detector.h>

#include <widget/widget_shared.h>
#include <widget/lrcparser.h>
#include <widget/webvttparser.h>
#include <widget/krcparser.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

#include <algorithm>

namespace {
	constexpr int kRubySpacing = 5;

	struct RubyLayoutSegment {
		QString text;
		QString ruby;
		int text_width{ 0 };
		int ruby_width{ 0 };
		int column_width{ 0 };
	};

	QSharedPointer<ILrcParser> makeLrcParser(const QString& file_path, QString& lrc_path, bool& use_default) {
		const QFileInfo file_info(file_path);
		const QString file_dir = file_info.path();
		const QString base_name = file_info.completeBaseName();
		const QString suffix = file_info.completeSuffix();

		lrc_path = file_dir + "/"_str + base_name;
		std::function<QSharedPointer<ILrcParser>()> make_parser_func;

		const OrderedMap<QString, std::function<QSharedPointer<ILrcParser>()>> lrc_parser_map{
			{
				".lrc"_str, []() {
				return QSharedPointer<ILrcParser>(new LrcParser());
				}
			},
			{
				".vtt"_str, []() {
				return QSharedPointer<ILrcParser>(new WebVTTParser());
				}
			},
			{
				".krc"_str, []() {
				return QSharedPointer<ILrcParser>(new KrcParser());
				}
			},
		};

		for (const auto& parser_pair : lrc_parser_map) {
			// Path like "C:/filename.lrc"
			auto pattern1 = lrc_path + parser_pair.first;
			// Path like "C:/filename.mp3.lrc"
			auto pattern2 = lrc_path + "."_str + suffix + parser_pair.first;
			if (QFileInfo::exists(pattern1)) {
				lrc_path = lrc_path + parser_pair.first;
				make_parser_func = parser_pair.second;
				break;
				
			}
			else if (QFileInfo::exists(pattern2)) {
				lrc_path = lrc_path + "."_str + suffix + parser_pair.first;
				make_parser_func = parser_pair.second;
				break;
			}
		}

		if (!make_parser_func) {
			// Create default parser, make GUI happy!
			use_default = true;
			return QSharedPointer<ILrcParser>(new LrcParser());
		}
		return make_parser_func();
	}

	std::vector<RubyLayoutSegment> makeRubyLayout(
		const std::vector<FuriganaEntity>& furiganas,
		const QFontMetrics& text_metrics,
		const QFontMetrics& ruby_metrics) {
		std::vector<RubyLayoutSegment> layout;
		layout.reserve(furiganas.size());

		for (const auto& entity : furiganas) {
			RubyLayoutSegment segment;
			segment.text = QString::fromStdWString(entity.text);
			segment.ruby = QString::fromStdWString(entity.furigana);
			segment.text_width = text_metrics.horizontalAdvance(segment.text);
			segment.ruby_width = ruby_metrics.horizontalAdvance(segment.ruby);
			segment.column_width = (std::max)(segment.text_width, segment.ruby_width);
			layout.push_back(std::move(segment));
		}
		return layout;
	}

	int rubyLayoutWidth(const std::vector<RubyLayoutSegment>& layout) {
		auto width = 0;
		for (const auto& segment : layout) {
			width += segment.column_width;
		}
		return width;
	}

	int lyricVisualWidth(
		const LyricEntry& entry,
		int32_t index,
		const std::vector<std::vector<FuriganaEntity>>& furiganas,
		const QFontMetrics& text_metrics,
		const QFontMetrics& ruby_metrics) {
		if (index >= 0 && index < static_cast<int32_t>(furiganas.size()) && !furiganas[index].empty()) {
			return rubyLayoutWidth(makeRubyLayout(furiganas[index], text_metrics, ruby_metrics));
		}
		return text_metrics.horizontalAdvance(QString::fromStdWString(entry.lrc));
	}

	int karaokeHighlightWidth(
		const LyricEntry& entry,
		int32_t stream_time,
		const QFontMetrics& metrics) {
		const auto delta = stream_time - static_cast<int32_t>(entry.timestamp.count());
		auto highlight_width = 0.0;

		for (const auto& word : entry.words) {
			const auto text = QString::fromStdWString(word.content);
			const auto word_width = metrics.horizontalAdvance(text);
			const auto word_start = static_cast<int32_t>(word.offset.count());
			const auto word_length = static_cast<int32_t>(word.length.count());
			const auto word_end = word_start + word_length;

			if (delta <= word_start) {
				break;
			}

			if (word_length <= 0 || delta >= word_end) {
				highlight_width += word_width;
				continue;
			}

			const auto fraction = std::clamp(
				static_cast<double>(delta - word_start) / static_cast<double>(word_length),
				0.0,
				1.0);
			highlight_width += word_width * fraction;
			break;
		}

		return static_cast<int>(highlight_width + 0.5);
	}
}

LyricsShowWidget::LyricsShowWidget(QWidget* parent) 
	: WheelableWidget(false, parent)
    , lrc_color_(kNormalColor)
    , lrc_highlight_color_(kHighLightColor)
	, karaoke_highlight_color_(kKaraokeHighLightColor) {
    initial();
}

void LyricsShowWidget::resizeFontSize() {
	if (!lyric_) {
		return;
	}

	auto font_size = 16;
	constexpr int kMinFontSize = 8;
	const auto max_width = (std::max)(0, size().width() - 30);

	auto max_lyric_width = [this]() {
		QFont ruby_font = lrc_font_;
		ruby_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		const QFontMetrics text_metrics(lrc_font_);
		const QFontMetrics ruby_metrics(ruby_font);

		auto width = 0;
		auto index = 0;
		for (const auto& entry : *lyric_) {
			width = (std::max)(width,
				lyricVisualWidth(entry,
					index,
					furiganas_,
					text_metrics,
					ruby_metrics));
			++index;
		}
		return width;
	};

	lrc_font_.setPointSize(font_size);
	while (max_width > 0 && max_lyric_width() > max_width) {
		font_size -= 5;
		if (font_size < kMinFontSize) {
			font_size = kMinFontSize;
			break;
		}
		lrc_font_.setPointSize(font_size);
	}
}

void LyricsShowWidget::resizeEvent(QResizeEvent* event) {
	resizeFontSize();
	WheelableWidget::resizeEvent(event);
}

void LyricsShowWidget::initial() {
    lrc_font_ = qTheme.defaultFont();
	lrc_font_.setBold(true);
	current_mask_font_ = lrc_font_;
	lrc_font_.setPointSize(qAppSettings.valueAsInt(kLyricsFontSize));
	lyric_.reset(new LrcParser());

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

		auto* font_size_menu = action_map.addSubMenu(tr("Font size"));
		(void)font_size_menu->addAction(tr("Increase font size"), [this]() {
			auto size = lrc_font_.pointSizeF();
			if (size < 60) {
				lrc_font_.setPointSizeF(size + 5);
				//resizeFontSize();
				update();
			}
			});

		(void)font_size_menu->addAction(tr("Decrease font size"), [this]() {
			auto size = lrc_font_.pointSizeF();
			if (size > 12) {
				lrc_font_.setPointSizeF(size - 5);
				//resizeFontSize();
				update();
			}
			});

		action_map.exec(pt);
		});

	setAcceptDrops(true);

    const auto opencc_config_path = applicationPath() + "/opencc"_str;
	convert_.Load("s2tw.json", opencc_config_path.toStdString());
}

void LyricsShowWidget::setBackgroundColor(QColor color) {
	background_color_ = color;
}

QString LyricsShowWidget::parsedLyrics() const {
	std::wostringstream ostr;
	for (auto itr = lyric_->cbegin(); itr != lyric_->cend(); ++itr) {
		auto s = itr->lrc;
		auto pos = itr->lrc.find(L'\r');
		if (pos != std::wstring::npos) {
			s.erase(pos, 1);
		}
		pos = itr->lrc.find(L'\n');
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
	//entry.lrc = tr("Not Found Lyrics").toStdWString();
	lyric_->addLrc(entry);
	is_lrc_valid_ = true;
}

void LyricsShowWidget::paintItem(QPainter* painter, int32_t index, QRect& rect) {
	if (!lyric_) {
		return;
	}
	int32_t total_count = lyric_->size();
	if (index < 0 || index >= total_count) {
		return;
	}

	// 1) 先準備字型
	QFont base_font = lrc_font_;
	double base_font_size = base_font.pointSizeF();
	if (base_font_size <= 0.0) {
		base_font_size = 16.0;
		base_font.setPointSizeF(base_font_size);
	}
	painter->setFont(base_font);

	// 2) 決定預設筆色(若為當前行, 可換高亮)
	QColor pen_color = lrc_color_;
	if ((index == item_) && (item_offset_ == 0)) {
		pen_color = lrc_highlight_color_;
	}
	painter->setPen(pen_color);

	// 3) 取得該行資料與逐字資訊
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// 4) 準備 Furigana 與一般字體的 Metrics
	QFont furigana_font = lrc_font_;
	QFont kanji_font = lrc_font_;
	QFontMetrics furigana_metrics(furigana_font);
	QFontMetrics metrics(painter->font());

	qint64 global_time = pos_;
	qint64 line_start = entry.timestamp.count();

	if ((is_fulled_ || is_lrc_valid_) && words.empty()) {
		const QString text = QString::fromStdWString(entry.lrc);

		// 置中計算 (以行寬 - 文字寬) / 2
		const int text_width = metrics.horizontalAdvance(text);
		const double x = (rect.width() - text_width) / 2.0;
		// y 基準線：在該行矩形的垂直置中
		const int baseline = rect.y() + (rect.height() + metrics.ascent()) / 2;

		// 繪製主行歌詞
		painter->drawText(x, baseline, text);

		// 如果此行還有翻譯 (tlrc)，就再畫一行
		if (!entry.tlrc.empty()) {
			// 可依需求縮小翻譯字體
			QFont translation_font = base_font;
			const float translation_scale = 0.8f;
			translation_font.setPointSizeF(base_font.pointSizeF() * translation_scale);
			painter->setFont(translation_font);

			QFontMetrics tm(translation_font);
			const QString tr_text = QString::fromStdWString(entry.tlrc);
			const int tr_width = tm.horizontalAdvance(tr_text);

			// 下方再留個行距，比如 5
			const int translation_baseline = baseline
				+ tm.height()
				+ 5;

			const double x_tr = (rect.width() - tr_width) / 2.0;
			painter->setPen(pen_color);
			painter->drawText(x_tr, translation_baseline, tr_text);

			// 記得恢復原字體
			painter->setFont(base_font);
		}
		return;
	}

	// ------------------------------------------------------------------------
	// (A) 若沒有逐字資訊 (words.empty())，整行繪製
	// ------------------------------------------------------------------------
	if (words.empty()) {
		if (!furiganas_.empty() && index < static_cast<int32_t>(furiganas_.size())) {
			// Furigana 字體縮小
			furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
			furigana_metrics = QFontMetrics(furigana_font);
			const auto& furigana_result = furiganas_[index];
			const auto ruby_layout = makeRubyLayout(furigana_result, metrics, furigana_metrics);
			double x = (rect.width() - rubyLayoutWidth(ruby_layout)) / 2.0;

			if (global_time >= line_start) {
				// 當前行 => 用同一個高亮色
				painter->setPen(lrc_highlight_color_);
			}
			else {
				// 其他行 => 用正常顏色
				painter->setPen(lrc_color_);
			}

			const int content_height = furigana_metrics.height() + kRubySpacing + metrics.height();
			const int content_top = rect.y() + (rect.height() - content_height) / 2;
			const int furigana_baseline = content_top + furigana_metrics.ascent();
			const int text_baseline = content_top + furigana_metrics.height() + kRubySpacing + metrics.ascent();

			for (const auto& segment : ruby_layout) {
				// 先繪製 Furigana (若有)
				if (!segment.ruby.isEmpty()) {
					painter->setFont(furigana_font);
					painter->drawText(
						x + (segment.column_width - segment.ruby_width) / 2.0,
						furigana_baseline,
						segment.ruby);
				}
				// 再繪製主字 (Kanji)
				painter->setFont(kanji_font);
				painter->drawText(
					x + (segment.column_width - segment.text_width) / 2.0,
					text_baseline,
					segment.text
				);
				x += segment.column_width;
			}
		}
		return;
	}

	if (!furiganas_.empty() && index < furiganas_.size()) {
		// ------------------------------------------------------------------------
		// (B) 有逐字資訊，則做卡拉OK逐字高亮 + Furigana
		// ------------------------------------------------------------------------
		// 5) 先繪製整行 Furigana (不需要對 words 做 1:1 大小)
		furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		furigana_metrics = QFontMetrics(furigana_font);
		const auto& furigana_result = furiganas_[index];
		const auto ruby_layout = makeRubyLayout(furigana_result, metrics, furigana_metrics);
		double x_f = (rect.width() - rubyLayoutWidth(ruby_layout)) / 2.0;
		const int content_height = furigana_metrics.height() + kRubySpacing + metrics.height();
		const int content_top = rect.y() + (rect.height() - content_height) / 2;
		const int furigana_baseline = content_top + furigana_metrics.ascent();
		const int base_line = content_top + furigana_metrics.height() + kRubySpacing + metrics.ascent();

		if (global_time >= line_start) {
			// 當前行 => 用同一個高亮色
			painter->setPen(lrc_highlight_color_);
		}
		else {
			// 其他行 => 用正常顏色
			painter->setPen(lrc_color_);
		}

		// 先用較小字體繪 Furigana
		painter->setFont(furigana_font);
		for (const auto& segment : ruby_layout) {
			// 逐字畫出 Furigana
			if (!segment.ruby.isEmpty()) {
				painter->drawText(
					x_f + (segment.column_width - segment.ruby_width) / 2.0,
					furigana_baseline,
					segment.ruby);
			}
			x_f += segment.column_width;
		}
	}

	// 6) 再用卡拉OK方式畫主文字
	qint64 delta = global_time - line_start;

	painter->setFont(lrc_font_);
	QFontMetrics fm(painter->font());

	// 計算該行所有 words 的總寬度 (置中)
	int total_width = 0;
	for (auto& w : words) {
		QString w_text = QString::fromStdWString(w.content);
		total_width += fm.horizontalAdvance(w_text);
	}
	double x = (rect.width() - total_width) / 2.0;
	int baseline = rect.y() + (rect.height() + fm.ascent()) / 2;

	// 逐字繪製 + 高亮
	for (int i = 0; i < static_cast<int>(words.size()); ++i) {
		const auto& w = words[i];
		QString word_text = QString::fromStdWString(w.content);
		int word_width = fm.horizontalAdvance(word_text);

		// 先畫「未亮」顏色
		painter->setPen(pen_color);
		painter->drawText(x, baseline, word_text);

		// 計算此字的高亮 fraction
		qint64 w_start = w.offset.count();
		qint64 w_end = w_start + w.length.count();
		double fraction = 0.0;
		if (delta <= w_start) {
			fraction = 0.0;
		}
		else if (delta >= w_end) {
			fraction = 1.0;
		}
		else {
			fraction = static_cast<double>(delta - w_start) / static_cast<double>(w_end - w_start);
		}
		fraction = std::clamp(fraction, 0.0, 1.0);

		// 如果 fraction>0 就用 clipRect 疊加高亮
		if (fraction > 0.0) {
			painter->save();
			painter->setPen(karaoke_highlight_color_); // 你自定義的高亮色
			int highlight_width = static_cast<int>(word_width * fraction);
			painter->setClipRect(x, rect.y(), highlight_width, rect.height());
			painter->drawText(x, baseline, word_text);
			painter->restore();
		}
		x += word_width;
	}

	// 7) 繪製翻譯行 (如果有)
	if (!entry.tlrc.empty()) {
		QColor translation_color;
		if (global_time >= line_start) {
			// 當前行 => 用同一個高亮色
			translation_color = lrc_highlight_color_;
		}
		else {
			// 其他行 => 用正常顏色
			translation_color = lrc_color_;
		}

		float  translation_scale_ = 0.8f;                 // 翻譯行字體相對主行大小
		int    translation_line_spacing_ = 5;             // 主行與翻譯行之間的垂直間距(像素)

		// 取得翻譯文字
		const QString translation_text = QString::fromStdWString(entry.tlrc);

		// 設定翻譯行字體 (可與主行不同大小)
		QFont translation_font = lrc_font_;
		translation_font.setPointSizeF(lrc_font_.pointSizeF()* translation_scale_);
		painter->setFont(translation_font);

		// 重新計算字體高度
		QFontMetrics tm(translation_font);

		// 決定翻譯行的基準線(在主行 baseline 再往下移一些)
		int translation_baseline = baseline
			+ tm.height()
			+ translation_line_spacing_; // 加點間距

		// 置中計算 (讓翻譯跟主行都置中)
		int translation_width = tm.horizontalAdvance(translation_text);
		double x_trans = (rect.width() - translation_width) / 2.0;

		// 設定顏色(與主行顏色不同也可)
		painter->setPen(translation_color);

		// 繪製翻譯文字
		painter->drawText(x_trans, translation_baseline, translation_text);
	}
}

void LyricsShowWidget::paintBackground(QPainter* painter) {
	if (!background_color_.isValid()) {
		return;
	}
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::paintItemMask(QPainter* painter) {
}

int32_t LyricsShowWidget::itemHeight() const {
	const QFontMetrics metrics(lrc_font_);
	auto height = static_cast<int32_t>(metrics.height() * 1.5);

	if (!furiganas_.empty()) {
		QFont furigana_font = lrc_font_;
		furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		const QFontMetrics furigana_metrics(furigana_font);
		height += furigana_metrics.height() + kRubySpacing;
	}

	if (lyric_->hasTranslation()) {
		height += static_cast<int32_t>(metrics.height() * 0.8) + 5;
	}

    return height;
}

int32_t LyricsShowWidget::itemCount() const {
	if (!lyric_) {
		return 0;
	}
	return lyric_->size();
}

void LyricsShowWidget::stop() {
	furiganas_.clear();
	mask_length_ = -1000;
	item_ = 0;
	item_offset_ = 0;
	item_percent_ = 0;
	last_lyric_index_ = -1;
	last_karaoke_index_ = -1;
	last_karaoke_highlight_width_ = -1;
	pos_ = 0;
	is_scrolled_ = false;
	do_signal_ = true;
	is_lrc_valid_ = false;
	is_fulled_ = false;
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
			loadFile(url.toLocalFile());
			break;
		}
		event->acceptProposedAction();
	}
}

void LyricsShowWidget::loadFromParser(const QSharedPointer<ILrcParser>& parser) {
	lyric_ = parser;
	furiganas_.clear();
	
	for (auto& lrc : *lyric_) {
		if (language_detector_.IsJapanese(lrc.lrc)) {
			furiganas_.push_back(furigana_.Convert(lrc.lrc));
		} else {
			// 這裡要補空的，否則會造成 index 不一致.
			furiganas_.emplace_back();
		}
		if (language_detector_.IsChinese(lrc.lrc)) {
			lrc.lrc = convert_.Convert(lrc.lrc);
			for (auto& word : lrc.words) {
				word.content = convert_.Convert(word.content);
			}
			lrc.lrc = convert_.Convert(lrc.lrc);
		}
		lrc.tlrc = convert_.Convert(lrc.tlrc);
	}
	is_lrc_valid_ = true;
	is_fulled_ = false;
	setCurrentIndex(0);
	resizeFontSize();
	update();
}

bool LyricsShowWidget::isValid() const {
	return is_lrc_valid_;
}

bool LyricsShowWidget::loadFile(const QString &file_path) {
	stop();

	auto use_default = false;
	QString lrc_path;

	auto parser = makeLrcParser(file_path, 
		lrc_path, 
		use_default);

	if (use_default || !parser->parseFile(lrc_path.toStdWString())) {
		setDefaultLrc();
		return false;
	}

	loadFromParser(parser);
	return true;
}

void LyricsShowWidget::setFullLrc(const QString& lrc, double duration) {
	// 1) 停止並清空舊歌詞資料
	stop();

	// 2) 以換行分割整段文字，可視需求是否跳過空行
	//    這裡如果要顯示空行，也可以改成 Qt::KeepEmptyParts
	const auto lines = lrc.split(L'\n', Qt::KeepEmptyParts);

	// 若沒有任何行，則顯示預設「無歌詞」
	if (lines.isEmpty()) {
		setDefaultLrc();
		return;
	}

	// 3) 可依照傳進來的 duration，計算每行要分配的「模擬時間」(可選)
	//    若只想所有行都同時顯示，可直接都設定 timestamp = 0
	double time_per_line = 0.0;
	if (duration > 0.0 && lines.size() > 1) {
		time_per_line = duration / lines.size();
	}

	double current_time_sec = 0.0;

	// 4) 建立每一行的 LyricEntry，沒有實際時間戳則可以全設 0
	for (int i = 0; i < lines.size(); ++i) {
		LyricEntry entry;
		entry.index = i;
		entry.lrc = lines.at(i).toStdWString();

		// 若想完全無時間戳，就用 0；若想模擬有時間序，則可使用下列寫法
		/*entry.timestamp = std::chrono::milliseconds(
			static_cast<int>(current_time_sec * 1000.0)
		);*/

		// 加入容器
		lyric_->addLrc(entry);

		// 下行若要模擬遞增，可再累加
		current_time_sec += time_per_line;
	}

	// 5) 標記為「純文字模式」
	is_fulled_ = true;
	is_lrc_valid_ = true;
	setCurrentIndex(0);

	// 6) 重新繪製
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
		is_lrc_valid_ = true;
		is_fulled_ = false;
		LanguageDetector detector;
		for (auto& lrc : *lyric_) {
			if (detector.IsJapanese(lrc.lrc)) {
				furiganas_.push_back(furigana_.Convert(lrc.lrc));
			} else {
				// 這裡要補空的，否則會造成 index 不一致.
				furiganas_.emplace_back();
				lrc.tlrc = convert_.Convert(lrc.tlrc);
			}
		}
	}
	resizeFontSize();
	setLrcTime(0);
	update();
}

void LyricsShowWidget::setLrc(const QString &lrc, const QString& trlyrc) {
	orilyrc_ = lrc;
	trlyrc_ = trlyrc;
	lrc_ = orilyrc_;
	stop();
	loadLrc(lrc_);
}

QRect LyricsShowWidget::itemBoundingRect(int index, int offset) const {
	const int w = width();
	const int h = height();
	const int iH = itemStepHeight();

	// 與 paintEvent() 內部一致：
	//   "中心行" item_ 大約畫在 y = h/2 - item_offset_
	//   => 這行與 item_ 的差 = (index - item_)，然後乘 iH
	int diff = (index - item_);
	int lineY = (h / 2) + diff * iH - offset;

	return {0, lineY, w, iH};
}

void LyricsShowWidget::setLrcTime(int32_t stream_time) {
	if (stop_scroll_time_) {
		return;
	}

	stream_time = stream_time + kScrollTime;
	pos_ = stream_time;

	if (is_scrolled_) {
		update();
		return;
	}

	if (is_fulled_ || !lyric_ || !lyric_->size()) {
		return;
	}

    const auto& ly =
		lyric_->getLyrics(std::chrono::milliseconds(stream_time));

	auto line_changed = false;
	if (last_lyric_index_ != ly.index && item_offset_ == 0) {
		mask_length_ = -1000;
		current_roll_rect_ = QRect(0, 0, 0, 0);
		real_current_text_ = QString::fromStdWString(ly.lrc);
		last_lyric_index_ = ly.index;
		last_karaoke_index_ = -1;
		last_karaoke_highlight_width_ = -1;
		line_changed = true;
		onScrollTo(ly.index);
	}

	if (item_offset_ != 0) {
		update();
		return;
	}

	if (ly.words.empty()) {
		if (line_changed) {
			update();
		}
		return;
	}

	const QFontMetrics metrics(lrc_font_);
	const auto highlight_width = karaokeHighlightWidth(ly, stream_time, metrics);
	if (line_changed
		|| last_karaoke_index_ != ly.index
		|| qAbs(highlight_width - last_karaoke_highlight_width_) >= 1) {
		last_karaoke_index_ = ly.index;
		last_karaoke_highlight_width_ = highlight_width;
		update();
	}
}

void LyricsShowWidget::setLrcFont(const QFont & font) {
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

void LyricsShowWidget::setKaraokeHighlightColor(const QColor& color) {
	karaoke_highlight_color_ = color;
	update();
}
