#include <QApplication>
#include <QDir>

#include <widget/str_utilts.h>
#include <widget/localelanguage.h>

LocaleLanguage::LocaleLanguage() {
	setDefaultLanguage();
}

LocaleLanguage::LocaleLanguage(const QString& name) {
	QLocale l;
	l = name;
	setLanguageByLocale(l);
}

void LocaleLanguage::setDefaultLanguage() {
	QLocale l = QLocale::system();
	setLanguageByLocale(l);
}

void LocaleLanguage::setLanguage(QLocale::Language lang, QLocale::Country country) {
	QLocale l(lang, country);
	setLanguageByLocale(l);
}

void LocaleLanguage::setLanguageByLocale(const QLocale& l) {
	lang_ = l.language();
	country_ = l.country();
	native_name_lang_ = l.nativeLanguageName();
	eng_name_ = QLocale::languageToString(l.language());
	lang_iso_code_ = l.name().left(2);
	country_iso_code_ = l.name().mid(3);
}

static void switchTranslator(QTranslator& translator, const QString& filename) {
	QApplication::removeTranslator(&translator);

	auto path = QApplication::applicationDirPath();
	path.append(Q_UTF8("/langs/"));

	if (translator.load(path + filename)) {
		QApplication::installTranslator(&translator);
	}
}

LocaleLanguageManager::LocaleLanguageManager() {
}

QList<LocaleLanguage> LocaleLanguageManager::languageNames() {
	QList<LocaleLanguage> languagesList;

	auto path = QApplication::applicationDirPath();
	path.append(Q_UTF8("/langs"));
	QDir dir(path);

	auto fileNames = dir.entryList(QStringList(Q_UTF8("*.qm")));
	for (auto i = 0; i < fileNames.size(); ++i) {
		auto locale = fileNames[i];
		locale.truncate(locale.lastIndexOf(Q_UTF8(".")));
		languagesList.append(LocaleLanguage(locale));
	}

	return languagesList;
}

void LocaleLanguageManager::loadLanguage(const QString& lang) {
	if (current_lang_ != lang) {
		current_lang_ = lang;
		QLocale locale = QLocale(lang);
		QLocale::setDefault(locale);
		switchTranslator(translator_, QString(Q_UTF8("%1.qm")).arg(lang));
	}
}
