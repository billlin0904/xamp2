//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMutex>
#include <QObject>

#include <base/metadata.h>

#include <metadata/metadatareader.h>
#include <metadata/metadataextractadapter.h>
#include <metadata/taglibmetareader.h>

#include <widget/pixmapcache.h>

using MetadataExtractAdapterBase = xamp::metadata::MetadataExtractAdapter;
using xamp::metadata::Metadata;
using xamp::metadata::Path;
using xamp::base::LruCache;

class IdCache {
public:
    IdCache() = default;

    void clear() {
        album_id_cache.Clear();
        artist_id_cache.Clear();
        cover_id_cache.Clear();
    }

    std::tuple<int32_t, int32_t, QString> cache(const QString& album, const QString& artist);

    LruCache<int32_t, QString> cover_id_cache;
private:    
    LruCache<QString, int32_t> album_id_cache;
    LruCache<QString, int32_t> artist_id_cache;
};

class PlayListTableView;

class MetadataExtractAdapter
	: public QObject
    , public MetadataExtractAdapterBase {
	Q_OBJECT
public:
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

	~MetadataExtractAdapter() override;

    void OnWalk(const xamp::metadata::Path& path, Metadata metadata) override;

	void OnWalkNext() override;

    bool IsCancel() const override;

	void Cancel() override;

    void Reset() override;

signals:
	void readCompleted(std::vector<xamp::base::Metadata> enitiy);

public:
    void processMetadata(const std::vector<Metadata>& metadatas, PlayListTableView *playlist = nullptr);

    void readMetadata(MetadataExtractAdapter* adapter, QString const& file_name);
	    
private:
    bool cancel_;
    std::vector<Metadata> metadatas_;
    QMutex mutex_;
    IdCache cache_;
};

