#include <widget/aboutpage.h>
#include <ui_aboutdialog.h>
#include <QFile>
#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>

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
    ui_->lblProjectTitle->setStyleSheet("QLabel#lblProjectTitle { border: none; background: transparent; }"_str);

	dp_font.setBold(false);
    dp_font.setPointSizeF(qTheme.fontSize(8));
    ui_->lblDescription->setFont(dp_font);
    ui_->lblDescription->setText(QString::fromStdWString(L"Cross-platform native DSD and low latency playback music player."));

    ui_->lblDomain->setText(
        "<html><head/><body>"
        "<a href=\"https://github.com/billlin0904/xamp2\">Github</a>"
        "</body></html>"_str);

    auto* buy_me_coffee_button = new QPushButton(this);
    buy_me_coffee_button->setObjectName("buyMeCoffeeButton"_str);
    buy_me_coffee_button->setCursor(Qt::PointingHandCursor);
    buy_me_coffee_button->setFocusPolicy(Qt::NoFocus);
    buy_me_coffee_button->setFlat(true);
    buy_me_coffee_button->setToolTip("Buy Me a Coffee"_str);
    buy_me_coffee_button->setIcon(QIcon(":/xamp/Resource/White/buymeacoffee.png"_str));
    buy_me_coffee_button->setIconSize(QSize(92, 26));
    buy_me_coffee_button->setFixedSize(QSize(100, 30));
    buy_me_coffee_button->setStyleSheet(
        "QPushButton#buyMeCoffeeButton { border: none; background: transparent; padding: 0px; }"
        "QPushButton#buyMeCoffeeButton:hover { background: rgba(255, 255, 255, 18); border-radius: 4px; }"_str);
    ui_->horizontalLayout_2->insertWidget(3, buy_me_coffee_button, 0, Qt::AlignVCenter);
    (void)QObject::connect(buy_me_coffee_button, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://buymeacoffee.com/billlin0904"_str));
    });
    ui_->lbIGithubIcon->setPixmap(qTheme.githubIcon());
    ui_->lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2026 XAMP2 Project."));

    QFont font(qTheme.uiFont());
    font.setBold(false);
    font.setPointSize(qTheme.defaultFontSize());
    ui_->txtBws->setFont(font);
    ui_->txtBws->setVisible(false);

    QFile lincense_file("lincense.txt"_str);
    if (lincense_file.open(QIODevice::ReadOnly)) {
        license_ = QLatin1String(lincense_file.readAll());
    }

    QFile credits_file("credits.txt"_str);
    if (credits_file.open(QIODevice::ReadOnly)) {
        credits_ = QLatin1String(credits_file.readAll());
    }

    auto f = font;
    f.setPointSizeF(qTheme.fontSize(8));
    ui_->lblAppBuild->setFont(f);
#if defined(_MSC_VER)
    ui_->lblAppBuild->setText("Build MSVC: "_str + qFormat(__DATE__) + " "_str + qFormat(__TIME__));
#elif defined(__clang__)
    ui_->lblAppBuild->setText("Build Clang: "_str + qFormat(__DATE__) + " "_str + qFormat(__TIME__));
#elif defined(__GNUC__)
    ui_->lblAppBuild->setText("Build GCC: "_str + qFormat(__DATE__) + " "_str + qFormat(__TIME__));
#else
    ui_->lblAppBuild->setText("Build: "_str + qFormat(__DATE__) + " "_str + qFormat(__TIME__));
#endif

    ui_->lblCopying->setFont(f);
    ui_->lblAppBuild->setMinimumWidth(220);
    ui_->lblAppBuild->setMaximumWidth(QWIDGETSIZE_MAX);
    ui_->lblAppBuild->setAlignment(Qt::AlignCenter);

    (void)QObject::connect(ui_->btnCredits,
        &QPushButton::clicked,
        this, 
        &AboutPage::onCreditsOrLicenseChecked);
    (void)QObject::connect(ui_->btnLicense,
        &QPushButton::clicked,
        this,
        &AboutPage::onCreditsOrLicenseChecked);

    (void)QObject::connect(ui_->restartAppButton,
        &QPushButton::clicked, [this]() {
            emit restartApp();
        });

    ui_->restartAppButton->hide();

    //ui_->waitForUpdateProcessIndicator->startAnimation();
    //ui_->waitForUpdateProcessIndicator->setStoppedIcon(qTheme.fontIcon(Glyphs::ICON_CIRCLE_CHECK));

    ui_->lblLogo->setStyleSheet("background-color: transparent"_str);
    ui_->lblProjectTitle->setStyleSheet("background-color: transparent"_str);
    ui_->lblCopying->setStyleSheet("background-color: transparent"_str);
    ui_->lblAppBuild->setStyleSheet("background-color: transparent; color: gray;"_str);
}

AboutPage::~AboutPage() {
    delete ui_;
}

void AboutPage::onThemeChangedFinished(ThemeColor theme_color) {
    ui_->lbIGithubIcon->setPixmap(qTheme.githubIcon());
}

void AboutPage::onCreditsOrLicenseChecked(bool checked) {
	const auto* sender_ptr = sender();
    if (sender_ptr->objectName() == "btnLicense"_str) {
        ui_->txtBws->setText(license_);
        ui_->btnCredits->setChecked(false);
    }
    else if (sender_ptr->objectName() == "btnCredits"_str) {
        ui_->txtBws->setText(credits_);
        ui_->btnLicense->setChecked(false);
    }
    ui_->txtBws->setVisible(checked);
    ui_->wdtContent->setVisible(!checked);
}

void AboutPage::onUpdateNewVersion(const QVersionNumber& version) {
    //ui_->lblAppBuild->setText(tr("Ver. ")
    //    + formatVersion(version));
    //ui_->waitForUpdateProcessIndicator->stopAnimation();
    //ui_->restartAppButton->show();
}
