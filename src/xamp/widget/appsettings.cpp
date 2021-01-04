#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>

#include <widget/playerorder.h>
#include <widget/appsettings.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, QList<AppEQSettings>> AppSettings::preset_settings_;

static QMap<QString, QList<AppEQSettings>> loadEQ() {
    QMap<QString, QList<AppEQSettings>> preset_settings;

    auto presetpath = QDir::currentPath() + Q_UTF8("/eqpresets/");
    auto file_ext = QStringList() << Q_UTF8("*.eq");

    for (QDirIterator itr(presetpath, file_ext, QDir::Files | QDir::NoDotAndDotDot); itr.hasNext();) {
        QFile file(itr.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QTextStream in(&file);
        auto line = in.readLine();
        QList<AppEQSettings> bands;
        while (!line.isNull()) {
            auto start = line.indexOf(kGainStr);
            auto end = line.indexOf(kDbStr);
            if (start == -1 || end == -1) {
                break;
            }
            auto gain_str = line.mid(start + kGainStr.size(), end - start - kGainStr.size()).trimmed();
            start = line.indexOf(kQStr);
            auto q_gain_str = line.mid(start + kQStr.size()).trimmed();
            bands.push_back({ gain_str.toFloat(), q_gain_str.toFloat() });
            line = in.readLine();
        }
        preset_settings[itr.fileInfo().baseName()] = bands;
    }
    return preset_settings;
}

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
    loadEQPreset();
}

void AppSettings::loadEQPreset() {
    preset_settings_ = loadEQ();
}

QMap<QString, QList<AppEQSettings>> const & AppSettings::getEQPreset() {
    return preset_settings_;
}

void AppSettings::save() {
	if (!settings_) {
		return;
	}
    settings_->sync();
}

QString AppSettings::getMyMusicFolderPath() {
	auto folder_path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	if (folder_path.isEmpty()) {
		return Qt::EmptyString;
	}
	return folder_path[0];
}

void AppSettings::saveUserEQSettings(QString const &key, QList<AppEQSettings> const & settings) {
    loadEQPreset();
}

void AppSettings::setOrDefaultConfig() {
    loadIniFile(Q_UTF8("xamp.ini"));
    setDefaultValue(kAppSettingDeviceType, Qt::EmptyString);
    setDefaultValue(kAppSettingDeviceId, Qt::EmptyString);
    setDefaultValue(kAppSettingWidth, 600);
    setDefaultValue(kAppSettingHeight, 500);
    setDefaultValue(kAppSettingVolume, 50);
    setDefaultValue(kAppSettingNightMode, false);
    setDefaultValue(kAppSettingOrder, static_cast<int32_t>(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE));
    setDefaultValue(kAppSettingBackgroundColor, QColor("#01121212"));
    setDefaultValue(kAppSettingEnableBlur, true);
	setDefaultValue(kAppSettingPreventSleep, true);
    setDefaultValue(kLyricsFontSize, 12);
    setDefaultValue(kAppSettingMinimizeToTrayAsk, true);
    setDefaultValue(kAppSettingMinimizeToTray, false);
}

Uuid AppSettings::getID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
		return Uuid::INVALID_ID;
	}
	return Uuid::FromString(str.toStdString());
}

QSize AppSettings::getSizeValue(const QString& width_key,
	const QString& height_key) {
	return QSize{
		getAsInt(width_key),
		getAsInt(height_key),
	};
}

QVariant AppSettings::getValue(const QString& key) {
	if (!settings_->contains(key)) {
		return default_settings_.value(key);
	}
	return settings_->value(key);
}

int32_t AppSettings::getAsInt(const QString& key) {
	return getValue(key).toInt();
}

void AppSettings::loadLanguage(const QString& lang) {
	manager_.loadLanguage(lang);
}
