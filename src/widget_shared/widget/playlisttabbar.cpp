#include <widget/playlisttabbar.h>
#include <QMouseEvent>
#include <QPushButton>
#include <QProxyStyle>
#include <QPainter>

#include <thememanager.h>
#include <widget/xtooltip.h>
#include <widget/util/str_util.h>

// TextAlignedStyle
// 
// QTabWidget not support set text aligned flag.
// So, I use QProxyStyle to draw text aligned.
//
class TextAlignedStyle : public QProxyStyle {
public:
	explicit TextAlignedStyle(QStyle* style = nullptr)
		: QProxyStyle(style) {
	}

    void drawItemText(QPainter* painter, const QRect& rect, int /*flags*/, const QPalette& pal, bool enabled,
		const QString& text, QPalette::ColorRole textRole) const override {
		QCommonStyle::drawItemText(painter, rect, Qt::AlignVCenter | Qt::AlignLeft, pal, enabled, text, textRole);
	}
};

PlaylistTabBar::PlaylistTabBar(QWidget* parent)
	: QTabBar(parent) {	
	setExpanding(true);
	setTabsClosable(true);
	setUsesScrollButtons(false);
    setElideMode(Qt::TextElideMode::ElideRight);
	setMovable(true);
	auto f = font();
	f.setPointSize(qTheme.fontSize(10));
	setFont(f);
	setFocusPolicy(Qt::StrongFocus);
    setStyle(new TextAlignedStyle());
}

void PlaylistTabBar::setTabCount(int32_t count) {
	tab_count_ = count;
}

void PlaylistTabBar::onFinishRename() {
	if (!line_edit_) {
		return;
	}
	setTabText(edited_index_, line_edit_->text());
	emit textChanged(edited_index_, line_edit_->text());
	line_edit_->deleteLater();
	line_edit_ = nullptr;
}

void PlaylistTabBar::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton) {
		QTabBar::mouseDoubleClickEvent(event);
		return;
	}

	edited_index_ = currentIndex();
	if (edited_index_ == -1) {
		return;
	}

	constexpr auto top_margin = 3;
	constexpr auto left_margin = 24;

	const auto rect = tabRect(edited_index_);

	line_edit_ = new QLineEdit(this);
	line_edit_->installEventFilter(this);

	switch (qTheme.themeColor()) {
	case ThemeColor::LIGHT_THEME:
		line_edit_->setStyleSheet(qFormat(R"(
	QLineEdit {
		background-color: #C9CDD0;
	}	
    )"));
		break;
	case ThemeColor::DARK_THEME:
		line_edit_->setStyleSheet(qFormat(R"(
	QLineEdit {
		background-color: #455364;
	}	
    )"));
		break;
	}

	auto f = font();
	f.setBold(true);
	line_edit_->setFont(f);
	line_edit_->setText(tabText(edited_index_));
	line_edit_->move(rect.left() + left_margin, rect.top() + top_margin);
	line_edit_->resize(rect.width() - 2 * left_margin, rect.height() - 2 * top_margin);
	line_edit_->selectAll();
	line_edit_->setFocus();
	(void)QObject::connect(line_edit_, &QLineEdit::editingFinished,
		this, &PlaylistTabBar::onFinishRename);
	line_edit_->show();
}

void PlaylistTabBar::finishRename() {
	if (line_edit_ && !line_edit_->hasFocus()) {
		onFinishRename();
	}
}

void PlaylistTabBar::focusOutEvent(QFocusEvent* event) {
	QTabBar::focusOutEvent(event);
	finishRename();
}

bool PlaylistTabBar::eventFilter(QObject* object, QEvent* event) {
	if (object == line_edit_ && event->type() == QEvent::KeyPress) {
		auto* ke = static_cast<QKeyEvent*>(event);
		if (ke->key() == Qt::Key_Escape) {
			onFinishRename();
			return true;
		}
	}
	return false;
}

QSize PlaylistTabBar::tabSizeHint(int index) const {
	QSize size(QTabBar::tabSizeHint(index));
	auto width = dynamic_cast<QWidget*>(parent())->width() - kMaxButtonWidth;
	if (tab_count_ <= 4) {
		return QSize(width / 4, size.height());
	}
	size.setWidth(width / tab_count_);
	return size;
}

void PlaylistTabBar::resizeEvent(QResizeEvent* event) {
	QTabBar::resizeEvent(event);
}
