#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget* parent) 
    : QDialog(parent) {
    ui.setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
}
