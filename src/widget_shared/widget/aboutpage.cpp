#include <widget/aboutpage.h>
#include <ui_aboutdialog.h>
#include <QFile>

#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <version.h>
#include <thememanager.h>

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::AboutDialog();
    ui_->setupUi(this);

    ui_->lblLogo->setPixmap(qTheme.applicationIcon().pixmap(128, 128));

    QFont dp_font(qTheme.displayFont());
    dp_font.setBold(true);
    dp_font.setPointSizeF(qTheme.fontSize(26));

    ui_->lblProjectTitle->setFont(dp_font);
    ui_->lblProjectTitle->setText(kApplicationName);
    ui_->lblProjectTitle->setStyleSheet(qTEXT("QLabel#lblProjectTitle { border: none; background: transparent; }"));

	dp_font.setBold(false);
    dp_font.setPointSizeF(qTheme.fontSize(8));
    ui_->lblDescription->setFont(dp_font);
    ui_->lblDescription->setText(QString::fromStdWString(L"Cross-platform native DSD and low latency playback music player."));

    QString domain_txt(qTEXT("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui_->lblDomain->setText(domain_txt.arg(qTEXT("github.com/billlin0904/xamp2")));
    ui_->lbIGithubIcon->setPixmap(qTheme.githubIcon());
    ui_->lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2024 XAMP2 Project."));

    QFont font(qTheme.uiFont());
    font.setBold(false);
    font.setPointSize(qTheme.defaultFontSize());
    ui_->txtBws->setFont(font);
    ui_->txtBws->setVisible(false);

    QFile lincense_file(qTEXT("lincense.txt"));
    if (lincense_file.open(QIODevice::ReadOnly)) {
        license_ = QLatin1String(lincense_file.readAll());
    }

    QFile credits_file(qTEXT("credits.txt"));
    if (credits_file.open(QIODevice::ReadOnly)) {
        credits_ = QLatin1String(credits_file.readAll());
    }

    ui_->lblAppBuild->setText(tr("Ver. ") + kApplicationVersion);

    (void)QObject::connect(ui_->btnCredits,
        &QPushButton::clicked,
        this, 
        &AboutPage::OnCreditsOrLicenseChecked);
    (void)QObject::connect(ui_->btnLicense,
        &QPushButton::clicked,
        this,
        &AboutPage::OnCreditsOrLicenseChecked);

    (void)QObject::connect(ui_->restartAppButton,
        &QPushButton::clicked, [this]() {
            emit RestartApp();
        });

    ui_->restartAppButton->hide();

    //ui_->waitForUpdateProcessIndicator->startAnimation();
    //ui_->waitForUpdateProcessIndicator->setStoppedIcon(qTheme.fontIcon(Glyphs::ICON_CIRCLE_CHECK));

    ui_->lblLogo->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblProjectTitle->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblCopying->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblAppBuild->setStyleSheet(qTEXT("background-color: transparent; color: gray;"));
}

AboutPage::~AboutPage() {
    delete ui_;
}

void AboutPage::onThemeChangedFinished(ThemeColor theme_color) {
    ui_->lbIGithubIcon->setPixmap(qTheme.githubIcon());
}

void AboutPage::OnCreditsOrLicenseChecked(bool checked) {
	const auto* sender_ptr = sender();
    if (sender_ptr->objectName() == qTEXT("btnLicense")) {
        ui_->txtBws->setText(license_);
        ui_->btnCredits->setChecked(false);
    }
    else if (sender_ptr->objectName() == qTEXT("btnCredits")) {
        ui_->txtBws->setText(credits_);
        ui_->btnLicense->setChecked(false);
    }
    ui_->txtBws->setVisible(checked);
    ui_->wdtContent->setVisible(!checked);
}

void AboutPage::OnUpdateNewVersion(const QVersionNumber& version) {
    //ui_->lblAppBuild->setText(tr("Ver. ")
    //    + formatVersion(version));
    //ui_->waitForUpdateProcessIndicator->stopAnimation();
    //ui_->restartAppButton->show();
}
