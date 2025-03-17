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

namespace {
	QSharedPointer<ILrcParser> makeLrcParser(const QString& file_path, QString& lrc_path, bool& use_default) {
		const QFileInfo file_info(file_path);
		const QString file_dir = file_info.path();
		const QString base_name = file_info.completeBaseName();
		const QString suffix = file_info.completeSuffix();

		lrc_path = file_dir + QDir::separator() + base_name;
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
			if (QFileInfo::exists(lrc_path + parser_pair.first)) {
				lrc_path = lrc_path + parser_pair.first;
				make_parser_func = parser_pair.second;
				break;
				// Path like "C:/filename.mp3.lrc"
			}
			else if (QFileInfo::exists(lrc_path + "."_str + suffix + parser_pair.first)) {
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
}

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
	is_lrc_valid_ = true;
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

void LyricsShowWidget::paintItem(QPainter* painter, int32_t index, QRect& rect) {
	if (!lyric_) {
		return;
	}
	int32_t total_count = lyric_->size();
	if (index < 0 || index >= total_count) {
		return;
	}

	// 1) ���ǳƦr��
	QFont base_font = lrc_font_;
	double base_font_size = base_font.pointSizeF();
	if (base_font_size <= 0.0) {
		base_font_size = 16.0;
		base_font.setPointSizeF(base_font_size);
	}
	painter->setFont(base_font);

	// 2) �M�w�w�]����(�Y����e��, �i�����G)
	QColor pen_color = lrc_color_;
	if ((index == item_) && (item_offset_ == 0)) {
		pen_color = lrc_highlight_color_;
	}
	painter->setPen(pen_color);

	// 3) ���o�Ӧ��ƻP�v�r��T
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// 4) �ǳ� Furigana �P�@��r�骺 Metrics
	QFont furigana_font = lrc_font_;
	QFont kanji_font = lrc_font_;
	QFontMetrics furigana_metrics(furigana_font);
	QFontMetrics metrics(painter->font());

	qint64 global_time = pos_;
	qint64 line_start = entry.timestamp.count();

	// ------------------------------------------------------------------------
	// (A) �Y�S���v�r��T (words.empty())�A���ø�s
	// ------------------------------------------------------------------------
	if (words.empty()) {
		if (!furiganas_.empty()) {
			double x = (rect.width() - metrics.horizontalAdvance(
				QString::fromStdWString(entry.lrc))) / 2.0;

			// Furigana �r���Y�p
			furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
			auto furigana_result = furiganas_[index];

			if (global_time >= line_start) {
				// ��e�� => �ΦP�@�Ӱ��G��
				painter->setPen(lrc_highlight_color_);
			}
			else {
				// ��L�� => �Υ��`�C��
				painter->setPen(lrc_color_);
			}

			for (const auto& entity : furigana_result) {
				auto kanji_text = QString::fromStdWString(entity.text);
				int kanji_width = metrics.horizontalAdvance(kanji_text);
				auto furigana_length = entity.furigana.size();

				// ��ø�s Furigana (�Y��)
				if (furigana_length > 0) {
					painter->setFont(furigana_font);
					double furigana_char_width = static_cast<double>(kanji_width) / furigana_length;
					int furigana_y = rect.y()
						+ (rect.height() - metrics.height()) / 2
						- furigana_metrics.height();
					for (int i = 0; i < static_cast<int>(furigana_length); ++i) {
						auto furigana_char = QString::fromStdWString(entity.furigana).mid(i, 1);
						painter->drawText(x + i * furigana_char_width, furigana_y, furigana_char);
					}
				}
				// �Aø�s�D�r (Kanji)
				painter->setFont(kanji_font);
				painter->drawText(
					x,
					rect.y() + (rect.height() - metrics.height()) / 2,
					kanji_text
				);
				x += kanji_width;
			}
		}
		return;
	}

	if (!furiganas_.empty() && index < furiganas_.size()) {
		// ------------------------------------------------------------------------
		// (B) ���v�r��T�A�h���d��OK�v�r���G + Furigana
		// ------------------------------------------------------------------------
		// 5) ��ø�s��� Furigana (���ݭn�� words �� 1:1 �j�p)
		furigana_font.setPointSizeF(lrc_font_.pointSizeF() * 0.5);
		double x_f = (rect.width() - metrics.horizontalAdvance(
			QString::fromStdWString(entry.lrc))) / 2.0;
		auto furigana_result = furiganas_[index];
		int base_line = rect.y() + (rect.height() + metrics.ascent()) / 2;

		if (global_time >= line_start) {
			// ��e�� => �ΦP�@�Ӱ��G��
			painter->setPen(lrc_highlight_color_);
		}
		else {
			// ��L�� => �Υ��`�C��
			painter->setPen(lrc_color_);
		}

		// ���θ��p�r��ø Furigana
		painter->setFont(furigana_font);
		for (const auto& entity : furigana_result) {
			auto kanji_text = QString::fromStdWString(entity.text);
			int kanji_width = metrics.horizontalAdvance(kanji_text);
			int furigana_length = static_cast<int>(entity.furigana.size());

			int furigana_y = base_line - furigana_metrics.height();

			// �v�r�e�X Furigana
			if (furigana_length > 0) {
				double furigana_char_width = static_cast<double>(kanji_width) / furigana_length;
				for (int i = 0; i < furigana_length; ++i) {
					QString fchar = QString::fromStdWString(entity.furigana).mid(i, 1);
					painter->drawText(x_f + i * furigana_char_width, furigana_y, fchar);
				}
			}
			x_f += kanji_width;
		}
	}

	// 6) �A�Υd��OK�覡�e�D��r
	qint64 delta = global_time - line_start;

	painter->setFont(lrc_font_);
	QFontMetrics fm(painter->font());

	// �p��Ӧ�Ҧ� words ���`�e�� (�m��)
	int total_width = 0;
	for (auto& w : words) {
		QString w_text = QString::fromStdWString(w.content);
		total_width += fm.horizontalAdvance(w_text);
	}
	double x = (rect.width() - total_width) / 2.0;
	int baseline = rect.y() + (rect.height() + fm.ascent()) / 2;

	// �v�rø�s + ���G
	for (int i = 0; i < static_cast<int>(words.size()); ++i) {
		const auto& w = words[i];
		QString word_text = QString::fromStdWString(w.content);
		int word_width = fm.horizontalAdvance(word_text);

		// ���e�u���G�v�C��
		painter->setPen(pen_color);
		painter->drawText(x, baseline, word_text);

		// �p�⦹�r�����G fraction
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

		// �p�G fraction>0 �N�� clipRect �|�[���G
		if (fraction > 0.0) {
			painter->save();
			painter->setPen(kKaraokeHighLightColor); // �A�۩w�q�����G��
			int highlight_width = static_cast<int>(word_width * fraction);
			painter->setClipRect(x, rect.y(), highlight_width, rect.height());
			painter->drawText(x, baseline, word_text);
			painter->restore();
		}
		x += word_width;
	}

	// 7) ø�s½Ķ�� (�p�G��)
	if (!entry.tlrc.empty()) {
		QColor translation_color;
		if (global_time >= line_start) {
			// ��e�� => �ΦP�@�Ӱ��G��
			translation_color = lrc_highlight_color_;
		}
		else {
			// ��L�� => �Υ��`�C��
			translation_color = lrc_color_;
		}

		float  translation_scale_ = 0.8f;                 // ½Ķ��r��۹�D��j�p
		int    translation_line_spacing_ = 5;             // �D��P½Ķ�椧�����������Z(����)

		// ���o½Ķ��r
		const QString translation_text = QString::fromStdWString(entry.tlrc);

		// �]�w½Ķ��r�� (�i�P�D�椣�P�j�p)
		QFont translation_font = lrc_font_;
		translation_font.setPointSizeF(lrc_font_.pointSizeF()* translation_scale_);
		painter->setFont(translation_font);

		// ���s�p��r�鰪��
		QFontMetrics tm(translation_font);

		// �M�w½Ķ�檺��ǽu(�b�D�� baseline �A���U���@��)
		int translation_baseline = baseline
			+ tm.height()
			+ translation_line_spacing_; // �[�I���Z

		// �m���p�� (��½Ķ��D�泣�m��)
		int translation_width = tm.horizontalAdvance(translation_text);
		double x_trans = (rect.width() - translation_width) / 2.0;

		// �]�w�C��(�P�D���C�⤣�P�]�i)
		painter->setPen(translation_color);

		// ø�s½Ķ��r
		painter->drawText(x_trans, translation_baseline, translation_text);
	}
}

void LyricsShowWidget::paintBackground(QPainter* painter) {
	painter->fillRect(rect(), background_color_);
}

void LyricsShowWidget::paintItemMask(QPainter* painter) {
}

int32_t LyricsShowWidget::itemHeight() const {
	const QFontMetrics metrics(lrc_font_);
	auto ratio = 1.5;
	if (lyric_->hasTranslation()) {
		ratio = 2.5;
	}
    return static_cast<int32_t>(metrics.height() * ratio);
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
			loadFile(url.toLocalFile());
			break;
		}
		event->acceptProposedAction();
	}
}

void LyricsShowWidget::loadFromParser(const QSharedPointer<ILrcParser>& parser) {
	lyric_ = parser;
	furiganas_.clear();

	CharsetDetector detector;
	for (auto& lrc : *lyric_) {
		if (detector.IsJapanese(lrc.lrc)) {
			furiganas_.push_back(furigana_.Convert(lrc.lrc));
		} else {
			// �o�̭n�ɪŪ��A�_�h�|�y�� index ���@�P.
			furiganas_.push_back({});
		}
		lrc.tlrc = convert_.Convert(lrc.tlrc);
	}
	is_lrc_valid_ = true;
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
		CharsetDetector detector;
		for (auto& lrc : *lyric_) {
			if (detector.IsJapanese(lrc.lrc)) {
				furiganas_.push_back(furigana_.Convert(lrc.lrc));
			} else {
				// �o�̭n�ɪŪ��A�_�h�|�y�� index ���@�P.
				furiganas_.push_back({});
				lrc.tlrc = convert_.Convert(lrc.tlrc);
			}
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

	if (is_fulled_ || is_scrolled_ || !lyric_->size()) {
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
	const auto precent = static_cast<float>(post_ly.index) / static_cast<float>(lyric_->size());

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
