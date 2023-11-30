#include <widget/networkdiskcache.h>
#include <QMutexLocker>
#include <QNetworkDiskCache>

QMutex NetworkDiskCache::mutex_;
QNetworkDiskCache* NetworkDiskCache::cache_ = nullptr;

NetworkDiskCache::NetworkDiskCache(QObject* parent)
	: QAbstractNetworkCache(parent) {
	QMutexLocker l(&mutex_);
	if (!cache_) {
		cache_ = new QNetworkDiskCache();
	}	
}

qint64 NetworkDiskCache::cacheSize() const {
	QMutexLocker l(&mutex_);
	return cache_->cacheSize();
}

QIODevice* NetworkDiskCache::data(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return cache_->data(url);
}

void NetworkDiskCache::insert(QIODevice* device) {
	QMutexLocker l(&mutex_);
	cache_->insert(device);
}

QNetworkCacheMetaData NetworkDiskCache::metaData(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return cache_->metaData(url);
}

QIODevice* NetworkDiskCache::prepare(
	const QNetworkCacheMetaData& metaData) {
	QMutexLocker l(&mutex_);
	return cache_->prepare(metaData);
}

bool NetworkDiskCache::remove(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return cache_->remove(url);
}

void NetworkDiskCache::updateMetaData(
	const QNetworkCacheMetaData& metaData) {
	QMutexLocker l(&mutex_);
	cache_->updateMetaData(metaData);
}

void NetworkDiskCache::clear() {
	QMutexLocker l(&mutex_);
	cache_->clear();
}
