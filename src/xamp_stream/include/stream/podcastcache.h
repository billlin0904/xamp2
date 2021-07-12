// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <filesystem>
#include <memory.h>
#include <fstream>

#include <base/singleton.h>
#include <base/lrucache.h>
#include <stream/wavefilewriter.h>

namespace xamp::stream {

using xamp::base::LruCache;
using xamp::base::AudioFormat;
using xamp::stream::WaveFileWriter;

class XAMP_STREAM_API PadcastFileCache : public std::enable_shared_from_this<PadcastFileCache> {
public:
	explicit PadcastFileCache(std::string const& cache_id)
		: is_completed_(false)
		, cache_id_(cache_id) {
		auto file_name = std::tmpnam(nullptr) + std::string(".m4a");
		auto tmp_dir_path =	Fs::temp_directory_path();
		tmp_dir_path /= file_name;
		path_ = tmp_dir_path;
		file_.open(path_, std::ios::binary);
	}

	virtual ~PadcastFileCache() {
		try {
			std::filesystem::remove(path_);
		} catch (...) {	
		}
	}
	
	void Write(void const* buf, size_t buf_size) {
		file_.write(static_cast<const char*>(buf), buf_size);
	}

	void Close() {
		file_.close();
		is_completed_ = true;
	}

	[[nodiscard]] Path GetFilePath() const {
		return path_;
	}

	[[nodiscard]] bool IsCompleted() const noexcept {
		return is_completed_;
	}

	[[nodiscard]] std::string GetCacheID() {
		return cache_id_;
	}
private:
	bool is_completed_;
	Path path_;
	std::string cache_id_;
	std::ofstream file_;
};

class XAMP_STREAM_API PodcastFileCacheManager {
public:
	friend class Singleton<PodcastFileCacheManager>;

	XAMP_DISABLE_COPY(PodcastFileCacheManager)
	
	std::shared_ptr<PadcastFileCache> GetOrAdd(std::string const& cache_id) {
		auto cache = cache_.Find(cache_id);
		if (!cache) {
			cache_.AddOrUpdate(cache_id, std::make_shared<PadcastFileCache>(cache_id));
			cache = cache_.Find(cache_id);
		}
		return (*cache)->shared_from_this();
	}

	void Remove(std::string const& cache_id) {
		cache_.Erase(cache_id);
	}

	void Remove(std::shared_ptr<PadcastFileCache> const & file_cache) {
		Remove(file_cache->GetCacheID());
	}

	void Load(Path const& path) {
		for (auto const& file_or_dir 
			: RecursiveDirectoryIterator(Fs::absolute(path), kIteratorOptions)) {
		}
	}

	static std::string ToCacheID(Path const& file_path) {
		Path path(file_path);
		auto dot_pos = path.filename().string().find(".");
		auto cache_id = path.filename().string().substr(0, dot_pos);
		return cache_id;
	}
	
private:
	PodcastFileCacheManager() = default;
	
	LruCache<std::string, std::shared_ptr<PadcastFileCache>> cache_;
};

}
