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
DirectoryWatcher AppSettings::file_watcher_;

void AppSettings::startMonitorFile(FramelessWindow *window) {
	/*(void)QObject::connect(&file_watcher_,
		&DirectoryWatcher::fileChanged,
		window, 
		&FramelessWindow::onFileChanged,
		Qt::QueuedConnection);
	file_watcher_.start();
	file_watcher_.addPath(getMyMusicFolderPath());*/
}

void AppSettings::shutdownMonitorFile() {
	//file_watcher_.shutdown();
	//file_watcher_.wait();
}

void AppSettings::addMonitorPath(QString const& file_name) {
	//file_watcher_.addPath(file_name);
}

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
