//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <mutex>

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace base;

template <typename T>
class XAMP_PLAYER_API MemPool {
public:
	explicit MemPool(size_t buf_size, size_t num_entries = 8)
		: buf_size_(buf_size)
		, num_entries_(0) {
		FillPool(num_entries);
	}

	XAMP_DISABLE_COPY(MemPool)

	void clear() {
		std::lock_guard guard{ mutex_ };
		free_.clear();
	}
	
	Buffer<T> Allocate() {
		std::lock_guard guard{ mutex_ };
		if (free_.empty()) {
			FillPool(num_entries_);
		}
		auto result = std::move(free_.back());
		free_.pop_back();
		--num_entries_;
		return result;
	}

	void Release(Buffer<T> &buffer) {
		std::lock_guard guard{ mutex_ };
		free_.push_back(std::move(buffer));
	}

private:
	void FillPool(size_t num_entries) {
		for (size_t i = 0; i < num_entries; ++i) {
			free_.push_back(Buffer<T>(buf_size_));
		}
		num_entries_ += num_entries;
	}

	size_t buf_size_;
	size_t num_entries_;
	mutable std::mutex mutex_;
	std::vector<Buffer<T>> free_;
};

}
