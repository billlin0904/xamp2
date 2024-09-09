#include <widget/audio_embedding/audio_embedding_service.h>
#include <widget/util/json_util.h>

namespace {
    constexpr auto BASE_URL = qTEXT("http://127.0.0.1:8000");
}

AudioEmbeddingService::AudioEmbeddingService(QObject* parent)
    : QObject(parent) {
}

AudioEmbeddingService::~AudioEmbeddingService() = default;

void AudioEmbeddingService::embedAndSave(const QString& path, int32_t audio_id) {
    QVariantMap content;
    content[qTEXT("file_path")] = path;
    content[qTEXT("audio_id")] = qFormat("%1").arg(audio_id);
    const auto json = json_util::serialize(content);
    http::HttpClient(qFormat("%1/embed_and_save").arg(BASE_URL))
        .json(json)
        .post();
}

void AudioEmbeddingService::queryEmbeddings(const QList<QString> &paths) {
    QMultiMap<QString, QVariant> params;
    Q_FOREACH(auto path, paths) {
        params.insert(qTEXT("paths"), path);
    }

    http::HttpClient(qFormat("%1/query_embeddings").arg(BASE_URL))
        .params(params)
		.addAccpetJsonHeader()
		.success([this](auto url, auto content) {
        QJsonDocument doc;
		if (!json_util::deserialize(content, doc)) {
			return;
		}

		QList<EmbeddingQueryResult> results;
        const auto json_array = doc.array();
        for (const auto& value : json_array) {
            if (value.isObject()) {
                auto obj = value.toObject();
                auto file_path = obj[qTEXT("file_path")].toString();
                auto distance = obj[qTEXT("distances")].toDouble();
                auto audio_id = obj[qTEXT("audio_id")].toString();
                results.append(EmbeddingQueryResult{ file_path, distance, audio_id });
				XAMP_LOG_DEBUG("{} {}", file_path.toStdString(), distance);
            }
        }
        emit queryEmbeddingsReady(results);
		})
        .get();
}

void AudioEmbeddingService::flush() {
    http::HttpClient(qFormat("%1/flushdb").arg(BASE_URL))
        .del();
}
