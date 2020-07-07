#include <widget/str_utilts.h>
#include "aboutdialog.h"

inline constexpr ConstLatin1String kCredits(
R"(
)"
);

inline constexpr ConstLatin1String kLincense(
R"(
    The MIT License (MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
)");


AboutDialog::AboutDialog(QWidget* parent) 
    : XampDialog(parent) {
    ui.setupUi(this);
    ui.lblVersion->setText(Q_UTF8("0.0.0"));

    QIcon icon(Q_UTF8(":/xamp/xamp.ico"));
    ui.lblLogo->setPixmap(icon.pixmap(128, 128));

    QFont font = ui.lblProjectTitle->font();
    font.setBold(true);
    ui.lblProjectTitle->setFont(font);
    ui.lblProjectTitle->setText(tr("XAMP2"));

    ui.lblDescription->setText(Q_UTF8("Cross-platform native DSD and low latency playback music player."));

    QString domainTxt(Q_UTF8("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui.lblDomain->setText(domainTxt.arg(Q_UTF8("github.com/billlin0904/xamp2")));
    ui.lblCopying->setText(Q_STR("Copyright (c) 2018-2020 %1").arg(Q_UTF8("XAMP2 Project")));

    font.setBold(false);
    font.setPointSize(9);
    ui.txtBws->setFont(font);
    ui.txtBws->setVisible(false);
}

void AboutDialog::onCreditsOrLicenceChecked(bool checked) {
    if (sender()->objectName() == Q_UTF8("btnLicense")) {
        ui.txtBws->setText(kLincense);
        ui.btnCredits->setChecked(false);
    }
    else if (sender()->objectName() == Q_UTF8("btnCredits")) {
        ui.txtBws->setText(kCredits);
        ui.btnLicense->setChecked(false);
    }
    ui.txtBws->setVisible(checked);
    ui.wdtContent->setVisible(!checked);
}