#include <QFileInfo>
#include <QDir>
#include <QSet>

#include <metadata/taglibmetareader.h>
#include <widget/str_utilts.h>
#include <widget/filesystemwatcher.h>

FileSystemWatcher::FileSystemWatcher(QObject* parent)
	: QObject(parent) {
	(void) QObject::connect(&watcher_, SIGNAL(fileChanged(const QString&)), 
		this, 
		SLOT(onFileChanged(const QString&)));
	(void) QObject::connect(&watcher_, SIGNAL(directoryChanged(const QString&)),
		this, 
		SLOT(onDirectoryChanged(const QString&)));
}

FileSystemWatcher::~FileSystemWatcher() {
	adapter_.Cancel();
}

void FileSystemWatcher::addPath(const QString& path) {
	watcher_.addPath(path);
	QFileInfo file(path);
	if (file.isDir()) {
		const QDir dirw(path);
		file_map_[path] = dirw.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
	}
}

void FileSystemWatcher::onFileChanged(const QString& file) {	
	const xamp::metadata::Path path(file.toStdWString());
	xamp::metadata::TaglibMetadataReader reader;
	xamp::metadata::FromPath(path, &adapter_, &reader);

	for (const auto& result : adapter_.results) {
		MetadataExtractAdapter::processMetadata(result);
	}
	adapter_.results.clear();
}

void FileSystemWatcher::onDirectoryChanged(const QString& path) {
	auto entry_list = file_map_[path];

	const QDir dir(path);
	auto new_entry_list = dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::DirsFirst);

	auto new_dir_set = QSet<QString>::fromList(new_entry_list);
	auto current_dir_set = QSet<QString>::fromList(entry_list);

	auto new_files = new_dir_set - current_dir_set;
	auto deleted_files = current_dir_set - new_dir_set;

	file_map_[path] = new_entry_list;
	auto new_file = new_files.toList();

	if (!new_files.isEmpty() && !deleted_files.isEmpty()) {

	}
	else {
		if (!new_files.isEmpty()) {
			foreach (QString file, new_files) {
				onFileChanged(path + Q_UTF8("/") + file);
			}
		}

		if (!deleted_files.isEmpty()) {
			foreach(QString file, deleted_files) {
			}
		}
	}
}