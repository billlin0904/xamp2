#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>
#include <base/logger_impl.h>
#include <widget/appsettingnames.h>
#include <widget/xwindow.h>
#include <widget/playerorder.h>
#include <widget/appsettings.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;
QMap<QString, EQSettings> AppSettings::eq_settings_;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));
    loadEQPreset();
}

const QMap<QString, EQSettings>& AppSettings::getEQPreset() {
    return eq_settings_;
}

AppEQSettings AppSettings::getEQSettings() {
    return QVariant::fromValue(getValue(kAppSettingEQName)).value<AppEQSettings>();
}

void AppSettings::loadEQPreset() {
    auto path = QDir::currentPath() + Q_TEXT("/eqpresets/");
    auto file_ext = QStringList() << Q_TEXT("*.*");

    for (QDirIterator itr(path, file_ext, QDir::Files | QDir::NoDotAndDotDot);
        itr.hasNext();) {
        const auto filepath = itr.next();
        const QFileInfo file_info(filepath);
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            EQSettings settings;
            int i = 0;
            while (!in.atEnd()) {
                auto line = in.readLine();
                auto result = line.split(Q_TEXT(":"));
                auto str = result[1].toStdWString();
                if (result[0] == Q_TEXT("Preamp")) {
                    swscanf(str.c_str(), L"%f dB",
                        &settings.preamp);
                }
                else if (result[0].indexOf(Q_TEXT("Filter") != -1)) {
                    auto pos = str.find(L"Gain");
                    if (pos == std::wstring::npos) {
                        continue;
                    }
                    swscanf(&str[pos], L"Gain %f dB Q %f",
                        &settings.bands[i].gain, &settings.bands[i].Q);
                    ++i;
                    XAMP_LOG_DEBUG("Pase {}", line.toStdString());
                }
            }
            eq_settings_[file_info.baseName()] = settings;
        }
    }
}

void AppSettings::save() {
	if (!settings_) {
		return;
	}
    settings_->sync();
}

QString AppSettings::getCachePath() {
    auto folder_path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    return folder_path[0];
}

QString AppSettings::getMyMusicFolderPath() {
    if (!contains(kAppSettingMyMusicFolderPath)) {
        auto folder_path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
        if (folder_path.isEmpty()) {
            return Qt::EmptyString;
        }
        return folder_path[0];
    }
    return getValueAsString(kAppSettingMyMusicFolderPath);
}

Uuid AppSettings::getValueAsID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
        return Uuid::kNullUuid;
	}
	return Uuid::FromString(str.toStdString());
}

QList<QString> AppSettings::getList(QString const& key) {
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 1)
    return AppSettings::getValueAsString(key).split(Q_TEXT(","), Qt::SkipEmptyParts);
#else
    return AppSettings::getValueAsString(key).split(Q_UTF8(","), QString::SkipEmptyParts);
#endif
}

void AppSettings::removeList(QString const& key, QString const & value) {
    auto values = getList(key);

    auto itr = std::find(values.begin(), values.end(), value);
    if (itr != values.end()) {
        values.erase(itr);
    }

    QStringList all;
    Q_FOREACH(auto id, values) {
        all << id;
    }
    AppSettings::setValue(key, all.join(Q_TEXT(",")));
}

void AppSettings::addList(QString const& key, QString const & value) {
    auto values = getList(key);

    auto itr = std::find(values.begin(), values.end(), value);
    if (itr != values.end()) {
        return;
    }

    values.append(value);
    QStringList all;
    Q_FOREACH(auto id, values) {
        all << id;
    }
    AppSettings::setValue(key, all.join(Q_TEXT(",")));
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
    auto v = settings_->value(key);
    return v;
}

int32_t AppSettings::getAsInt(const QString& key) {
	return getValue(key).toInt();
}

void AppSettings::loadLanguage(const QString& lang) {
	manager_.loadLanguage(lang);
}
