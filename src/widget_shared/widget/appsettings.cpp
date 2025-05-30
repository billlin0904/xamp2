#include <widget/appsettings.h>

#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>

#include <base/logger_impl.h>
#include <base/crashhandler.h>
#include <base/port.h>

#include <stream/soxresampler.h>

#include <widget/jsonsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xmainwindow.h>

#include <widget/databasefacade.h>
#include <widget/util/log_util.h>
#include <widget/util/ui_util.h>
#include <widget/playerorder.h>
#include <widget/playlistentity.h>
#include <widget/databasecoverid.h>

namespace {
	void saveSoxrSetting(const QString& setting_name, int32_t sample_rate) {
		QMap<QString, QVariant> default_setting;

		default_setting[kResampleSampleRate] = sample_rate;
		default_setting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::SINC_VHQ);
		default_setting[kSoxrPhase] = 46;
		default_setting[kSoxrStopBand] = 100;
		default_setting[kSoxrPassBand] = 96;
		default_setting[kSoxrRollOffLevel] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);

		QMap<QString, QVariant> soxr_setting;
		if (qJsonSettings.valueAsMap(kSoxr).isEmpty()) {
			soxr_setting[setting_name] = default_setting;
		}
		else {
			soxr_setting = qJsonSettings.valueAsMap(kSoxr);
			soxr_setting[setting_name] = default_setting;
		}
		qJsonSettings.setValue(kSoxr, QVariant::fromValue(soxr_setting));
	}
}

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
	XAMP_ENSURES(settings_ != nullptr);
	loadEqPreset();
}

const QMap<QString, EqSettings>& AppSettings::eqPreset() {
	return eq_settings_;
}

AppEQSettings AppSettings::eqSettings() {
	return valueAs(kAppSettingEQName).value<AppEQSettings>();
}

void AppSettings::setEqSettings(const AppEQSettings& eq_settings) {
	setValue(kAppSettingEQName, QVariant::fromValue(eq_settings));
	eq_settings_[eq_settings.name] = eq_settings.settings;
}

bool AppSettings::dontShowMeAgain(const QString& text) {
	const auto string_hash = QString::number(qHash(text));
	const auto dont_show_me_again_list = valueAsStringList(kAppSettingDontShowMeAgainList);
	return !dont_show_me_again_list.contains(string_hash);
}

void AppSettings::addDontShowMeAgain(const QString& text) {
	const auto string_hash = QString::number(qHash(text));
	const auto dont_show_me_again_list = valueAsStringList(kAppSettingDontShowMeAgainList);
	if (!dont_show_me_again_list.contains(string_hash)) {
		addList(kAppSettingDontShowMeAgainList, string_hash);
	}
}

void AppSettings::parseGraphicEq(const QFileInfo file_info, QFile& file) {
	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);

	if (in.atEnd()) {
		return;
	}

	const auto line = in.readLine();
    const QStringList parameter_list(line.split(":"_str));
	if (parameter_list.count() < 2) {
		return;
	}

    auto node_list = parameter_list.at(1).split(";"_str);
	EqSettings settings;

	for (auto& node_str : node_list) {
        auto values = node_str.trimmed().split(" "_str, Qt::SkipEmptyParts);
		if (values.count() != 2) {
			continue;
		}

		const auto freq = values.at(0).toDouble();
		const auto gain = values.at(1).toDouble();

		EqBandSetting setting;
		setting.type = EQFilterTypes::FT_ALL_PEAKING_EQ;
		setting.frequency = freq;
		setting.gain = gain;
		setting.Q = 1.4;
		settings.bands.push_back(setting);
	}
	eq_settings_[file_info.baseName()] = settings;
}

