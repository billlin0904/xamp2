#include <ui_accountauthorizationpage.h>
#include <widget/util/str_utilts.h>
#include <thememanager.h>
#include <widget/accountauthorizationpage.h>

AccountAuthorizationPage::AccountAuthorizationPage(QWidget* parent)
	: QFrame(parent) {
	ui_ = new Ui::AccountAuthorizationPage();
	ui_->setupUi(this);
	ui_->expiresInLabel->setStyleSheet(qTEXT("background-color: transparent"));
	ui_->expiresInTimeLabel->setStyleSheet(qTEXT("background-color: transparent"));
	ui_->expiresAtLabel->setStyleSheet(qTEXT("background-color: transparent"));
	ui_->expiresAtTimeLabel->setStyleSheet(qTEXT("background-color: transparent"));
	ui_->accountLabel->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON_UNAUTHORIZATIONED));
	ui_->accountLabel->setStyleSheet(qTEXT("background-color: transparent"));
}

AccountAuthorizationPage::~AccountAuthorizationPage() {
	delete ui_;
}

void AccountAuthorizationPage::setOAuthToken(const OAuthToken& token) {
	ui_->expiresInTimeLabel->setText(QTime(0, 0, 0).addSecs(token.expires_in).toString());
	ui_->expiresAtTimeLabel->setText(QDateTime::fromSecsSinceEpoch(token.expires_at).toString());
	ui_->accountLabel->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON));
}
