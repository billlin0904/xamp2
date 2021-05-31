// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <filesystem>
#include <memory.h>
#include <fstream>

#include <base/lrucache.h>
#include <stream/wavefilewriter.h>

namespace xamp::stream {

using xamp::base::LruCache;
using xamp::base::AudioFormat;
using xamp::stream::WaveFileWriter;

class XAMP_STREAM_API PadcastFileCacheCallback : public std::enable_shared_from_this<PadcastFileCacheCallback> {
public:
	explicit PadcastFileCacheCallback(std::string const& cache_id)
		: is_completed_(false)
		, cache_id_(cache_id) {
		auto file_name = std::tmpnam(nullptr) + std::string(".m4a");
		auto tmp_dir_path{
			std::filesystem::temp_directory_path() /= file_name };
		path_ = tmp_dir_path;
		//tempfile_.Open(path_, AudioFormat::PCM48Khz);
		file_.open(path_, std::ios::binary);
	}

	virtual ~PadcastFileCacheCallback() {
		try {
			std::filesystem::remove(path_);
		} catch (...) {	
		}
	}
	
	void Write(void const* buf, size_t buf_size) {
		//tempfile_.Write(buf, buf_size);
		file_.write(static_cast<const char*>(buf), buf_size);
	}

	void Close() {
		//tempfile_.Close();
		file_.close();
		is_completed_ = true;
	}

	[[nodiscard]] std::filesystem::path GetFilePath() const {
		return path_;
	}

	[[nodiscard]] bool IsCompleted() const noexcept {
		return is_completed_;
	}

private:
	bool is_completed_;
	std::filesystem::path path_;
	std::string cache_id_;
	//WaveFileWriter tempfile_;
	std::ofstream file_;
};

class XAMP_STREAM_API PodcastFileCache {
public:
	std::shared_ptr<PadcastFileCacheCallback> GetOrAdd(std::string const& cache_id) {
		auto cache = cache_.Find(cache_id);
		if (!cache) {
			cache_.AddOrUpdate(cache_id, std::make_shared<PadcastFileCacheCallback>(cache_id));
			cache = cache_.Find(cache_id);
		}
		return (*cache)->shared_from_this();
	}
	
private:
	LruCache<std::string, std::shared_ptr<PadcastFileCacheCallback>> cache_;
};

}