// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <filesystem>
#include <fstream>

#include <base/singleton.h>
#include <base/lrucache.h>
#include <stream/wavefilewriter.h>

namespace xamp::stream {

using xamp::base::LruCache;

class XAMP_STREAM_API PodcastFileCache
    : public std::enable_shared_from_this<PodcastFileCache> {
public:
    explicit PodcastFileCache(std::string const& cache_id);

    void SetTempPath(std::string const& file_ext, Path const& path);

    virtual ~PodcastFileCache();

    [[nodiscard]] bool IsOpen() const;

    void Write(void const* buf, size_t buf_size);

    void Close();

    [[nodiscard]] Path GetFilePath() const;

    [[nodiscard]] bool IsCompleted() const noexcept;

    [[nodiscard]] std::string GetCacheID() const;
private:
    bool is_completed_;
    Path path_;
    std::string cache_id_;
    std::ofstream file_;
};

std::string ToCacheID(Path const& file_path);

class XAMP_STREAM_API PodcastFileCacheManager {
public:
	friend class Singleton<PodcastFileCacheManager>;

    XAMP_DISABLE_COPY(PodcastFileCacheManager)

    std::shared_ptr<PodcastFileCache> GetOrAdd(std::string const& cache_id);

    void Remove(std::string const& cache_id);

    void Remove(std::shared_ptr<PodcastFileCache> const & file_cache);

    void SetTempPath(Path const& path);

    static PodcastFileCacheManager& GetInstance();

private:
	PodcastFileCacheManager();

    Path path_;
	LruCache<std::string, std::shared_ptr<PodcastFileCache>> cache_;
};

}


#define PodcastCache xamp::stream::PodcastFileCacheManager::GetInstance()
