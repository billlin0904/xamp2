//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <string>

#include <widget/util/str_utilts.h>

namespace service {

namespace chroma {
	struct CollectionModel {
		QString id;
		QString name;
	};

	class ChromaClient {
	public:
		ChromaClient();

		void createCollection(const QString& collection_name);

		CollectionModel getCollection(const QString& collection_name);

		void deleteCollection(const QString& collection_name);

		std::vector<QString> listCollections();

		void upsertEmbeddings(const QString& collection_id,
			const QList<QString>& ids,
			const QList<float>& embeddings,
			const QList<QVariant>& objects);
	};
}

}


