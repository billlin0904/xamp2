#include <widget/localelanguage.h>

#include <QApplication>
#include <QDir>

#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <base/logger.h>
#include <base/logger_impl.h>

namespace {
	void switchTranslator(QTranslator& translator, const QString& filename) {
		qApp->removeTranslator(&translator);

		auto path = qApp->applicationDirPath();
		path.append(qTEXT("/langs/"));

		if (translator.load(path + filename)) {
			qApp->installTranslator(&translator);
		} else {
			XAMP_LOG_DEBUG("Failure to load translator {} file.", filename.toStdString());
		}
	}
}

LocaleLanguage::LocaleLanguage() {
	setDefaultLanguage();
}

LocaleLanguage::LocaleLanguage(const QString& name) {
	const QLocale locale(name);
	setLanguageByLocale(locale);
}

void LocaleLanguage::setDefaultLanguage() {
	const auto locale = QLocale::system();
	setLanguageByLocale(locale);
}

void LocaleLanguage::setLanguage(QLocale::Language lang, QLocale::Country country) {
	const QLocale locale(lang, country);
	setLanguageByLocale(locale);
}

void LocaleLanguage::setLanguageByLocale(const QLocale& locale) {
	lang_ = locale.language();
	country_ = locale.country();
	native_name_lang_ = locale.nativeLanguageName();
	eng_name_ = QLocale::languageToString(locale.language());
	lang_iso_code_ = locale.name().left(2);
	country_iso_code_ = locale.name().mid(3);
}

LocaleLanguageManager::LocaleLanguageManager() = default;

QList<LocaleLanguage> LocaleLanguageManager::languageNames() {
	QList<LocaleLanguage> languages_list;

	auto path = QApplication::applicationDirPath();
	path.append(qTEXT("/langs"));
	QDir dir(path);

	auto file_names = dir.entryList(QStringList(qTEXT("*.qm")));
	for (auto locale : file_names) {
		if (locale.contains("qt") || locale.contains("widget_shared")) {
			continue;
		}
		locale.truncate(locale.lastIndexOf(qTEXT(".")));
		languages_list.append(LocaleLanguage(locale));
	}

	return languages_list;
}

void LocaleLanguageManager::loadPrefixTranslator(QTranslator& translator, const QString& prefix, const QString& lang) {
	const auto qt_lang_file_name = QString(qTEXT("%1_%2.qm")).arg(prefix).arg(lang);
	switchTranslator(translator, qt_lang_file_name);
}

void LocaleLanguageManager::loadLanguage(const QString& lang) {
	if (current_lang_ != lang) {
		current_lang_ = lang;
		locale_ = QLocale(lang);
		QLocale::setDefault(locale_);
		loadPrefixTranslator(qt_translator_, qTEXT("qt"), lang);
		loadPrefixTranslator(widget_shared_translator_, qTEXT("widget_shared"), lang);
		switchTranslator(translator_, QString(qTEXT("%1.qm")).arg(lang));
	}
}

QLocale LocaleLanguageManager::locale() const {
	return locale_;
}
