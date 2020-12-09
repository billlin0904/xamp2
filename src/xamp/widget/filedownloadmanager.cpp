#include <widget/http.h>
#include <widget/filedownloadmanager.h>

FileDownloadManager::FileDownloadManager(QObject* parent)
	: QObject(parent) {
    (void) QObject::connect(&timer_, &QTimer::timeout, [this](){
            download();
        });
    timer_.setInterval(1000);
    timer_.start();
}

void FileDownloadManager::addQueue(int32_t music_id, const QUrl& url, const QString& file_name) {
    entries_.push_back(DownloadEntry{ music_id, url, file_name });
}

void FileDownloadManager::download() {
    if (entries_.isEmpty()) return;

    auto entry = entries_.front();
    entries_.pop_front();

    auto download_handler = [entry, this](const auto& file_path) {
        emit downloadComplete(entry.music_id, file_path);        
        stateChange(entry.music_id);
    };

    auto error_handler = [entry, this](auto error_message) {
        stateChange(entry.music_id);
        entries_.push_back(entry);
    };

    http::HttpClient(entry.url).downloadFile(
        entry.file_name,
        download_handler,
        error_handler);
}