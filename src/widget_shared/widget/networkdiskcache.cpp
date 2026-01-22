#include <widget/networkdiskcache.h>
#include <widget/appsettings.h>

#include <QMutexLocker>
#include <QNetworkDiskCache>

QMutex NetworkDiskCache::mutex_;
QNetworkDiskCache* NetworkDiskCache::thumbnail_cache_ = nullptr;

NetworkDiskCache::NetworkDiskCache(QObject* parent)
	: QAbstractNetworkCache(parent) {
	QMutexLocker l(&mutex_);
	if (!thumbnail_cache_) {
		thumbnail_cache_ = new QNetworkDiskCache(parent);
		thumbnail_cache_->setCacheDirectory(qAppSettings.getOrCreateCachePath());
		thumbnail_cache_->setMaximumCacheSize(5 * 1024 * 1024);
	}	
}

qint64 NetworkDiskCache::cacheSize() const {
	QMutexLocker l(&mutex_);
	return thumbnail_cache_->cacheSize();
}

QIODevice* NetworkDiskCache::data(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return thumbnail_cache_->data(url);
}

void NetworkDiskCache::insert(QIODevice* device) {
	QMutexLocker l(&mutex_);
	thumbnail_cache_->insert(device);
}

QNetworkCacheMetaData NetworkDiskCache::metaData(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return thumbnail_cache_->metaData(url);
}

QIODevice* NetworkDiskCache::prepare(
	const QNetworkCacheMetaData& metaData) {
	QMutexLocker l(&mutex_);
	return thumbnail_cache_->prepare(metaData);
}

bool NetworkDiskCache::remove(const QUrl& url) {
	QMutexLocker l(&mutex_);
	return thumbnail_cache_->remove(url);
}

void NetworkDiskCache::updateMetaData(
	const QNetworkCacheMetaData& metaData) {
	QMutexLocker l(&mutex_);
	thumbnail_cache_->updateMetaData(metaData);
}

void NetworkDiskCache::clear() {
	QMutexLocker l(&mutex_);
	thumbnail_cache_->clear();
}
