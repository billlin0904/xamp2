#include <widget/networkdiskcache.h>
#include <widget/appsettings.h>

#include <QDir>

namespace {
    constexpr qint64 kNetworkCacheSize = 256LL * 1024LL * 1024LL;
}

NetworkDiskCache::NetworkDiskCache(QObject* parent)
    : QNetworkDiskCache(parent) {
    const auto cache_dir = QDir(qAppSettings.getOrCreateCachePath()).filePath(QStringLiteral("network"));
    setCacheDirectory(cache_dir);
    setMaximumCacheSize(kNetworkCacheSize);
}
