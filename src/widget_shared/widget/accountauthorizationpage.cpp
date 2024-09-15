#include <ui_accountauthorizationpage.h>
#include <widget/util/str_util.h>
#include <thememanager.h>
#include <widget/accountauthorizationpage.h>

AccountAuthorizationPage::AccountAuthorizationPage(QWidget* parent)
	: QFrame(parent) {
	ui_ = new Ui::AccountAuthorizationPage();
	ui_->setupUi(this);
	ui_->expiresInLabel->setStyleSheet("background-color: transparent"_str);
	ui_->expiresInTimeLabel->setStyleSheet("background-color: transparent"_str);
	ui_->expiresAtLabel->setStyleSheet("background-color: transparent"_str);
	ui_->expiresAtTimeLabel->setStyleSheet("background-color: transparent"_str);
	ui_->accountLabel->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON_UNAUTHORIZATIONED));
	ui_->accountLabel->setStyleSheet("background-color: transparent"_str);
}

AccountAuthorizationPage::~AccountAuthorizationPage() {
	delete ui_;
}

void AccountAuthorizationPage::setOAuthToken(const OAuthToken& token) {
	ui_->expiresInTimeLabel->setText(QDateTime::fromSecsSinceEpoch(token.expires_in + token.expires_at).toString());
	ui_->expiresAtTimeLabel->setText(QDateTime::fromSecsSinceEpoch(token.expires_at).toString());
	ui_->accountLabel->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON));
}
