#include <widget/util/json_util.h>
#include <widget/chroma_client.h>

ChromaClient::ChromaClient(QObject* parent, QNetworkAccessManager* nam, std::shared_ptr<ObjectPool<QByteArray>> buffer_pool)
	: QObject(parent)
	, nam_(nam)
	, buffer_pool_(buffer_pool) {
}

void ChromaClient::setUrl(const QString& url) {
	url_ = url;
}

void ChromaClient::createCollection(const QString& collection_name, const std::optional<VectorSettings>& settings) {
	http::HttpClient client(nam_, buffer_pool_, getUrl() + qSTR("collections/%0?wait=true").arg(collection_name));

	if (settings) {
		QJsonObject request;
		request[qTEXT("size")]     = settings.value().size;
		request[qTEXT("distance")] = settings.value().distance;
		client.json(json_util::serialize(request));
	}

	client.post();
}

void ChromaClient::getCollection(const QString& collection_id) {
	http::HttpClient(nam_, buffer_pool_, getUrl() + qSTR("collections/%0").arg(collection_id))
		.get();
}

void ChromaClient::deleteCollection(const QString& collection_id) {
	http::HttpClient(nam_, buffer_pool_, getUrl() + qSTR("collections/%0?timeout=30").arg(collection_id))
		.del();
}

void ChromaClient::listCollections() {
	http::HttpClient(nam_, buffer_pool_, getUrl() + qSTR("collections/"))
		.get();
}

void ChromaClient::upsertEmbeddings(const QString& collection_id, const UpsertEmbeddings& upsert_embedding) {
	QJsonObject request;
	request[qTEXT("ids")] = QJsonValue::fromVariant(upsert_embedding.ids);

	QJsonArray embeddings;
	for (auto v : upsert_embedding.embeddings)
		embeddings.append(QJsonValue(v));
	request[qTEXT("embeddings")] = embeddings;

	if (upsert_embedding.metadatas) {
		QJsonArray metadatas;
		for (auto v : upsert_embedding.metadatas.value())
			embeddings.append(QJsonValue::fromVariant(v));
		request[qTEXT("metadatas")] = metadatas;
	}

	http::HttpClient(nam_, buffer_pool_, getUrl() + qSTR("collections/%0/upsert").arg(collection_id))
		.json(json_util::serialize(request))
		.post();
}

void ChromaClient::queryEmbeddings(const QString& collection_id, const QList<float>& embeddings, int n_results, QStringList include) {
	QJsonObject request;

	QJsonArray query_embeddings;
	for (auto v : embeddings)
		query_embeddings.append(QJsonValue::fromVariant(v));
	request[qTEXT("query_embeddings")] = query_embeddings;

	request[qTEXT("n_results")] = n_results;

	if (!include.isEmpty()) {
		QJsonArray includes;
		for (auto v : include)
			includes.append(v);
		request[qTEXT("include")] = includes;
	}

	http::HttpClient(nam_, buffer_pool_, getUrl() + qSTR("collections/%0/query").arg(collection_id))
		.json(json_util::serialize(request))
		.success([this](const auto& url, const auto& content) {
			QVariantMap result;
		    json_util::deserialize(content, result);
			QueryResultModel model;
			//emit queryEmbeddingsResponse(model);
			})
		.post();
}

QString ChromaClient::getUrl() {
	if (!url_.endsWith(qTEXT("/"))) {
		url_.append(qTEXT("/"));
	}
	return url_;
}
