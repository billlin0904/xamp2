//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <string>

#include <widget/http.h>

#include <widget/widget_shared_global.h>
#include <widget/util/str_utilts.h>

struct CollectionModel {
	QString id;
	QString name;
};

Q_DECLARE_METATYPE(CollectionModel)

struct QueryResultModel {
	QList<QList<QString>> ids;
	QList<QList<QList<float>>> embeddings;
	QList<QList<QMap<QString, QVariant>>> metadatas;
	QList<QList<double>> distances;
};

Q_DECLARE_METATYPE(QueryResultModel)

struct VectorSettings {
	int32_t size;
	QString distance;
};

struct UpsertEmbeddings {
	QList<QString> ids;
	QList<float> embeddings;
	std::optional<QList<QVariant>> metadatas;
};

class XAMP_WIDGET_SHARED_EXPORT ChromaClient : public QObject {
	Q_OBJECT
public:
	ChromaClient(QObject* parent, QNetworkAccessManager* nam, std::shared_ptr<ObjectPool<QByteArray>> buffer_pool);

	void setUrl(const QString& url);

	void createCollection(const QString& collection_name, const std::optional<VectorSettings>& settings = std::nullopt);

	void getCollection(const QString& collection_id);

	void deleteCollection(const QString& collection_id);

	void listCollections();

	void upsertEmbeddings(const QString& collection_id, const UpsertEmbeddings& upsert_embedding);

	void queryEmbeddings(const QString& collection_id, const QList<float>& embeddings, int n_results, QStringList include = QStringList());

signals:
	//void queryEmbeddingsResponse(const QueryResultModel &model);

private:
	QString getUrl();

	QNetworkAccessManager* nam_;
	std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
	QString url_;
};
