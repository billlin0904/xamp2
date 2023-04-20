#include <widget/appsettings.h>

#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>

#include <base/logger_impl.h>
#include <widget/appsettingnames.h>
#include <widget/xmainwindow.h>
#include <widget/playerorder.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, EqSettings> AppSettings::eq_settings_;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
    LoadEqPreset();
}

const QMap<QString, EqSettings>& AppSettings::GetEqPreset() {
    return eq_settings_;
}

AppEQSettings AppSettings::GetEqSettings() {
    return GetValue(kAppSettingEQName).value<AppEQSettings>();
}

void AppSettings::SetEqSettings(AppEQSettings const& eq_settings) {
    SetValue(kAppSettingEQName, QVariant::fromValue(eq_settings));
    eq_settings_[eq_settings.name] = eq_settings.settings;
}

bool AppSettings::DontShowMeAgain(const QString& text) {
    const auto string_hash = QString::number(qHash(text));
    auto dont_show_me_again_list = ValueAsStringList(kAppSettingDontShowMeAgainList);
    return !dont_show_me_again_list.contains(string_hash);
}

void AppSettings::AddDontShowMeAgain(const QString& text) {
	const auto string_hash = QString::number(qHash(text));
    auto dont_show_me_again_list = ValueAsStringList(kAppSettingDontShowMeAgainList);
    if (!dont_show_me_again_list.contains(string_hash)) {
        AddList(kAppSettingDontShowMeAgainList, string_hash);
    }
}

void AppSettings::LoadEqPreset() {
	const auto path = QDir::currentPath() + qTEXT("/eqpresets/");
	const auto file_ext = QStringList() << qTEXT("*.*");

    for (QDirIterator itr(path, file_ext, QDir::Files | QDir::NoDotAndDotDot);
        itr.hasNext();) {
        const auto filepath = itr.next();
        const QFileInfo file_info(filepath);
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            EqSettings settings;
            //settings.SetDefault();
            int i = 0;
            while (!in.atEnd()) {
                auto line = in.readLine();
                auto result = line.split(qTEXT(":"));
                auto str = result[1].toStdWString();
                if (result[0] == qTEXT("Preamp")) {
                    swscanf(str.c_str(), L"%f dB",
                        &settings.preamp);
                }
                else if (result[0].indexOf(qTEXT("Filter") != -1)) {
                    settings.bands.push_back(EqBandSetting());
                    auto pos = str.find(L"Fc");
                    swscanf(&str[pos], L"Fc %f Hz",
                        &settings.bands[i].frequency);
	                pos = str.find(L"Gain");
                    if (pos == std::wstring::npos) {
                        continue;
                    }
                    swscanf(&str[pos], L"Gain %f dB Q %f",
                        &settings.bands[i].gain, &settings.bands[i].Q);
                    ++i;
                    XAMP_LOG_TRACE("Parse {}", line.toStdString());
                }
            }
            eq_settings_[file_info.baseName()] = settings;
        }
    }
    eq_settings_[qTEXT("Manual")] = EqSettings();
}

void AppSettings::save() {
	if (!settings_) {
		return;
	}
    settings_->sync();
}

QString AppSettings::DefaultCachePath() {
    auto folder_path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    return folder_path[0];
}

QString AppSettings::GetMyMusicFolderPath() {
    if (!contains(kAppSettingMyMusicFolderPath)) {
        auto folder_path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
        if (folder_path.isEmpty()) {
            return kEmptyString;
        }
        return folder_path[0];
    }
    return ValueAsString(kAppSettingMyMusicFolderPath);
}

Uuid AppSettings::ValueAsID(const QString& key) {
	auto str = GetValue(key).toString();
	if (str.isEmpty()) {
        return Uuid::kNullUuid;
	}
	return Uuid::FromString(str.toStdString());
}

QList<QString> AppSettings::ValueAsStringList(QString const& key) {
    auto setting_str = AppSettings::ValueAsString(key);
    if (setting_str.isEmpty()) {
        return {};
    }
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 1)
    return setting_str.split(qTEXT(","), Qt::SkipEmptyParts);
#else
    return setting_str.split(Q_UTF8(","), QString::SkipEmptyParts);
#endif
}

void AppSettings::RemoveList(QString const& key, QString const & value) {
    auto values = ValueAsStringList(key);

    auto itr = std::find(values.begin(), values.end(), value);
    if (itr != values.end()) {
        values.erase(itr);
    }

    QStringList all;
    Q_FOREACH(auto id, values) {
        all << id;
    }
    AppSettings::SetValue(key, all.join(qTEXT(",")));
}

void AppSettings::AddList(QString const& key, QString const & value) {
    auto values = ValueAsStringList(key);

    auto itr = std::find(values.begin(), values.end(), value);
    if (itr != values.end()) {
        return;
    }

    values.append(value);
    QStringList all;
    Q_FOREACH(auto id, values) {
        all << id;
    }
    SetValue(key, all.join(qTEXT(",")));
}

QSize AppSettings::ValueAsSize(const QString& width_key,
	const QString& height_key) {
	return QSize{
		ValueAsInt(width_key),
		ValueAsInt(height_key),
	};
}

QVariant AppSettings::GetValue(const QString& key) {
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
    auto v = settings_->value(key);
    return v;
}

int32_t AppSettings::ValueAsInt(const QString& key) {
	return GetValue(key).toInt();
}

void AppSettings::LoadLanguage(const QString& lang) {
	manager_.LoadLanguage(lang);
}

QLocale AppSettings::locale() {
    return manager_.locale();
}
