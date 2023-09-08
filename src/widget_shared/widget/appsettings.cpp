#include <widget/appsettings.h>

#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>

#include <base/logger_impl.h>
#include <base/crashhandler.h>

#include <stream/soxresampler.h>

#include <widget/jsonsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xmainwindow.h>

#include <widget/databasefacade.h>
#include <widget/log_util.h>
#include <widget/playerorder.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, EqSettings> AppSettings::eq_settings_;

void AppSettings::LoadIniFile(const QString& file_name) {
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
    const auto dont_show_me_again_list = ValueAsStringList(kAppSettingDontShowMeAgainList);
    return !dont_show_me_again_list.contains(string_hash);
}

void AppSettings::AddDontShowMeAgain(const QString& text) {
	const auto string_hash = QString::number(qHash(text));
	const auto dont_show_me_again_list = ValueAsStringList(kAppSettingDontShowMeAgainList);
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
            in.setEncoding(QStringConverter::Utf8);
            EqSettings settings;
            int i = 0;
            while (!in.atEnd()) {
                auto line = in.readLine();
                auto result = line.split(qTEXT(":"));
                auto str = result[1].toStdWString();
                if (result[0] == qTEXT("Preamp")) {
                    swscanf(str.c_str(), L"%f dB",
                        &settings.preamp);
                }
                else if (result[0].indexOf(qTEXT("Filter")) != -1) {
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
    EqSettings default_settings;
    default_settings.SetDefault();
    eq_settings_[qTEXT("Manual")] = default_settings;
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

Uuid AppSettings::ValueAsId(const QString& key) {
	const auto str = GetValue(key).toString();
	if (str.isEmpty()) {
        return Uuid::kNullUuid;
	}
	return Uuid::FromString(str.toStdString());
}

QList<QString> AppSettings::ValueAsStringList(QString const& key) {
	const auto setting_str = AppSettings::ValueAsString(key);
    if (setting_str.isEmpty()) {
        return {};
    }
    return setting_str.split(qTEXT(","), Qt::SkipEmptyParts);
}

void AppSettings::RemoveList(QString const& key, QString const & value) {
    auto values = ValueAsStringList(key);

    const auto itr = std::ranges::find(values, value);
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

    const auto itr = std::ranges::find(values, value);
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

void AppSettings::LoadSoxrSetting() {
    XAMP_LOG_DEBUG("LoadSoxrSetting.");

    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    default_setting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::VHQ);
    default_setting[kSoxrPhase] = 46;
    default_setting[kSoxrStopBand] = 100;
    default_setting[kSoxrPassBand] = 96;
    default_setting[kSoxrRollOffLevel] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);

    QMap<QString, QVariant> soxr_setting;
    soxr_setting[kSoxrDefaultSettingName] = default_setting;

    JsonSettings::SetDefaultValue(kSoxr, QVariant::fromValue(soxr_setting));

    if (JsonSettings::ValueAsMap(kSoxr).isEmpty()) {
        JsonSettings::SetValue(kSoxr, QVariant::fromValue(soxr_setting));
        AppSettings::SetValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
        AppSettings::SetDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    }
}

void AppSettings::LoadR8BrainSetting() {
    XAMP_LOG_DEBUG("LoadR8BrainSetting.");

    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    JsonSettings::SetDefaultValue(kR8Brain, QVariant::fromValue(default_setting));
    if (JsonSettings::ValueAsMap(kR8Brain).isEmpty()) {
        JsonSettings::SetValue(kR8Brain, QVariant::fromValue(default_setting));
    }
    if (!AppSettings::contains(kAppSettingResamplerType)) {
        AppSettings::SetValue(kAppSettingResamplerType, kR8Brain);
    }
}

void AppSettings::SaveLogConfig() {
    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> well_known_log_name;
    QMap<QString, QVariant> override_map;

    for (const auto& logger : XAM_LOG_MANAGER().GetAllLogger()) {
        if (logger->GetName() != std::string(kXampLoggerName)) {
            well_known_log_name[FromStdStringView(logger->GetName())] = log_util::GetLogLevelString(logger->GetLevel());
        }
    }

    min_level[kLogDefault] = qTEXT("debug");

    XAM_LOG_MANAGER().SetLevel(log_util::ParseLogLevel(min_level[kLogDefault].toString()));

    for (auto itr = well_known_log_name.begin()
        ; itr != well_known_log_name.end(); ++itr) {
        override_map[itr.key()] = itr.value();
        XAM_LOG_MANAGER().GetLogger(itr.key().toStdString())
            ->SetLevel(log_util::ParseLogLevel(itr.value().toString()));
    }

    min_level[kLogOverride] = override_map;
    log[kLogMinimumLevel] = min_level;
    JsonSettings::SetValue(kLog, QVariant::fromValue(log));
    JsonSettings::SetDefaultValue(kLog, QVariant::fromValue(log));
}

void AppSettings::LoadOrSaveLogConfig() {
    XAMP_LOG_DEBUG("LoadOrSaveLogConfig.");

    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> override_map;

    QMap<QString, QVariant> well_known_log_name;

    for (const auto& logger : XAM_LOG_MANAGER().GetAllLogger()) {
        if (logger->GetName() != std::string(kXampLoggerName)) {
            well_known_log_name[FromStdStringView(logger->GetName())] = qTEXT("info");
        }
    }

    if (JsonSettings::ValueAsMap(kLog).isEmpty()) {
        min_level[kLogDefault] = qTEXT("debug");

        XAM_LOG_MANAGER().SetLevel(log_util::ParseLogLevel(min_level[kLogDefault].toString()));

        for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            override_map[itr.key()] = itr.value();
            XAM_LOG_MANAGER().GetLogger(itr.key().toStdString())
                ->SetLevel(log_util::ParseLogLevel(itr.value().toString()));
        }

        min_level[kLogOverride] = override_map;
        log[kLogMinimumLevel] = min_level;
        JsonSettings::SetValue(kLog, QVariant::fromValue(log));
        JsonSettings::SetDefaultValue(kLog, QVariant::fromValue(log));
    }
    else {
        log = JsonSettings::ValueAsMap(kLog);
        min_level = log[kLogMinimumLevel].toMap();

        const auto default_level = min_level[kLogDefault].toString();
        XAM_LOG_MANAGER().SetLevel(log_util::ParseLogLevel(default_level));

        override_map = min_level[kLogOverride].toMap();

        for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            if (!override_map.contains(itr.key())) {
                override_map[itr.key()] = itr.value();
            }
        }

        for (auto itr = override_map.begin()
            ; itr != override_map.end(); ++itr) {
            const auto& log_name = itr.key();
            auto log_level = itr.value().toString();
            XAM_LOG_MANAGER().GetLogger(log_name.toStdString())
                ->SetLevel(log_util::ParseLogLevel(log_level));
        }
    }

#ifdef _DEBUG
    XAM_LOG_MANAGER().GetLogger(kCrashHandlerLoggerName)
        ->SetLevel(LOG_LEVEL_DEBUG);
#endif
}


