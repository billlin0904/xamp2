//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLocale>
#include <QString>
#include <QTranslator>

#include <widget/widget_shared_global.h>
#include <widget/str_utilts.h>

class XAMP_WIDGET_SHARED_EXPORT LocaleLanguage {
public:
	LocaleLanguage();

	explicit LocaleLanguage(const QString& name);

	void setDefaultLanguage();
	
	void setLanguage(QLocale::Language lang, QLocale::Country country);

	QString language() const {
		return lang_;
	}

	QString langIsoCode() const {
		return lang_iso_code_;
	}

	QString countryIsoCode() const {
		return country_iso_code_;
	}

	QString isoCode() const {
		return langIsoCode() + qTEXT("_") + countryIsoCode();
	}

	QString nativeNameLang() const {
		return native_name_lang_;
	}
private:
	void setLanguageByLocale(const QLocale& locale);

	QString lang_;
	QString country_;
	QString native_name_lang_;
	QString eng_name_;
	QString lang_iso_code_;
	QString country_iso_code_;
};

class XAMP_WIDGET_SHARED_EXPORT LocaleLanguageManager {
public:
	LocaleLanguageManager();

	static QList<LocaleLanguage> languageNames();

	void loadLanguage(const QString& lang);

	QLocale locale() const;
private:
	QTranslator translator_;
	QString current_lang_;
	QLocale locale_;
};
