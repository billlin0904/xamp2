#include <QFile>

#include <widget/str_utilts.h>
#include "thememanager.h"
#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget* parent) 
    : XampDialog(parent) {
    ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui.lblVersion->setText(Q_UTF8("0.0.0"));

    ui.lblLogo->setPixmap(ThemeManager::instance().appIcon().pixmap(128, 128));

    auto font = ui.lblProjectTitle->font();
    font.setBold(true);
    font.setPointSizeF(18);
    ui.lblProjectTitle->setFont(font);
    ui.lblProjectTitle->setText(tr("XAMP2"));
    ui.lblProjectTitle->setStyleSheet(Q_UTF8("QLabel#lblProjectTitle { border: none; background: transparent; }"));

    ui.lblDescription->setText(Q_UTF8("Cross-platform native DSD and low latency playback music player."));

    QString domainTxt(Q_UTF8("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui.lblDomain->setText(domainTxt.arg(Q_UTF8("github.com/billlin0904/xamp2")));
    ui.lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2021 XAMP2 Project."));

    font.setBold(false);
    font.setPointSize(9);
    ui.txtBws->setFont(font);
    ui.txtBws->setVisible(false);

    QFile lincense_file(Q_UTF8("lincense.txt"));
    lincense_file.open(QIODevice::ReadOnly);
    lincense_ = QLatin1String(lincense_file.readAll());

    QFile credits_file(Q_UTF8("credits.txt"));
    credits_file.open(QIODevice::ReadOnly);
    credits_ = QLatin1String(credits_file.readAll());

    ThemeManager::instance().setBackgroundColor(this);
}

void AboutDialog::onCreditsOrLicenceChecked(bool checked) {
    if (sender()->objectName() == Q_UTF8("btnLicense")) {
        ui.txtBws->setText(lincense_);
        ui.btnCredits->setChecked(false);
    }
    else if (sender()->objectName() == Q_UTF8("btnCredits")) {
        ui.txtBws->setText(credits_);
        ui.btnLicense->setChecked(false);
    }
    ui.txtBws->setVisible(checked);
    ui.wdtContent->setVisible(!checked);
}
