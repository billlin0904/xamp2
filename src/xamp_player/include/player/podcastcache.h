// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory.h>
#include <fstream>
#include <filesystem>

#include <base/lrucache.h>
#include <base/filecachecallback.h>
#include <player/player.h>

namespace xamp::player {

using xamp::base::LruCache;
using xamp::base::FileCacheCallback;

class XAMP_PLAYER_API TempFileCacheCallback : public FileCacheCallback {
public:
	explicit TempFileCacheCallback(std::string const& cache_id)
		: cache_id_(cache_id) {
		const auto path = std::filesystem::temp_directory_path();
		tempfile_.open(path);
	}
	
	void Write(float const* buf, size_t buf_size) override {
		tempfile_.write(buf, buf_size * sizeof(float));
	}

	void Close() override {
		tempfile_.close();
	}

private:
	std::string cache_id_;
	std::ofstream tempfile_;
};

class XAMP_PLAYER_API PodcastFileCache {
public:
private:
	LruCache<std::string, std::shared_ptr<FileCacheCallback>> cache_;
};

}