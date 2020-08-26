#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>

#include <stream/equalizer.h>
#include <widget/playerorder.h>
#include <widget/appsettings.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, QList<AppEQSettings>> AppSettings::preset_settings_;

static void saveEQ(QString const& key, QList<AppEQSettings> const& settings) {
    const auto presetpath = QDir::currentPath() + Q_UTF8("/eqpresets/");

    QFile file(presetpath + key + Q_UTF8(".eq"));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    size_t i = 0;
    
    QTextStream stream(&file);
    for (auto setting : settings) {
        stream << "Filter " << i
            << ":ON PK Fc"
            << xamp::stream::kEQBands[i] << " Hz "
            << "Gain " << setting.gain << " dB "
            << "Q " << setting.Q << "\r\n";
        ++i;
    }

    file.close();
}

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
        QString line = in.readLine();
        QList<AppEQSettings> bands;
        while (!line.isNull()) {
            auto start = line.indexOf(kGainStr);
            auto end = line.indexOf(kDbStr);
            if (start == -1 || end == -1) {
                break;
            }
            auto gainStr = line.mid(start + kGainStr.size(), end - start - kGainStr.size()).trimmed();
            start = line.indexOf(kQStr);
            auto QGainStr = line.mid(start + kQStr.size()).trimmed();
            bands.push_back({ gainStr.toFloat(), QGainStr.toFloat() });
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
	return QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0];
}

void AppSettings::saveUserEQSettings(QString const &key, QList<AppEQSettings> const & settings) {
    saveEQ(key, settings);
    loadEQPreset();
}

void AppSettings::setOrDefaultConfig() {
    loadIniFile(Q_UTF8("xamp.ini"));
    setDefaultValue(kAppSettingDeviceType, Qt::EmptyStr);
    setDefaultValue(kAppSettingDeviceId, Qt::EmptyStr);
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

xamp::base::ID AppSettings::getID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
		return xamp::base::ID::INVALID_ID;
	}
	return xamp::base::ID::FromString(str.toStdString());
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
