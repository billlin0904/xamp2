//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

class LocaleLanguage {
public:
	void setDefaultLanguage();
	
	void setLanguage(QLocale::Language lang, QLocale::Country country);

	QString lang() const {
		return lang_;
	}

	QString nativeNameWithCountryCode() const;
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
	QStringList languageNameNativeList() const;
private:
	LocaleLanguage lang_;
	QList<LocaleLanguage> translate_langs_;
};
