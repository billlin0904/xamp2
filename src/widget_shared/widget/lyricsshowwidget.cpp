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
#include <QTextLayout>
#include <QtGlobal>
#include <QMenu>

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
	constexpr int kLyricsHorizontalMargin = 30;
	constexpr int kWrappedRowSpacing = 8;
	constexpr float kTranslationScale = 0.8f;
	constexpr int kTranslationSpacing = 5;

	struct RubyLayoutSegment {
		QString text;
		QString ruby;
		int text_width{ 0 };
		int ruby_width{ 0 };
		int column_width{ 0 };
	};

	struct WrappedWordRow {
		size_t begin{ 0 };
		size_t end{ 0 };
		int width{ 0 };
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
			// create default parser, make GUI happy!
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

	std::vector<std::vector<RubyLayoutSegment>> wrapRubyLayout(
		const std::vector<RubyLayoutSegment>& layout,
		int max_width) {
		std::vector<std::vector<RubyLayoutSegment>> rows;
		if (layout.empty()) {
			return rows;
		}

		std::vector<RubyLayoutSegment> row;
		auto row_width = 0;
		const auto effective_width = (std::max)(1, max_width);
		for (const auto& segment : layout) {
			if (!row.empty() && row_width + segment.column_width > effective_width) {
				rows.push_back(std::move(row));
				row.clear();
				row_width = 0;
			}
			row_width += segment.column_width;
			row.push_back(segment);
		}
		if (!row.empty()) {
			rows.push_back(std::move(row));
		}
		return rows;
	}

	std::vector<WrappedWordRow> makeWrappedWordRows(
		const std::vector<LyricWord>& words,
		const QFontMetrics& metrics,
		int max_width) {
		std::vector<WrappedWordRow> rows;
		if (words.empty()) {
			return rows;
		}

		const auto effective_width = (std::max)(1, max_width);
		WrappedWordRow row;
		row.begin = 0;
		for (size_t i = 0; i < words.size(); ++i) {
			const auto word_width = metrics.horizontalAdvance(QString::fromStdWString(words[i].content));
			if (row.end > row.begin && row.width + word_width > effective_width) {
				rows.push_back(row);
				row.begin = i;
				row.end = i;
				row.width = 0;
			}
			row.end = i + 1;
			row.width += word_width;
		}
		if (row.end > row.begin) {
			rows.push_back(row);
		}
		return rows;
	}

	int wrappedTextLineCount(const QString& text, const QFont& font, int max_width) {
		if (text.isEmpty()) {
			return 1;
		}

		QTextLayout layout(text, font);
		QTextOption option;
		option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		layout.setTextOption(option);

		auto count = 0;
		layout.beginLayout();
		while (true) {
			auto line = layout.createLine();
			if (!line.isValid()) {
				break;
			}
			line.setLineWidth((std::max)(1, max_width));
			++count;
		}
		layout.endLayout();
		return (std::max)(1, count);
	}

	void drawCenteredWrappedText(
		QPainter* painter,
		const QRectF& rect,
		const QString& text,
		const QFont& font,
		const QColor& color) {
		painter->save();
		painter->setFont(font);
		painter->setPen(color);

		QTextOption option(Qt::AlignHCenter | Qt::AlignVCenter);
		option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		painter->drawText(rect, text, option);
		painter->restore();
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

	QFont makeLyricsDisplayFont(const QFont& font) {
		auto result = font;
		result.setBold(false);
		result.setStyleName(QString());
		result.setWeight(QFont::Black);
		return result;
	}

	QString decodeLyricsEntities(const QString& text) {
		if (!text.contains(QChar(u'&'))) {
			return text;
		}

		QString result;
		result.reserve(text.size());

		for (qsizetype i = 0; i < text.size();) {
			if (text[i] != QChar(u'&')) {
				result += text[i++];
				continue;
			}

			const auto semicolon = text.indexOf(QChar(u';'), i + 1);
			if (semicolon < 0 || semicolon - i > 32) {
				result += text[i++];
				continue;
			}

			const auto entity = text.mid(i + 1, semicolon - i - 1);
			if (entity == "amp"_str) {
				result += QChar(u'&');
			}
			else if (entity == "apos"_str) {
				result += QChar(u'\'');
			}
			else if (entity == "quot"_str) {
				result += QChar(u'"');
			}
			else if (entity == "lt"_str) {
				result += QChar(u'<');
			}
			else if (entity == "gt"_str) {
				result += QChar(u'>');
			}
			else if (entity.startsWith("#x"_str, Qt::CaseInsensitive)) {
				bool ok = false;
				const auto code_point = entity.mid(2).toUInt(&ok, 16);
				if (!ok || code_point > 0x10FFFF) {
					result += text[i++];
					continue;
				}
				const auto value = static_cast<char32_t>(code_point);
				result += QString::fromUcs4(&value, 1);
			}
			else if (entity.startsWith(QChar(u'#'))) {
				bool ok = false;
				const auto code_point = entity.mid(1).toUInt(&ok, 10);
				if (!ok || code_point > 0x10FFFF) {
					result += text[i++];
					continue;
				}
				const auto value = static_cast<char32_t>(code_point);
				result += QString::fromUcs4(&value, 1);
			}
			else {
				result += text[i++];
				continue;
			}
			i = semicolon + 1;
		}

		return result;
	}

	QString collapseConsecutiveLineBreaks(const QString& text) {
		if (!text.contains(QChar(u'\n')) && !text.contains(QChar(u'\r'))) {
			return text;
		}

		QString result;
		result.reserve(text.size());
		auto previous_was_line_break = false;

		for (const auto ch : text) {
			const auto is_line_break = ch == QChar(u'\n') || ch == QChar(u'\r');
			if (is_line_break) {
				if (!previous_was_line_break) {
					result += QChar(u'\n');
					previous_was_line_break = true;
				}
				continue;
			}
			result += ch;
			previous_was_line_break = false;
		}
		return result;
	}

	QString normalizeLyricsText(const QString& text) {
		return collapseConsecutiveLineBreaks(decodeLyricsEntities(text));
	}

	std::wstring normalizeLyricsText(const std::wstring& text) {
		return normalizeLyricsText(QString::fromStdWString(text)).toStdWString();
	}

	void decodeLyricsEntry(LyricEntry& entry) {
		entry.lrc = normalizeLyricsText(entry.lrc);
		entry.tlrc = normalizeLyricsText(entry.tlrc);
		for (auto& word : entry.words) {
			word.content = normalizeLyricsText(word.content);
		}
	}

	QString formatSrtTimestamp(std::chrono::milliseconds time) {
		if (time < std::chrono::milliseconds(0)) {
			time = std::chrono::milliseconds(0);
		}

		const auto total_ms = time.count();
		const auto hours = total_ms / 3600000;
		const auto minutes = (total_ms / 60000) % 60;
		const auto seconds = (total_ms / 1000) % 60;
		const auto milliseconds = total_ms % 1000;

		return QStringLiteral("%1:%2:%3,%4")
			.arg(hours, 2, 10, QChar(u'0'))
			.arg(minutes, 2, 10, QChar(u'0'))
			.arg(seconds, 2, 10, QChar(u'0'))
			.arg(milliseconds, 3, 10, QChar(u'0'));
	}

	QString normalizeSrtText(QString text) {
		text.replace("\r\n"_str, "\n"_str);
		text.replace(QChar(u'\r'), QChar(u'\n'));
		return text.trimmed();
	}

	QString lyricEntryTextForSrt(const LyricEntry& entry) {
		QStringList lines;
		const auto text = normalizeSrtText(QString::fromStdWString(entry.lrc));
		if (!text.isEmpty()) {
			lines.append(text);
		}

		const auto translated_text = normalizeSrtText(QString::fromStdWString(entry.tlrc));
		if (!translated_text.isEmpty()) {
			lines.append(translated_text);
		}
		return lines.join(QChar(u'\n'));
	}

	std::chrono::milliseconds lyricEntryStartTime(const LyricEntry& entry) {
		return entry.start_time.count() > 0 ? entry.start_time : entry.timestamp;
	}

	std::chrono::milliseconds lyricEntryEndTime(const LyricEntry& entry) {
		if (entry.end_time > lyricEntryStartTime(entry)) {
			return entry.end_time;
		}

		auto max_word_end = std::chrono::milliseconds(0);
		for (const auto& word : entry.words) {
			max_word_end = (std::max)(max_word_end, word.offset + word.length);
		}
		return max_word_end.count() > 0
			? lyricEntryStartTime(entry) + max_word_end
			: std::chrono::milliseconds(0);
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
	updateGeometry();
}

void LyricsShowWidget::resizeEvent(QResizeEvent* event) {
	resizeFontSize();
	WheelableWidget::resizeEvent(event);
}

void LyricsShowWidget::initial() {
    lrc_font_ = qTheme.defaultFont();
	lrc_font_.setBold(false);
	lrc_font_.setStyleName(QString());
	lrc_font_.setWeight(QFont::Black);
	current_mask_font_ = lrc_font_;
	lrc_font_.setPointSize(qAppSettings.valueAsInt(kLyricsFontSize));
	const auto saved_alpha = qAppSettings.valueAs("lyricsUnsungAlpha"_str);
	if (saved_alpha.isValid()) unsung_alpha_ = std::clamp(saved_alpha.toInt(), 0, 255);
	lyric_.reset(new LrcParser());

	resizeFontSize();
	setDefaultLrc();

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &LyricsShowWidget::customContextMenuRequested, [this](auto pt) {
		XMenu menu(this);
		(void)QObject::connect(menu.addAction(tr("Show original lyrics")), &QAction::triggered, this, [this]() {
			lrc_ = orilyrc_;
			loadLrc(lrc_);

		});

		(void)QObject::connect(menu.addAction(tr("Show translate lyrics")), &QAction::triggered, this, [this]() {
			lrc_ = trlyrc_;
			loadLrc(lrc_);
			resizeFontSize();
		});

		(void)QObject::connect(menu.addAction(tr("Copy lyrics")), &QAction::triggered, this, [this]() {
			QApplication::clipboard()->setText(parsedLyrics());
			});

		(void)QObject::connect(menu.addAction(tr("Copy lyrics (SRT fomrat)")), &QAction::triggered, this, [this]() {
			QApplication::clipboard()->setText(parsedSrtLyrics());
			});

		emit populateContextMenu(&menu);

		auto* font_size_menu = menu.addMenu(tr("Font size"));
		(void)QObject::connect(font_size_menu->addAction(tr("Increase font size")), &QAction::triggered, this, [this]() {
			auto size = lrc_font_.pointSizeF();
			if (size < 60) {
				lrc_font_.setPointSizeF(size + 5);				
				update();
				qAppSettings.setValue(kLyricsFontSize, static_cast<int>(lrc_font_.pointSizeF()));
			}
			});

		(void)QObject::connect(font_size_menu->addAction(tr("Decrease font size")), &QAction::triggered, this, [this]() {
			auto size = lrc_font_.pointSizeF();
			if (size > 12) {
				lrc_font_.setPointSizeF(size - 5);				
				update();
				qAppSettings.setValue(kLyricsFontSize, static_cast<int>(lrc_font_.pointSizeF()));
			}
			});

		menu.exec(mapToGlobal(pt));
		});

	setAcceptDrops(true);

    const auto opencc_config_path = applicationPath() + "/opencc"_str;
	convert_.load("s2tw.json", opencc_config_path.toStdString());
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

QString LyricsShowWidget::parsedSrtLyrics() const {
	if (!lyric_ || lyric_->size() <= 0) {
		return {};
	}

	QString result;
	auto subtitle_index = 1;
	for (auto itr = lyric_->cbegin(); itr != lyric_->cend(); ++itr) {
		const auto text = lyricEntryTextForSrt(*itr);
		if (text.isEmpty()) {
			continue;
		}

		auto start_time = lyricEntryStartTime(*itr);
		auto end_time = lyricEntryEndTime(*itr);
		if (end_time <= start_time) {
			auto next = itr;
			++next;
			for (; next != lyric_->cend(); ++next) {
				const auto next_start_time = lyricEntryStartTime(*next);
				if (next_start_time > start_time) {
					end_time = next_start_time;
					break;
				}
			}
		}
		if (end_time <= start_time) {
			end_time = start_time + std::chrono::seconds(3);
		}

		result += QString::number(subtitle_index++);
		result += "\r\n"_str;
		result += formatSrtTimestamp(start_time);
		result += " --> "_str;
		result += formatSrtTimestamp(end_time);
		result += "\r\n"_str;
		result += text;
		result += "\r\n\r\n"_str;
	}
	return result;
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
	QFont base_font = makeLyricsDisplayFont(lrc_font_);

	double base_font_size = base_font.pointSizeF();
	if (base_font_size <= 0.0) {
		base_font_size = 16.0;
		base_font.setPointSizeF(base_font_size);
	}
	painter->setFont(base_font);

	// 2) 決定預設筆色(若為當前行, 可換高亮)
	const auto is_current_line = index == item_ && item_offset_ == 0;
	QColor pen_color = lrc_color_;
	if (is_current_line) {
		pen_color = lrc_highlight_color_;
	}
	painter->setPen(pen_color);

	// 3) 取得該行資料與逐字資訊
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// 4) 準備 Furigana 與一般字體的 Metrics
	QFont furigana_font = base_font;
	QFont kanji_font = base_font;
	QFontMetrics furigana_metrics(furigana_font);
	QFontMetrics metrics(painter->font());
	const auto max_text_width = (std::max)(1, rect.width() - kLyricsHorizontalMargin * 2);

	qint64 global_time = pos_;
	qint64 line_start = entry.timestamp.count();
	const auto has_furigana =
		index >= 0
		&& index < static_cast<int32_t>(furiganas_.size())
		&& !furiganas_[index].empty();

	if ((is_fulled_ || is_lrc_valid_) && words.empty() && !has_furigana) {
		const QString text = QString::fromStdWString(entry.lrc);
		const auto main_lines = wrappedTextLineCount(text, base_font, max_text_width);
		const auto main_height = main_lines * metrics.lineSpacing();
		auto content_height = main_height;

		// 如果此行還有翻譯 (tlrc)，就再畫一行
		auto translation_height = 0;
		QFont translation_font = base_font;
		translation_font.setPointSizeF(base_font.pointSizeF() * kTranslationScale);
		QFontMetrics tm(translation_font);
		QString tr_text;
		if (!entry.tlrc.empty()) {
			tr_text = QString::fromStdWString(entry.tlrc);
			translation_height = wrappedTextLineCount(tr_text, translation_font, max_text_width) * tm.lineSpacing();
			content_height += kTranslationSpacing + translation_height;
		}

		auto top = rect.y() + (rect.height() - content_height) / 2.0;
		drawCenteredWrappedText(painter,
			QRectF(kLyricsHorizontalMargin, top, max_text_width, main_height),
			text,
			base_font,
			pen_color);

		if (!tr_text.isEmpty()) {
			top += main_height + kTranslationSpacing;
			drawCenteredWrappedText(painter,
				QRectF(kLyricsHorizontalMargin, top, max_text_width, translation_height),
				tr_text,
				translation_font,
				pen_color);
		}
		return;
	}

	// ------------------------------------------------------------------------
	// (A) 若沒有逐字資訊 (words.empty())，整行繪製
	// ------------------------------------------------------------------------
	if (words.empty()) {
		if (has_furigana) {
			// Furigana 字體縮小
			furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
			furigana_metrics = QFontMetrics(furigana_font);
			const auto& furigana_result = furiganas_[index];
			const auto ruby_layout = makeRubyLayout(furigana_result, metrics, furigana_metrics);
			double x = (rect.width() - rubyLayoutWidth(ruby_layout)) / 2.0;

			painter->setPen(is_current_line ? lrc_highlight_color_ : lrc_color_);

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

	painter->setFont(base_font);
	QFontMetrics fm(painter->font());
	const auto word_rows = makeWrappedWordRows(words, fm, max_text_width);
	if (word_rows.empty()) {
		return;
	}

	std::vector<std::vector<RubyLayoutSegment>> ruby_rows;
	if (!furiganas_.empty() && index < static_cast<int32_t>(furiganas_.size())) {
		furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		furigana_metrics = QFontMetrics(furigana_font);
		ruby_rows = wrapRubyLayout(
			makeRubyLayout(furiganas_[index], fm, furigana_metrics),
			max_text_width);
	}

	const auto row_count = (std::max)(word_rows.size(), ruby_rows.size());
	const auto ruby_height = ruby_rows.empty() ? 0 : furigana_metrics.height() + kRubySpacing;
	const auto row_height = ruby_height + fm.height();
	auto content_height = static_cast<int>(row_count) * row_height
		+ static_cast<int>((std::max)(size_t{ 1 }, row_count) - 1) * kWrappedRowSpacing;

	QColor translation_color = global_time >= line_start ? lrc_highlight_color_ : lrc_color_;
	QFont translation_font = base_font;
	translation_font.setPointSizeF(lrc_font_.pointSizeF() * kTranslationScale);
	QFontMetrics tm(translation_font);
	QString translation_text;
	auto translation_height = 0;
	if (!entry.tlrc.empty()) {
		translation_text = QString::fromStdWString(entry.tlrc);
		translation_height = wrappedTextLineCount(translation_text, translation_font, max_text_width) * tm.lineSpacing();
		content_height += kTranslationSpacing + translation_height;
	}

	auto row_top = rect.y() + (rect.height() - content_height) / 2.0;
	const auto delta = global_time - line_start;
	for (size_t row_index = 0; row_index < row_count; ++row_index) {
		if (row_index < ruby_rows.size()) {
			const auto& ruby_row = ruby_rows[row_index];
			auto x = (rect.width() - rubyLayoutWidth(ruby_row)) / 2.0;
			const auto furigana_baseline = row_top + furigana_metrics.ascent();
			painter->setFont(furigana_font);
			painter->setPen(global_time >= line_start ? lrc_highlight_color_ : lrc_color_);
			for (const auto& segment : ruby_row) {
				if (!segment.ruby.isEmpty()) {
					painter->drawText(
						x + (segment.column_width - segment.ruby_width) / 2.0,
						furigana_baseline,
						segment.ruby);
				}
				x += segment.column_width;
			}
		}

		if (row_index < word_rows.size()) {
			const auto& row = word_rows[row_index];
			auto x = (rect.width() - row.width) / 2.0;
			const auto baseline = static_cast<int>(row_top) + ruby_height + fm.ascent();

			painter->setFont(base_font);
			for (auto i = row.begin; i < row.end; ++i) {
				const auto& w = words[i];
				const auto word_text = QString::fromStdWString(w.content);
				const auto word_width = fm.horizontalAdvance(word_text);


				const auto w_start = w.offset.count();
				const auto w_end = w_start + w.length.count();
				auto fraction = 0.0;
				if (w.length.count() <= 0 && delta > w_start) {
					fraction = 1.0;
				}
				else if (delta > w_start) {
					fraction = delta >= w_end
						? 1.0
						: static_cast<double>(delta - w_start) / static_cast<double>(w_end - w_start);
				}
				fraction = std::clamp(fraction, 0.0, 1.0);

				const auto highlight_width = static_cast<int>(word_width * fraction);
				const QRect highlight_rect(x, row_top, highlight_width, row_height);
				// Do not draw the opaque base underneath the translucent sung portion.
				painter->save();
				painter->setClipRegion(QRegion(rect).subtracted(QRegion(highlight_rect)), Qt::IntersectClip);
				auto unsung_color = pen_color;
				if (is_current_line) {
					unsung_color.setAlpha(pen_color.alpha() * unsung_alpha_ / 255);
				}
				painter->setPen(unsung_color);
				painter->drawText(x, baseline, word_text);
				painter->restore();

				if (fraction > 0.0) {
					painter->save();
					painter->setPen(karaoke_highlight_color_);
					painter->setClipRect(highlight_rect, Qt::IntersectClip);
					painter->drawText(x, baseline, word_text);
					painter->restore();
				}
				x += word_width;
			}
		}

		row_top += row_height + kWrappedRowSpacing;
	}

	// 7) 繪製翻譯行 (如果有)
	if (!translation_text.isEmpty()) {
		drawCenteredWrappedText(painter,
			QRectF(kLyricsHorizontalMargin, row_top, max_text_width, translation_height),
			translation_text,
			translation_font,
			translation_color);
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
	const auto base_font = makeLyricsDisplayFont(lrc_font_);
	const QFontMetrics metrics(base_font);
	const auto max_text_width = (std::max)(1, width() - kLyricsHorizontalMargin * 2);
	auto max_row_count = size_t{ 1 };

	QFont furigana_font = base_font;
	QFontMetrics furigana_metrics(furigana_font);
	if (!furiganas_.empty()) {
		furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		furigana_metrics = QFontMetrics(furigana_font);
	}

	if (lyric_) {
		auto index = 0;
		for (const auto& entry : *lyric_) {
			auto row_count = size_t{ 1 };
			if (!entry.words.empty()) {
				row_count = makeWrappedWordRows(entry.words, metrics, max_text_width).size();
			}
			else {
				row_count = static_cast<size_t>(
					wrappedTextLineCount(QString::fromStdWString(entry.lrc), base_font, max_text_width));
			}

			if (index >= 0
				&& index < static_cast<int32_t>(furiganas_.size())
				&& !furiganas_[index].empty()) {
				const auto ruby_rows = wrapRubyLayout(
					makeRubyLayout(furiganas_[index], metrics, furigana_metrics),
					max_text_width);
				row_count = (std::max)(row_count, ruby_rows.size());
			}
			max_row_count = (std::max)(max_row_count, row_count);
			++index;
		}
	}

	const auto ruby_height = !furiganas_.empty()
		? furigana_metrics.height() + kRubySpacing
		: 0;
	auto height = static_cast<int32_t>(max_row_count) * (ruby_height + metrics.height())
		+ static_cast<int32_t>(max_row_count - 1) * kWrappedRowSpacing;

	if (lyric_->hasTranslation()) {
		QFont translation_font = base_font;
		translation_font.setPointSizeF(lrc_font_.pointSizeF() * kTranslationScale);
		const QFontMetrics tm(translation_font);
		height += tm.height() + kTranslationSpacing;
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
		decodeLyricsEntry(lrc);
		if (language_detector_.isJapanese(lrc.lrc)) {
			furiganas_.push_back(furigana_.convert(lrc.lrc));
		} else {
			// 這裡要補空的，否則會造成 index 不一致.
			furiganas_.emplace_back();
		}
		if (language_detector_.isChinese(lrc.lrc)) {
			lrc.lrc = convert_.convert(lrc.lrc);
			for (auto& word : lrc.words) {
				word.content = convert_.convert(word.content);
			}
			lrc.lrc = convert_.convert(lrc.lrc);
		}
		lrc.tlrc = convert_.convert(lrc.tlrc);
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
	const auto lines = normalizeLyricsText(lrc).split(QChar(u'\n'), Qt::KeepEmptyParts);

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
	std::wistringstream stream{ collapseConsecutiveLineBreaks(lrc).toStdWString() };
	if (!lyric_->parse(stream)) {
		setDefaultLrc();
	}
	else {
		stop_scroll_time_ = false;
		is_lrc_valid_ = true;
		is_fulled_ = false;
		LanguageDetector detector;
		for (auto& lrc : *lyric_) {
			decodeLyricsEntry(lrc);
			if (detector.isJapanese(lrc.lrc)) {
				furiganas_.push_back(furigana_.convert(lrc.lrc));
			} else {
				// 這裡要補空的，否則會造成 index 不一致.
				furiganas_.emplace_back();
				lrc.tlrc = convert_.convert(lrc.tlrc);
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

	const QFontMetrics metrics(makeLyricsDisplayFont(lrc_font_));
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
	lrc_font_ = makeLyricsDisplayFont(font);
	current_mask_font_ = lrc_font_;
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

void LyricsShowWidget::setUnsungAlpha(int alpha) {
	unsung_alpha_ = std::clamp(alpha, 0, 255);
	update();
}
