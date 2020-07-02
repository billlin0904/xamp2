#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>

#include <widget/playerorder.h>
#include <widget/appsettings.h>

static QLatin1String const kGainStr("Gain");
static QLatin1String const kDbStr("dB");
static QLatin1String const kQStr("Q");

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, QList<FilterBand>> AppSettings::EQBands;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));

    auto presetpath = QDir::currentPath() + Q_UTF8("/eqpresets/");
    auto file_ext = QStringList() << Q_UTF8("*.eq");

    for (QDirIterator itr(presetpath, file_ext, QDir::Files | QDir::NoDotAndDotDot);
         itr.hasNext();) {
        QFile file(itr.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        QTextStream in(&file);
        QString line = in.readLine();
        QList<FilterBand> bands;
        while (!line.isNull()) {
            auto start = line.indexOf(kGainStr);
            auto end = line.indexOf(kDbStr);
            auto gainStr = line.mid(start + kGainStr.size(), end - start - kGainStr.size()).trimmed();
            start = line.indexOf(kQStr);
            auto QGainStr = line.mid(start + kQStr.size()).trimmed();
            bands.push_back({gainStr.toFloat(), QGainStr.toFloat()});
            line = in.readLine();
        }
        EQBands[itr.fileInfo().baseName()] = bands;
    }
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

void AppSettings::setOrDefaultConfig() {
    loadIniFile(Q_UTF8("xamp.ini"));
    setDefaultValue(kAppSettingDeviceType, QEmptyString);
    setDefaultValue(kAppSettingDeviceId, QEmptyString);
    setDefaultValue(kAppSettingWidth, 600);
    setDefaultValue(kAppSettingHeight, 500);
    setDefaultValue(kAppSettingVolume, 50);
    setDefaultValue(kAppSettingNightMode, false);
    setDefaultValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    setDefaultValue(kAppSettingBackgroundColor, QColor("#01121212"));
    setDefaultValue(kAppSettingEnableBlur, true);
	setDefaultValue(kAppSettingPreventSleep, true);
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
