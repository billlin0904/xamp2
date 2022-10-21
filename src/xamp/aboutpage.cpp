#include <QFile>

#include <base/simd.h>
#include <widget/str_utilts.h>
#include "version.h"
#include "thememanager.h"
#include "aboutpage.h"

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    ui.lblLogo->setPixmap(qTheme.appIcon().pixmap(128, 128));

    QFont font(Q_TEXT("UI"));
    font.setBold(true);
    font.setPointSizeF(26);

    ui.lblProjectTitle->setFont(font);
    ui.lblProjectTitle->setText(tr("XAMP2"));
    ui.lblProjectTitle->setStyleSheet(Q_TEXT("QLabel#lblProjectTitle { border: none; background: transparent; }"));

    ui.lblDescription->setText(Q_TEXT("Cross-platform native DSD and low latency playback music player."));

    QString domain_txt(Q_TEXT("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui.lblDomain->setText(domain_txt.arg(Q_TEXT("github.com/billlin0904/xamp2")));
    ui.lbIGithubIcon->setPixmap(qTheme.githubIcon());
    ui.lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2022 XAMP2 Project."));

    font.setBold(false);
    font.setPointSize(10);
    ui.txtBws->setFont(font);
    ui.txtBws->setVisible(false);

    QFile lincense_file(Q_TEXT("lincense.txt"));
    if (lincense_file.open(QIODevice::ReadOnly)) {
        lincense_ = QLatin1String(lincense_file.readAll());
    }

    QFile credits_file(Q_TEXT("credits.txt"));
    if (credits_file.open(QIODevice::ReadOnly)) {
        credits_ = QLatin1String(credits_file.readAll());
    }

    setStyleSheet(Q_TEXT("QFrame#AboutDialog { background-color: transparent }"));
    ui.lblLogo->setStyleSheet(Q_TEXT("background-color: transparent"));
    ui.lblProjectTitle->setStyleSheet(Q_TEXT("background-color: transparent"));
    ui.lblCopying->setStyleSheet(Q_TEXT("background-color: transparent"));
    ui.lblDescription->setStyleSheet(Q_TEXT("background-color: transparent"));
    ui.lblDomain->setStyleSheet(Q_TEXT("background-color: transparent"));
}

void AboutPage::onCreditsOrLicenceChecked(bool checked) {
    if (sender()->objectName() == Q_TEXT("btnLicense")) {
        ui.txtBws->setText(lincense_);
        ui.btnCredits->setChecked(false);
    }
    else if (sender()->objectName() == Q_TEXT("btnCredits")) {
        ui.txtBws->setText(credits_);
        ui.btnLicense->setChecked(false);
    }
    ui.txtBws->setVisible(checked);
    ui.wdtContent->setVisible(!checked);
}
