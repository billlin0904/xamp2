//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLocale>
#include <QString>
#include <QTranslator>

#include <widget/str_utilts.h>

class LocaleLanguage {
public:
	LocaleLanguage();

	explicit LocaleLanguage(const QString& name);

	void setDefaultLanguage();
	
	void setLanguage(QLocale::Language lang, QLocale::Country country);

	QString lang() const {
		return lang_;
	}

	QString langIsoCode() const {
		return lang_iso_code_;
	}

	QString countryIsoCode() const {
		return country_iso_code_;
	}

	QString getIsoCode() const {
		return langIsoCode() + Q_UTF8("_") + countryIsoCode();
	}

	QString nativeNameLang() const {
		return native_name_lang_;
	}
private:
	void setLanguageByLocale(const QLocale& l);
	QString lang_;
	QString country_;
	QString native_name_lang_;
	QString eng_name_;
	QString lang_iso_code_;
	QString country_iso_code_;
};

class LocaleLanguageManager {
public:
	LocaleLanguageManager();

	static QList<LocaleLanguage> languageNames();

	void loadLanguage(const QString& lang);
private:
	QTranslator translator_;
	QString current_lang_;
};
