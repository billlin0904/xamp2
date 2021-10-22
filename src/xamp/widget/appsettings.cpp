#include <QStandardPaths>
#include <QDirIterator>
#include <QTextStream>
#include <QSize>

#include <widget/framelesswindow.h>
#include <widget/playerorder.h>
#include <widget/appsettings.h>

QScopedPointer<QSettings> AppSettings::settings_;
QMap<QString, QVariant> AppSettings::default_settings_;
LocaleLanguageManager AppSettings::manager_;

void AppSettings::loadIniFile(const QString& file_name) {
	settings_.reset(new QSettings(file_name, QSettings::IniFormat));    
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

Uuid AppSettings::getID(const QString& key) {
	auto str = getValue(key).toString();
	if (str.isEmpty()) {
        return Uuid::kInvalidUUID;
	}
	return Uuid::FromString(str.toStdString());
}

QList<QString> AppSettings::getList(QString const& key) {
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 1)
    return AppSettings::getValueAsString(key).split(Q_UTF8(","), Qt::SkipEmptyParts);
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
    QString all;
    Q_FOREACH(auto id, values) {
        all += id + Q_UTF8(",");
    }
    AppSettings::setValue(key, all);
}

void AppSettings::addList(QString const& key, QString const & value) {
    auto values = getList(key);
    Q_FOREACH(auto id, values) {
        if (id == value) {
            return;
        }
    }
    values.append(value);
    QString all;
    Q_FOREACH(auto id, values) {
        all += id + Q_UTF8(",");
    }
    AppSettings::setValue(key, all);
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
