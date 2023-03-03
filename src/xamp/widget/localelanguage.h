//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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

	void SetDefaultLanguage();
	
	void SetLanguage(QLocale::Language lang, QLocale::Country country);

	QString Language() const {
		return lang_;
	}

	QString LangIsoCode() const {
		return lang_iso_code_;
	}

	QString CountryIsoCode() const {
		return country_iso_code_;
	}

	QString GetIsoCode() const {
		return LangIsoCode() + qTEXT("_") + CountryIsoCode();
	}

	QString NativeNameLang() const {
		return native_name_lang_;
	}
private:
	void SetLanguageByLocale(const QLocale& locale);

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

	static QList<LocaleLanguage> LanguageNames();

	void LoadLanguage(const QString& lang);

	QLocale locale() const;
private:
	QTranslator translator_;
	QString current_lang_;
	QLocale locale_;
};
