#include <QFile>

#include <widget/str_utilts.h>
#include "thememanager.h"
#include "aboutpage.h"

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);
    setStyleSheet(Q_UTF8("background-color: transparent"));

#ifdef Q_OS_WIN32
    ui.lblVersion->setText(Q_STR("Build Visual Studio 20%1.%2.%3 (%4 %5)")
        .arg(_MSC_VER / 100)
		.arg((_MSC_FULL_VER / 100000) % 100)
		.arg(_MSC_FULL_VER % 100000)
        .arg(Q_UTF8(__DATE__))
		.arg(Q_UTF8(__TIME__)));
#else
    ui.lblVersion->setText(Q_STR("Build Clang %1.%2.%3")
                            .arg(__clang_major__)
                            .arg(__clang_minor__)
                            .arg(__clang_patchlevel__));
#endif

    ui.lblLogo->setPixmap(ThemeManager::instance().appIcon().pixmap(128, 128));

    QFont font(Q_UTF8("MonoFont"));
    font.setBold(true);
    font.setPointSizeF(18);
    font.setStyleStrategy(QFont::PreferAntialias);

    ui.lblProjectTitle->setFont(font);
    ui.lblProjectTitle->setText(tr("XAMP2"));
    ui.lblProjectTitle->setStyleSheet(Q_UTF8("QLabel#lblProjectTitle { border: none; background: transparent; }"));

    ui.lblDescription->setText(Q_UTF8("Cross-platform native DSD and low latency playback music player."));

    QString domain_txt(Q_UTF8("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui.lblDomain->setText(domain_txt.arg(Q_UTF8("github.com/billlin0904/xamp2")));
    ui.lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2021 XAMP2 Project."));

    font.setBold(false);
    font.setPointSize(10);
    ui.txtBws->setFont(font);
    ui.txtBws->setVisible(false);

    QFile lincense_file(Q_UTF8("lincense.txt"));
    lincense_file.open(QIODevice::ReadOnly);
    lincense_ = QLatin1String(lincense_file.readAll());

    QFile credits_file(Q_UTF8("credits.txt"));
    credits_file.open(QIODevice::ReadOnly);
    credits_ = QLatin1String(credits_file.readAll());
}

void AboutPage::onCreditsOrLicenceChecked(bool checked) {
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