void AppSettings::parseEQPreset(const QFileInfo file_info, QFile& file) {
	constexpr std::array<std::pair<std::wstring_view, EQFilterTypes>, 3> filter_types{
		std::make_pair(L"LSC", EQFilterTypes::FT_LOW_SHELF),
		std::make_pair(L"HSC", EQFilterTypes::FT_LOW_HIGH_SHELF),
		std::make_pair(L"PK", EQFilterTypes::FT_ALL_PEAKING_EQ),
	};

	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);

	EqSettings settings;
	int i = 0;

	while (!in.atEnd()) {
		auto line = in.readLine();
		if (line.startsWith("#"_str)) {
			continue;
		}
		auto result = line.split(":"_str);
		auto str = result[1].toStdWString();
		if (result[0] == "Preamp"_str) {
			port_swscanf(str.c_str(), L"%f dB",
			        &settings.preamp);
		}
		else if (result[0].indexOf("Filter"_str) != -1) {
			settings.bands.emplace_back();
			auto pos = str.find(L"Fc");
			port_swscanf(&str[pos], L"Fc %f Hz",
			        &settings.bands[i].frequency);
			for (auto& filter_type : filter_types) {
				pos = str.find(filter_type.first);
				if (pos != std::wstring::npos) {
					settings.bands[i].type = filter_type.second;
					break;
				}
			}
			pos = str.find(L"Gain");
			if (pos == std::wstring::npos) {
				continue;
			}
			port_swscanf(&str[pos], L"Gain %f dB Q %f",
			        &settings.bands[i].gain, &settings.bands[i].Q);
			++i;
			XAMP_LOG_TRACE("Parse {}", line.toStdString());
		}
	}
	std::sort(settings.bands.begin(), settings.bands.end(), [](auto a, auto b) {
		return a.frequency < b.frequency;
		});
	eq_settings_[file_info.baseName()] = settings;
}

void AppSettings::loadEqPreset() {
	eq_settings_.clear();

	const auto path = applicationPath() + "/eqpresets/"_str;
	const auto file_ext = QStringList() << "*.*"_str;

	for (QDirIterator itr(path, file_ext, QDir::Files | QDir::NoDotAndDotDot);
	     itr.hasNext();) {
		const auto filepath = itr.next();
		const QFileInfo file_info(filepath);
		QFile file(filepath);
		if (file.open(QIODevice::ReadOnly)) {
			parseEQPreset(file_info, file);
		}
	}
	EqSettings default_settings;
	default_settings.SetDefault();
	eq_settings_["Manual"_str] = default_settings;
}

void AppSettings::save() {
	if (!settings_) {
		return;
	}
	settings_->sync();
}

QLocale AppSettings::locale() const {
	return manager_.locale();
}

QString AppSettings::getOrCreateLrcCachePath() {
	auto cache_path = getOrCreateCachePath() + R"(LrcCache\)"_str;
	const QDir dir(cache_path);
	if (!dir.exists()) {
		if (!dir.mkdir(cache_path)) {
			XAMP_LOG_ERROR("Create cache dir failure!");
		}
	}
	return cache_path;
}

QString AppSettings::getOrCreateImageCachePath() {
	auto cache_path = getOrCreateCachePath() + R"(ImageCache\)"_str;
	const QDir dir(cache_path);
	if (!dir.exists()) {
		if (!dir.mkdir(cache_path)) {
			XAMP_LOG_ERROR("Create cache dir failure!");
		}
	}
	return cache_path;
}

QString AppSettings::getOrCreateCachePath() {
    QString cache_path;
#ifdef Q_OS_WIN
	if (!qAppSettings.contains(kAppSettingCachePath)) {
		auto folder_path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);

		const QList<QString> paths{
			applicationPath() + QString("/Cache/"_str),
			folder_path[0],
		};

		Q_FOREACH(const auto path, paths) {
			cache_path = path;
			const QDir dir(cache_path);
			if (!dir.exists()) {
				if (!dir.mkdir(cache_path)) {
					XAMP_LOG_ERROR("Create cache dir failure!");
				}
				else {
					break;
				}
			}
		}

		cache_path = toNativeSeparators(cache_path);
		qAppSettings.setValue(kAppSettingCachePath, cache_path);
	}
	else {
		cache_path = qAppSettings.valueAsString(kAppSettingCachePath);
	}
#else
    cache_path = applicationPath() + "/Cache/");
    const QDir dir(cache_path);
    if (!dir.exists()) {
        if (!dir.mkdir(cache_path)) {
            XAMP_LOG_ERROR("Create cache dir failure!");
        }
    }
#endif
    if (cache_path.back() != QDir::separator()) {
        cache_path += QDir::separator();
    }

	return cache_path;
}

QString AppSettings::myMusicFolderPath() {
	if (!contains(kAppSettingMyMusicFolderPath)) {
		auto folder_path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
		if (folder_path.isEmpty()) {
			return kEmptyString;
		}
		return folder_path[0];
	}
	return valueAsString(kAppSettingMyMusicFolderPath);
}

