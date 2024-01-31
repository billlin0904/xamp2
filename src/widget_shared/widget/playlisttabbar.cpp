#include <widget/playlisttabbar.h>
#include <QMouseEvent>
#include <QPushButton>

#include <thememanager.h>
#include <widget/util/str_utilts.h>

PlaylistTabBar::PlaylistTabBar(QWidget* parent)
	: QTabBar(parent) {	
	setExpanding(true);
	setTabsClosable(true);
	setUsesScrollButtons(true);
	setElideMode(Qt::TextElideMode::ElideRight);
	setMovable(true);
	auto f = font();
	f.setPointSize(qTheme.fontSize(10));
	setFont(f);
	setFocusPolicy(Qt::StrongFocus);
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
	constexpr auto left_margin = 6;

	const auto rect = tabRect(edited_index_);

	line_edit_ = new QLineEdit(this);

	switch (qTheme.themeColor()) {
	case ThemeColor::LIGHT_THEME:
		line_edit_->setStyleSheet(qSTR(R"(
	QLineEdit {
		background-color: #C9CDD0;
	}	
    )"));
		break;
	case ThemeColor::DARK_THEME:
		line_edit_->setStyleSheet(qSTR(R"(
	QLineEdit {
		background-color: #455364;
	}	
    )"));
		break;
	}

	line_edit_->setText(tabText(edited_index_));
	line_edit_->move(rect.left() + left_margin, rect.top() + top_margin);
	line_edit_->resize(rect.width() - 2 * left_margin, rect.height() - 2 * top_margin);
	line_edit_->selectAll();
	line_edit_->setFocus();
	(void)QObject::connect(line_edit_, &QLineEdit::editingFinished,
		this, &PlaylistTabBar::onFinishRename);
	line_edit_->show();
}

void PlaylistTabBar::focusOutEvent(QFocusEvent* event) {
	QTabBar::focusOutEvent(event);

	if (line_edit_ && !line_edit_->hasFocus()) {
		onFinishRename();
	}
}
