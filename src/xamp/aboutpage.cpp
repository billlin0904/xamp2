#include <QFile>

#include <base/simd.h>
#include <widget/str_utilts.h>
#include "thememanager.h"
#include "aboutpage.h"

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

#ifdef Q_OS_WIN32
    ui.lblVersion->setText(Q_STR("Build Visual Studio 20%1.%2.%3 (%4 %5), %6 AVX2, Qt %7")
        .arg(_MSC_VER / 100)
		.arg((_MSC_FULL_VER / 100000) % 100)
		.arg(_MSC_FULL_VER % 100000)
        .arg(Q_TEXT(__DATE__))
		.arg(Q_TEXT(__TIME__))
        .arg(SIMD::IsCPUSupportAVX2() ? Q_TEXT("Support") : Q_TEXT("Unsupport"))
		.arg(Q_TEXT(QT_VERSION_STR)));
#else
    ui.lblVersion->setText(Q_STR("Build Clang %1.%2.%3")
                            .arg(__clang_major__)
                            .arg(__clang_minor__)
                            .arg(__clang_patchlevel__));
#endif

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
