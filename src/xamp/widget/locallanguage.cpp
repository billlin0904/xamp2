#include <QLocale>

#include <widget/str_utilts.h>
#include <widget/localelanguage.h>

static void ParseISO6391(const QString &file_name) {

}

void LocaleLanguage::setDefaultLanguage() {
	QLocale l;
	setLanguageByLocale(l);
}

void LocaleLanguage::setLanguage(QLocale::Language lang, QLocale::Country country) {
	QLocale l(lang, country);
	setLanguageByLocale(l);
}

QString LocaleLanguage::nativeNameWithCountryCode() const {
	if (country_iso_code_.isEmpty()) {
		return native_name_lang_ + Q_UTF8(" (") + country_iso_code_ + Q_UTF8(")");
	}
	return native_name_lang_;
}

void LocaleLanguage::setLanguageByLocale(const QLocale& l) {
	lang_ = l.language();
	country_ = l.country();
	native_name_lang_ = l.nativeLanguageName();
	eng_name_ = QLocale::languageToString(l.language());
	lang_iso_code_ = l.name().left(2);
	country_iso_code_ = l.name().mid(3);
}

QStringList LocaleLanguageManager::languageNameNativeList() const {
	QStringList languagesList;
	languagesList.clear();

	if (lang_.lang() != QLocale::C) {
		languagesList.append(lang_.nativeNameWithCountryCode());
	}

	for (auto i = 0; i < translate_langs_.count(); ++i) {
		languagesList.append(translate_langs_[i].nativeNameWithCountryCode());
	}

	return languagesList;
}