void AppSettings::RegisterMetaType() {
    XAMP_LOG_DEBUG("RegisterMetaType.");

    // For QSetting read
    //qRegisterMetaTypeStreamOperators<AppEQSettings>("AppEQSettings");

    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<QSharedPointer<DatabaseFacade>>("QSharedPointer<DatabaseFacade>");
    qRegisterMetaType<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<Vector<TrackInfo>>("Vector<TrackInfo>");
    qRegisterMetaType<DeviceState>("DeviceState");
    qRegisterMetaType<PlayerState>("PlayerState");
    qRegisterMetaType<PlayListEntity>("PlayListEntity");
    qRegisterMetaType<Errors>("Errors");
    qRegisterMetaType<Vector<float>>("Vector<float>");
    qRegisterMetaType<QList<PlayListEntity>>("QList<PlayListEntity>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<ComplexValarray>("ComplexValarray");
    qRegisterMetaType<QList<TrackInfo>>("QList<TrackInfo>");
    qRegisterMetaType<Vector<TrackInfo>>("Vector<TrackInfo>");
    qRegisterMetaType<DriveInfo>("DriveInfo");
    qRegisterMetaType<EncodingProfile>("EncodingProfile");
    qRegisterMetaType<std::wstring>("std::wstring");
}

void AppSettings::LoadAppSettings() {
    RegisterMetaType();

    XAMP_LOG_DEBUG("LoadAppSettings.");

    AppSettings::SetDefaultEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    AppSettings::SetDefaultEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
    AppSettings::SetDefaultEnumValue(kAppSettingReplayGainScanMode, ReplayGainScanMode::RG_SCAN_MODE_FAST);
    AppSettings::SetDefaultEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);

    AppSettings::SetDefaultValue(kAppSettingDeviceType, kEmptyString);
    AppSettings::SetDefaultValue(kAppSettingDeviceId, kEmptyString);
    AppSettings::SetDefaultValue(kAppSettingVolume, 50);
    AppSettings::SetDefaultValue(kAppSettingUseFramelessWindow, true);
    AppSettings::SetDefaultValue(kLyricsFontSize, 12);
    AppSettings::SetDefaultValue(kAppSettingMinimizeToTray, true);
    AppSettings::SetDefaultValue(kAppSettingDiscordNotify, false);
    AppSettings::SetDefaultValue(kFlacEncodingLevel, 8);
    AppSettings::SetDefaultValue(kAppSettingShowLeftList, true);
    AppSettings::SetDefaultValue(kAppSettingReplayGainTargetGain, kReferenceGain);
    AppSettings::SetDefaultValue(kAppSettingReplayGainTargetLoudnes, kReferenceLoudness);
    AppSettings::SetDefaultValue(kAppSettingEnableReplayGain, true);
    AppSettings::SetDefaultValue(kEnableBitPerfect, true);
    AppSettings::SetDefaultValue(kAppSettingWindowState, false);
    AppSettings::SetDefaultValue(kAppSettingScreenNumber, 1);
    AppSettings::SetDefaultValue(kAppSettingEnableSpectrum, true);
    AppSettings::SetDefaultValue(kAppSettingEnableShortcut, true);
    AppSettings::SetDefaultValue(kAppSettingEnterFullScreen, false);
    AppSettings::SetDefaultValue(kAppSettingEnableSandboxMode, true);
    AppSettings::SetDefaultValue(kAppSettingEnableDebugStackTrace, true);

    AppSettings::SetDefaultValue(kAppSettingAlbumPlaylistColumnName, qTEXT("3, 6, 26"));
    AppSettings::SetDefaultValue(kAppSettingFileSystemPlaylistColumnName, qTEXT("3, 6, 26"));
    AppSettings::SetDefaultValue(kAppSettingCdPlaylistColumnName, qTEXT("3, 6, 26"));
    XAMP_LOG_DEBUG("loadAppSettings success.");
}