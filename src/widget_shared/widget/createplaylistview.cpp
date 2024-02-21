#include <QDialog>
#include <widget/createplaylistview.h>
#include <ui_createplaylistdialog.h>

CreatePlaylistView::CreatePlaylistView(QWidget* parent)
	: QFrame(parent) {
	ui_ = new Ui::CreatePlaylistView();
	ui_->setupUi(this);
	QObject::connect(ui_->buttonBox, &QDialogButtonBox::accepted, dynamic_cast<QDialog*>(parent), &QDialog::accept);
	QObject::connect(ui_->buttonBox, &QDialogButtonBox::rejected, dynamic_cast<QDialog*>(parent), &QDialog::reject);
}

CreatePlaylistView::~CreatePlaylistView() {
	delete ui_;
}


QString CreatePlaylistView::title() const {
	return ui_->titleLineEdit->text();
}

QString CreatePlaylistView::desc() const {
	return ui_->descLineEdit->text();
}

int32_t CreatePlaylistView::privateStatus() const {
	return ui_->privateStatusComboBox->currentIndex();
}
