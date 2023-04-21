#include <widget/localelanguage.h>

#include <QApplication>
#include <QDir>

#include <widget/str_utilts.h>

LocaleLanguage::LocaleLanguage() {
	SetDefaultLanguage();
}

LocaleLanguage::LocaleLanguage(const QString& name) {
	QLocale locale;
	locale = name;
	SetLanguageByLocale(locale);
}

void LocaleLanguage::SetDefaultLanguage() {
	auto locale = QLocale::system();
	SetLanguageByLocale(locale);
}

void LocaleLanguage::SetLanguage(QLocale::Language lang, QLocale::Country country) {
	const QLocale locale(lang, country);
	SetLanguageByLocale(locale);
}

void LocaleLanguage::SetLanguageByLocale(const QLocale& locale) {
	lang_ = locale.language();
	country_ = locale.country();
	native_name_lang_ = locale.nativeLanguageName();
	eng_name_ = QLocale::languageToString(locale.language());
	lang_iso_code_ = locale.name().left(2);
	country_iso_code_ = locale.name().mid(3);
}

static void switchTranslator(QTranslator& translator, const QString& filename) {
	QApplication::removeTranslator(&translator);

	auto path = QApplication::applicationDirPath();
	path.append(qTEXT("/langs/"));

	if (translator.load(path + filename)) {
		QApplication::installTranslator(&translator);
	}
}

LocaleLanguageManager::LocaleLanguageManager() = default;

QList<LocaleLanguage> LocaleLanguageManager::LanguageNames() {
	QList<LocaleLanguage> languages_list;

	auto path = QApplication::applicationDirPath();
	path.append(qTEXT("/langs"));
	QDir dir(path);

	auto file_names = dir.entryList(QStringList(qTEXT("*.qm")));
	for (auto i = 0; i < file_names.size(); ++i) {
		auto locale = file_names[i];
		locale.truncate(locale.lastIndexOf(qTEXT(".")));
		languages_list.append(LocaleLanguage(locale));
	}

	return languages_list;
}

void LocaleLanguageManager::LoadLanguage(const QString& lang) {
	if (current_lang_ != lang) {
		current_lang_ = lang;
		locale_ = QLocale(lang);
		QLocale::setDefault(locale_);
		switchTranslator(translator_, QString(qTEXT("%1.qm")).arg(lang));
	}
}

QLocale LocaleLanguageManager::locale() const {
	return locale_;
}
