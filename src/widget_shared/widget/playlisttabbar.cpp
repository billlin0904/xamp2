#include <QMouseEvent>

#include <thememanager.h>
#include <widget/str_utilts.h>
#include <widget/playlisttabbar.h>

PlaylistTabBar::PlaylistTabBar(QWidget* parent)
	: QTabBar(parent) {	
	setExpanding(true);
	setTabsClosable(true);
	setUsesScrollButtons(true);
	setMovable(true);
	installEventFilter(this);
	auto f = font();
	f.setPointSize(qTheme.GetFontSize(8));
	setFont(f);
}

void PlaylistTabBar::OnRename() {
	if (!line_edit_) {
		return;
	}
	setTabText(edited_index_, line_edit_->text());
	emit TextChanged(edited_index_, line_edit_->text());
	line_edit_->deleteLater();
	line_edit_ = nullptr;
}

void PlaylistTabBar::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton) {
		QTabBar::mouseDoubleClickEvent(event);
		return;
	}

	edited_index_ = currentIndex();

	auto top_margin = 3;
	auto left_margin = 6;

	const auto rect = tabRect(edited_index_);

	line_edit_ = new QLineEdit(this);

	switch (qTheme.GetThemeColor()) {
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
		this, &PlaylistTabBar::OnRename);
	line_edit_->show();
}
