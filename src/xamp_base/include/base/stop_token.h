//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <base/align_ptr.h>
#include <base/base.h>

namespace xamp::base {

using SharedStopState = std::shared_ptr<std::atomic<bool>>;

struct NoStopState {
	explicit NoStopState() = default;
};

inline constexpr NoStopState kNoStopState{};

class XAMP_BASE_API StopToken {
public:
	StopToken() noexcept = default;

	StopToken(const StopToken& other) noexcept = default;

	StopToken(StopToken&& other) noexcept
		: shared_state_(std::exchange(other.shared_state_, nullptr)) {
	}

	~StopToken() = default;

	StopToken& operator=(const StopToken& other) noexcept {
		if (shared_state_ != other.shared_state_) {
			StopToken tmp{ other };
			swap(tmp);
		}
		return *this;
	}

	StopToken& operator=(StopToken&& other) noexcept {
		if (this != &other) {
			StopToken tmp{ std::move(other) };
			swap(tmp);
		}
		return *this;
	}

	[[nodiscard]] bool StopRequested() const noexcept {
		return (shared_state_ != nullptr)
			&& shared_state_->load(std::memory_order_relaxed);
	}

	[[nodiscard]] bool StopPossible() const noexcept {
		return (shared_state_ != nullptr)
			&& (shared_state_->load(std::memory_order_relaxed)
				|| (shared_state_.use_count() > 1));
	}

	[[nodiscard]] friend bool operator==(const StopToken& a, const StopToken& b) noexcept {
		return a.shared_state_ == b.shared_state_;
	}

	void swap(StopToken& other) noexcept {
		std::swap(shared_state_, other.shared_state_);
	}

	[[nodiscard]] friend bool operator!=(const StopToken& a, const StopToken& b) noexcept {
		return a.shared_state_ != b.shared_state_;
	}
private:
	friend class StopSource;

	explicit StopToken(SharedStopState state) noexcept
		: shared_state_(std::move(state)) {
	}

	SharedStopState shared_state_;
};

class XAMP_BASE_API StopSource {
public:
	StopSource()
		: shared_state_(std::make_shared<std::atomic_bool>(false)) {
	}

	explicit StopSource(NoStopState) noexcept {		
	}

	StopSource(const StopSource& other) noexcept = default;

	StopSource(StopSource&& other) noexcept
		: shared_state_(std::exchange(other.shared_state_, nullptr)) {
	}

	~StopSource() = default;

	StopSource& operator=(StopSource&& other) noexcept {
		StopSource tmp{ std::move(other) };
		swap(tmp);
		return *this;
	}

	StopSource& operator=(const StopSource& other) noexcept {
		if (shared_state_ != other.shared_state_) {
			StopSource tmp{ other };
			swap(tmp);
		}
		return *this;
	}

	bool RequestStop() const noexcept {
		if (shared_state_ != nullptr) {
			bool expected = false;
			return shared_state_->compare_exchange_strong(
				expected,
				true,
				std::memory_order_relaxed);
		}
		return false;
	}

	[[nodiscard]] bool StopRequested() const noexcept {
		return (shared_state_ != nullptr)
			&& shared_state_->load(std::memory_order_relaxed);
	}

	[[nodiscard]] bool StopPossible() const noexcept {
		return shared_state_ != nullptr;
	}

	[[nodiscard]] StopToken GetToken() const noexcept {
		return StopToken{ shared_state_ };
	}

	[[nodiscard]] friend bool operator==(const StopSource& a, const StopSource& b) noexcept {
		return a.shared_state_ == b.shared_state_;
	}

	[[nodiscard]] friend bool operator!=(const StopSource& a, const StopSource& b) noexcept {
		return a.shared_state_ != b.shared_state_;
	}

	void swap(StopSource& other) noexcept {
		std::swap(shared_state_, other.shared_state_);
	}
private:
	SharedStopState shared_state_;
};

}
