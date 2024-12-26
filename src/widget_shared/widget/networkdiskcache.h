//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMutex>
#include <QAbstractNetworkCache>

class QNetworkDiskCache;

class NetworkDiskCache : public QAbstractNetworkCache {
public:
	explicit NetworkDiskCache(QObject* parent = nullptr);

	qint64 cacheSize() const override;

	QIODevice* data(const QUrl& url) override;

	void insert(QIODevice* device) override;

	QNetworkCacheMetaData metaData(const QUrl& url) override;

	QIODevice* prepare(const QNetworkCacheMetaData& metaData) override;

	bool remove(const QUrl& url) override;

	void updateMetaData(const QNetworkCacheMetaData& metaData) override;

	void clear() override;

private:
	static QMutex mutex_;
	static QNetworkDiskCache* cache_;
};

