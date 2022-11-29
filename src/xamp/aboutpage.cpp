#include <QFile>
#include <widget/str_utilts.h>
#include "thememanager.h"
#include "aboutpage.h"

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    ui.lblLogo->setPixmap(qTheme.appIcon().pixmap(128, 128));

    QFont dp_font(qTEXT("DisplayFont"));
    dp_font.setBold(true);
    dp_font.setPointSizeF(26);

    ui.lblProjectTitle->setFont(dp_font);
    ui.lblProjectTitle->setText(qTEXT("XAMP2"));
    ui.lblProjectTitle->setStyleSheet(qTEXT("QLabel#lblProjectTitle { border: none; background: transparent; }"));

    ui.lblDescription->setText(qTEXT("Cross-platform native DSD and low latency playback music player."));

    QString domain_txt(qTEXT("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui.lblDomain->setText(domain_txt.arg(qTEXT("github.com/billlin0904/xamp2")));
    ui.lbIGithubIcon->setPixmap(qTheme.githubIcon());
    ui.lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2022 XAMP2 Project."));

    QFont font(qTEXT("UIFont"));
    font.setBold(false);
    font.setPointSize(qTheme.fontSize());
    ui.txtBws->setFont(font);
    ui.txtBws->setVisible(false);

    QFile lincense_file(qTEXT("lincense.txt"));
    if (lincense_file.open(QIODevice::ReadOnly)) {
        lincense_ = QLatin1String(lincense_file.readAll());
    }

    QFile credits_file(qTEXT("credits.txt"));
    if (credits_file.open(QIODevice::ReadOnly)) {
        credits_ = QLatin1String(credits_file.readAll());
    }

    setStyleSheet(qTEXT("QFrame#AboutDialog { background-color: transparent }"));
    ui.lblLogo->setStyleSheet(qTEXT("background-color: transparent"));
    ui.lblProjectTitle->setStyleSheet(qTEXT("background-color: transparent"));
    ui.lblCopying->setStyleSheet(qTEXT("background-color: transparent"));
    ui.lblDescription->setStyleSheet(qTEXT("background-color: transparent"));
    ui.lblDomain->setStyleSheet(qTEXT("background-color: transparent"));
    ui.wdtContent->setStyleSheet(qTEXT("background-color: transparent"));
}

void AboutPage::onCreditsOrLicenceChecked(bool checked) {
    if (sender()->objectName() == qTEXT("btnLicense")) {
        ui.txtBws->setText(lincense_);
        ui.btnCredits->setChecked(false);
    }
    else if (sender()->objectName() == qTEXT("btnCredits")) {
        ui.txtBws->setText(credits_);
        ui.btnLicense->setChecked(false);
    }
    ui.txtBws->setVisible(checked);
    ui.wdtContent->setVisible(!checked);
}
