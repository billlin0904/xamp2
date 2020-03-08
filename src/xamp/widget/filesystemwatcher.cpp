#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QtConcurrent>
#include <QFutureWatcher>

#include <metadata/taglibmetareader.h>
#include <widget/str_utilts.h>
#include <widget/filesystemwatcher.h>

static void readMetadata(MetadataExtractAdapter* adapter, const QString& file_name) {
	auto extract_handler = [adapter](const auto& file_name) {
		const xamp::metadata::Path path(file_name.toStdWString());
		xamp::metadata::TaglibMetadataReader reader;
		xamp::metadata::FromPath(path, adapter, &reader);
	};

	auto future = QtConcurrent::run(extract_handler, file_name);
	auto watcher = new QFutureWatcher<void>();
	(void)QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
		watcher->deleteLater();
		adapter->deleteLater();
		});

	watcher->setFuture(future);
}

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

void FileSystemWatcher::onFileChanged(const QString& file_name) {
	auto adapter = new MetadataExtractAdapter();
	(void)QObject::connect(adapter, &MetadataExtractAdapter::readCompleted, this, &FileSystemWatcher::processMeatadata);
	readMetadata(adapter, file_name);
}

void FileSystemWatcher::processMeatadata(const std::vector<xamp::base::Metadata>& medata) {
	MetadataExtractAdapter::processMetadata(medata);
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