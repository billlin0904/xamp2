#include <QMouseEvent>
#include <widget/playlisttabbar.h>

PlaylistTabBar::PlaylistTabBar(QWidget* parent)
	: QTabBar(parent) {
	line_edit_ = new QLineEdit(this);
	//line_edit_->setWindowFlags(Qt::Popup);
	//line_edit_->setFocusProxy(this);
	//setStyleSheet("color: black");
	line_edit_->hide();
	(void)QObject::connect(line_edit_, &QLineEdit::editingFinished,
		this, &PlaylistTabBar::OnRename);
	//installEventFilter(this);
}

void PlaylistTabBar::OnRename() {
	setTabText(edited_index_, line_edit_->text());
	line_edit_->hide();
	emit TextChanged(edited_index_, line_edit_->text());
}

bool PlaylistTabBar::eventFilter(QObject* watched, QEvent* event) {
	/*auto mouse_press = event->type() == QEvent::MouseButtonPress
	&& !line_edit_->geometry().contains(dynamic_cast<QMouseEvent*>(event)->globalPos());

	if (mouse_press
		|| (event->type() == QEvent::KeyPress && dynamic_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)) {
		line_edit_->hide();
		return true;
	}*/
	return QTabBar::eventFilter(watched, event);
}

void PlaylistTabBar::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton) {
		QTabBar::mouseDoubleClickEvent(event);
		return;
	}
	edited_index_ = currentIndex();
	line_edit_->setText(tabText(edited_index_));
	line_edit_->setGeometry(tabRect(edited_index_));
	if (!line_edit_->isVisible()) {
		line_edit_->show();
	}	
}
