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
	// �]1�^���ˬd
	if (!lyric_) {
		return;
	}
	// index �W�X�d��N���e
	const int32_t total_count = lyric_->getSize();
	if (index < 0 || index >= total_count) {
		return;
	}

	// �]2�^�r���L���޿� (�����A���쥻�޿�A�]�i�ίB�I�ӭp��)
	// ----------------------------------------------------------------------
	// ���q�򩳦r���X�o
	QFont baseFont = lrc_font_;
	double baseFontSize = baseFont.pointSizeF();
	if (baseFontSize <= 0.0) {
		baseFontSize = 16.0;
		baseFont.setPointSizeF(baseFontSize);
	}

	// �ھ� item_ / item_offset_ �Ӱ��j�p�Υ[��
	// �o�̥ܽd�@��²��g�k�A�i�̧A���ݭn�A�u��
	double scaleFactor = 0.12; // �۷���µ{������ 1.2 / 10
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
		// �����p�T�׼W�j
		newFontSize = baseFontSize + ch;
	}
	// ����r���Y�o�Ӥp�ιL�j
	const double minFontSize = 8.0;
	const double maxFontSize = 72.0;
	if (newFontSize < minFontSize) {
		newFontSize = minFontSize;
	}
	else if (newFontSize > maxFontSize) {
		newFontSize = maxFontSize;
	}

	// �M�w�n���n bold�B�n���n���G�C��
	QFont newFont = baseFont;
	newFont.setPointSizeF(newFontSize);
	painter->setFont(newFont);

	QColor penColor = lrc_color_;
	if (isCurrentLine && item_offset_ == 0) {
		// ���n�b������B�������ɡA���i�������G
		penColor = lrc_highlight_color_;
	}
	painter->setPen(penColor);

	// �]3�^���o�o������� LyricEntry �P�v�r��T
	const LyricEntry& entry = lyric_->lineAt(index);
	const auto& words = entry.words;

	// �]4�^�Y�S���v�r��T�A�����쥻�@���e�����
	if (words.empty()) {
		// �p�G�A�쥻�� Furigana �޿�A�i�b�o�̪����B�z
		// --- �L Furigana �� furiganas_.empty() �� ---
		const QFontMetrics metrics(painter->font());
		const auto text = QString::fromStdWString(entry.lrc);

		if (furiganas_.empty() || furiganas_[index].empty()) {
			// �����e�@��q
			painter->drawText(
				(rect.width() - metrics.horizontalAdvance(text)) / 2,
				rect.y() + (rect.height() - metrics.height()) / 2,
				text
			);
			return;
		}
		else {
			// �P�A���e�ۦP�� Furigana ø�s
			// (���ɤ����u�v�r partial highlight�v�A�]�� words �� empty)
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

				// ���e Furigana
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
				// �A�e�~�r
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

	// �]5�^�Y�Ӧ榳�v�r��T�A�i��u�d�� OK ���v�rø�s�v
	// -----------------------------------------------------------
	// �p��Ӧ�w�g�ۤF�h�ֲ@��
	// ���] pos_ ���u��������ɶ�(�@��)�v
	qint64 globalTime = pos_; // �A���{���i��O int32_t�A�]�i��O long long
	qint64 lineStart = entry.timestamp.count();
	qint64 delta = globalTime - lineStart;
	// delta < 0 => �|���ۨ�o��
	// delta > (end_time - start_time) => �w�g�ۧ�

	// ��X�Ӧ�Ҧ� word �`�e�סA��K�m��
	QFontMetrics fm(painter->font());
	int totalWidth = 0;
	for (auto& w : words) {
		auto wordText = QString::fromStdWString(w.content);
		totalWidth += fm.horizontalAdvance(wordText);
	}

	int x = (rect.width() - totalWidth) / 2;
	// ��ĳ�ΰ�uø�s
	int baseline = rect.y() + (rect.height() + fm.ascent()) / 2;

	// �p�G�ݭn�A�� Furigana ��X�A�i��o�̩�o������A�ܽd�H�u�v�r��r content�v���D
	// --------------------------------------------------------------------------
	for (auto& w : words) {
		QString wordText = QString::fromStdWString(w.content);
		int wordWidth = fm.horizontalAdvance(wordText);

		// ���r�b�椺���}�l/�����ɶ�
		qint64 wStart = w.offset.count();         // �۹�Ӧ�}�Y
		qint64 wEnd = wStart + w.length.count();

		// ��X�ثe�����o�Ӧr���u�����סv0~1
		double fraction = 0.0;
		if (delta <= wStart) {
			fraction = 0.0;    // �|���ۨ�
		}
		else if (delta >= wEnd) {
			fraction = 1.0;    // �w�ۧ�
		}
		else {
			fraction = double(delta - wStart) / double(wEnd - wStart);
		}
		if (fraction < 0.0) fraction = 0.0;
		if (fraction > 1.0) fraction = 1.0;

		// ���e�u���ۧ��v�C�� (penColor)
		painter->setPen(penColor);
		painter->drawText(x, baseline, wordText);

		// �Y fraction > 0�A�N���w�۳����A�� highLight color �|�W�h
		if (fraction > 0.0) {
			painter->save();
			//painter->setPen(lrc_highlight_color_);
			painter->setPen(QColor(77, 208, 225, 200));

			// clipRect: �u��ܦr�饪�� fraction �ϰ�
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