Uuid AppSettings::valueAsId(const QString& key) {
	const auto str = valueAs(key).toString();
	if (str.isEmpty()) {
		return Uuid::kNullUuid;
	}
	return Uuid::FromString(str.toStdString());
}

QList<QString> AppSettings::valueAsStringList(const QString& key) {
	if (key.isEmpty()) {
		return {};
	}
	const auto setting_str = valueAsString(key);
	if (setting_str.isEmpty()) {
		return {};
	}
	return setting_str.split(","_str, Qt::SkipEmptyParts);
}

void AppSettings::removeList(const QString& key, const QString& value) {
	auto values = valueAsStringList(key);

    const auto itr = std::find(values.begin(), values.end(), value);
	if (itr != values.end()) {
		values.erase(itr);
	}

	QStringList all;
	Q_FOREACH(auto id, values) {
		all << id;
	}
	setValue(key, all.join(","_str));
}

void AppSettings::addList(const QString& key, const QString& value) {
	auto values = valueAsStringList(key);

    const auto itr = std::find(values.begin(), values.end(), value);
	if (itr != values.end()) {
		return;
	}

	values.append(value);
	QStringList all;
	Q_FOREACH(auto id, values) {
		all << id;
	}
	setValue(key, all.join(","_str));
}

QSize AppSettings::valueAsSize(const QString& width_key,
                               const QString& height_key) {
	return QSize{
		valueAsInt(width_key),
		valueAsInt(height_key),
	};
}

QVariant AppSettings::valueAs(const QString& key) {
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	auto v = settings_->value(key);
	return v;
}

int32_t AppSettings::valueAsInt(const QString& key) {
	return valueAs(key).toInt();
}

void AppSettings::loadLanguage() {
	loadLanguage(valueAsString(kAppSettingLang));
}

void AppSettings::loadLanguage(const QString& lang) {
	manager_.loadLanguage(lang);
}

QLocale AppSettings::locale() {
	return manager_.locale();
}

void AppSettings::loadSoxrSetting() {
	XAMP_LOG_DEBUG("LoadSoxrSetting.");

	saveSoxrSetting(kSoxrDefaultSettingName, 96000);

	setValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
	setDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
}

void AppSettings::LoadR8BrainSetting() {
	XAMP_LOG_DEBUG("LoadR8BrainSetting.");

	QMap<QString, QVariant> default_setting;

	default_setting[kResampleSampleRate] = 96000;
	qJsonSettings.setDefaultValue(kR8Brain, QVariant::fromValue(default_setting));
	if (qJsonSettings.valueAsMap(kR8Brain).isEmpty()) {
		qJsonSettings.setValue(kR8Brain, QVariant::fromValue(default_setting));
	}
	if (!contains(kAppSettingResamplerType)) {
		setValue(kAppSettingResamplerType, kR8Brain);
	}
}

void AppSettings::saveLogConfig() {
	QMap<QString, QVariant> log;
	QMap<QString, QVariant> min_level;
	QMap<QString, QVariant> well_known_log_name;
	QMap<QString, QVariant> override_map;

	for (const auto& logger : XampLoggerFactory.GetAllLogger()) {
		if (logger->GetName() != std::string(kXampLoggerName)) {
			well_known_log_name[toQString(logger->GetName())] = log_util::getLogLevelString(logger->GetLevel());
		}
	}

	min_level[kLogDefault] = "debug"_str;

	XampLoggerFactory.SetLevel(log_util::parseLogLevel(min_level[kLogDefault].toString()));

	for (auto itr = well_known_log_name.begin()
	     ; itr != well_known_log_name.end(); ++itr) {
		override_map[itr.key()] = itr.value();
		XampLoggerFactory.GetLogger(itr.key().toStdString())
		                 ->SetLevel(log_util::parseLogLevel(itr.value().toString()));
	}

	min_level[kLogOverride] = override_map;
	log[kLogMinimumLevel] = min_level;
	qJsonSettings.setValue(kLog, QVariant::fromValue(log));
	qJsonSettings.setDefaultValue(kLog, QVariant::fromValue(log));
}

