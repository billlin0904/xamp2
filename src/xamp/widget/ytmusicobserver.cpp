#include <base/logger.h>
#include <widget/ytmusicobserver.h>

YtMusicObserver::YtMusicObserver(QObject* parent)
	: QObject(parent) {
}

void YtMusicObserver::postMessage(const QString& json) {
	XAMP_LOG_DEBUG(json.toStdString());
}