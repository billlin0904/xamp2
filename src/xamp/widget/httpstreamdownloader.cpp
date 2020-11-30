#include <QTemporaryFile>

#include <widget/http.h>
#include <widget/httpstreamdownloader.h>

HttpStreamDownloader::HttpStreamDownloader(QObject* parent)
	: QObject(parent) {
}

void HttpStreamDownloader::addQueue(int32_t music_id, const QUrl& url, const QString& file_name) {
    entries_.push_back(DownloadEntry{ music_id, url, file_name });
}

void HttpStreamDownloader::start() {
    if (entries_.isEmpty()) return;

    auto entry = entries_.front();
    entries_.pop_front();

    auto downloadHandler = [entry, this](const auto& file_path) {
        emit downloadComplete(entry.music_id, file_path);        
        stateChange(entry.music_id);
        start();
    };

    auto errorHandler = [entry, this](auto errorMessage) {
        stateChange(entry.music_id);
        entries_.push_back(entry);
        start();
    };

    http::HttpClient(entry.url.toString()).downloadFile(entry.file_name, downloadHandler, errorHandler);
}