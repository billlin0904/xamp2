#include <widget/audio_embedding/audio_embedding_service.h>
#include <widget/util/json_util.h>
#include <widget/httpx.h>

namespace {
    constexpr auto BASE_URL = "http://127.0.0.1:9090"_str;
}

AudioEmbeddingService::AudioEmbeddingService(QObject* parent)
    : QObject(parent)
	, http_client_(BASE_URL) {
}

AudioEmbeddingService::~AudioEmbeddingService() = default;

QCoro::Task<QString> AudioEmbeddingService::embedAndSave(const QString& path, int32_t audio_id) {
    /*QVariantMap content;
    content["file_path"_str] = path;
    content["audio_id"_str] = qFormat("%1").arg(audio_id);
    const auto json = json_util::serialize(content);
    http_client_.setUrl(qFormat("%1/embed_and_save").arg(BASE_URL));
    return http_client_
        .setJson(json)
        .post();*/
    QMultiMap<QString, QVariant> params;
    params.insert("file_path"_str, path);
    http_client_.setUrl(qFormat("%1/transcribe/").arg(BASE_URL));
    auto content = co_await http_client_
        .params(params)
        .addAcceptJsonHeader()
        .get();
    QJsonDocument json_doc;
    if (!json_util::deserialize(content, json_doc)) {
        co_return QString();
    }
    auto lrc_content = json_doc.object()["lrc_content"_str].toString();
	co_return lrc_content;
}

QCoro::Task<QList<EmbeddingQueryResult>> AudioEmbeddingService::queryEmbeddings(const QList<QString> &paths) {
    QMultiMap<QString, QVariant> params;
    Q_FOREACH(auto path, paths) {
        params.insert("paths"_str, path);
    }

    http_client_.setUrl(qFormat("%1/query_embeddings").arg(BASE_URL));
    auto content = co_await http_client_
        .params(params)
        .addAcceptJsonHeader()
        .get();

    QList<EmbeddingQueryResult> results;
    QJsonDocument doc;
    if (!json_util::deserialize(content, doc)) {
        co_return results;
    }
    
    const auto json_array = doc.array();
    for (const auto& value : json_array) {
        if (value.isObject()) {
            auto obj = value.toObject();
            auto file_path = obj["file_path"_str].toString();
            auto distance = obj["distances"_str].toDouble();
            auto audio_id = obj["audio_id"_str].toString();
            results.append(EmbeddingQueryResult{ file_path, distance, audio_id });
            XAMP_LOG_DEBUG("{} {}", file_path.toStdString(), distance);
        }
    }
    co_return results;
}

QCoro::Task<QString> AudioEmbeddingService::flush() {
    http_client_.setUrl(qFormat("%1/flushdb").arg(BASE_URL));
    return http_client_.del();
}

QCoro::Task<QString> AudioEmbeddingService::deleteEmbeddings(const QList<QString>& audio_ids) {
    QMultiMap<QString, QVariant> params;
    Q_FOREACH(auto path, audio_ids) {
        params.insert("audio_ids"_str, path);
    }

    http_client_.setUrl(qFormat("%1/delete_embedding").arg(BASE_URL));
    auto content = co_await http_client_
        .params(params)
        .addAcceptJsonHeader()
        .del();
    co_return content;
}
