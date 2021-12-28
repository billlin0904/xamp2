#include <base/logger.h>
#include <QJsonDocument>
#include <widget/str_utilts.h>
#include <widget/ytmusicobserver.h>

YtMusicObserver::YtMusicObserver(QObject* parent)
	: QObject(parent) {
}

void YtMusicObserver::postMessage(const QString& json) {
    XAMP_LOG_DEBUG(json.toStdString());

    QJsonParseError error;
    auto dict = QJsonDocument::fromJson(json.toUtf8(), &error).toVariant().toMap();
    if (error.error != QJsonParseError::NoError){
        return;
    }

    YtMusicMediaEntity entity;
    entity.title = dict[Q_UTF8("title")].toString();
    entity.by = dict[Q_UTF8("by")].toString();
    entity.thumbnail = dict[Q_UTF8("thumbnail")].toString();
    entity.length = dict[Q_UTF8("length")].toInt();
    entity.progress = dict[Q_UTF8("progress")].toInt();
    entity.isPlaying = dict[Q_UTF8("isPlaying")].toBool();

    updateMediaEntity(entity);
}