void AppSettings::loadOrSaveLogConfig() {
	XAMP_LOG_DEBUG("LoadOrSaveLogConfig.");

	QMap<QString, QVariant> log;
	QMap<QString, QVariant> min_level;
	QMap<QString, QVariant> override_map;

	QMap<QString, QVariant> well_known_log_name;

	for (const auto& logger : XampLoggerFactory.GetAllLogger()) {
		if (logger->GetName() != std::string(kXampLoggerName)) {
			well_known_log_name[toQString(logger->GetName())] = "info"_str;
		}
	}

	if (qJsonSettings.valueAsMap(kLog).isEmpty()) {
		min_level[kLogDefault] = "info"_str;

		XampLoggerFactory.SetLevel(log_util::parseLogLevel(min_level[kLogDefault].toString()));

		for (auto itr = well_known_log_name.begin()
		     ; itr != well_known_log_name.end(); ++itr) {
			override_map[itr.key()] = itr.value();
			XampLoggerFactory.GetLogger(itr.key().toStdString())
			                 ->SetLevel(log_util::parseLogLevel(itr.value().toString()));
		}

		min_level[kLogOverride] = override_map;
		log[kLogMinimumLevel] = min_level;
		qJsonSettings.setValue(kLog, QVariant::fromValue(log));
		qJsonSettings.setDefaultValue(kLog, QVariant::fromValue(log));
	}
	else {
		log = qJsonSettings.valueAsMap(kLog);
		min_level = log[kLogMinimumLevel].toMap();

		const auto default_level = min_level[kLogDefault].toString();
		XampLoggerFactory.SetLevel(log_util::parseLogLevel(default_level));

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
			XampLoggerFactory.GetLogger(log_name.toStdString())
			                 ->SetLevel(log_util::parseLogLevel(log_level));
		}
	}

#ifdef _DEBUG
	XampLoggerFactory.GetLogger(kCrashHandlerLoggerName)
	                 ->SetLevel(LOG_LEVEL_DEBUG);
#endif
}


void AppSettings::registerMetaType() {
	XAMP_LOG_DEBUG("RegisterMetaType.");
	qRegisterMetaType<int64_t>("int64_t");	
	qRegisterMetaType<DeviceState>("DeviceState");
	qRegisterMetaType<PlayerState>("PlayerState");
	qRegisterMetaType<PlayListEntity>("PlayListEntity");
	qRegisterMetaType<Errors>("Errors");
	qRegisterMetaType<std::vector<float>>("std::vector<float>");
	qRegisterMetaType<QList<PlayListEntity>>("QList<PlayListEntity>");
	qRegisterMetaType<size_t>("size_t");
	qRegisterMetaType<int32_t>("int32_t");
	qRegisterMetaType<ComplexValarray>("ComplexValarray");
	qRegisterMetaType<QList<TrackInfo>>("QList<TrackInfo>");
	qRegisterMetaType<std::vector<TrackInfo>>("std::vector<TrackInfo>");
	qRegisterMetaType<DriveInfo>("DriveInfo");
	qRegisterMetaType<std::wstring>("std::wstring");
	qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
	qRegisterMetaType<DatabaseCoverId>("DatabaseCoverId");
	qRegisterMetaType<AppEQSettings>("AppEQSettings");
}

void AppSettings::loadAppSettings() {
	registerMetaType();

	XAMP_LOG_DEBUG("LoadAppSettings.");

	setDefaultEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
	setDefaultEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);

	setDefaultValue(kAppSettingDeviceType, kEmptyString);
	setDefaultValue(kAppSettingDeviceId, kEmptyString);
	setDefaultValue(kAppSettingVolume, 50);
	setDefaultValue(kLyricsFontSize, 12);
	setDefaultValue(kAppSettingMinimizeToTray, true);
	setDefaultValue(kAppSettingWindowState, false);
	setDefaultValue(kAppSettingScreenNumber, 1);
	setDefaultValue(kAppSettingEnableSpectrum, false);
	setDefaultValue(kAppSettingEnableShortcut, true);
	setDefaultValue(kAppSettingEnterFullScreen, false);
	setDefaultValue(kAppSettingEnableSandboxMode, true);
	setDefaultValue(kAppSettingEnableDebugStackTrace, true);

	setDefaultValue(kAppSettingAlbumPlaylistColumnName, "3, 6, 26"_str);
	setDefaultValue(kAppSettingFileSystemPlaylistColumnName, "3, 6, 26"_str);
	setDefaultValue(kAppSettingCdPlaylistColumnName, "3, 6, 26"_str);
	XAMP_LOG_DEBUG("loadAppSettings success.");
}
