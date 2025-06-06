//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <array>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
class PimplPtr {
public:
    static constexpr size_t kMaxBufferSize = 256;

	XAMP_DISABLE_COPY(PimplPtr)

	template <typename... Args>
	explicit PimplPtr(Args&&...);

	PimplPtr(PimplPtr<T>&& other) noexcept;

	PimplPtr<T>& operator=(PimplPtr<T>&& other) noexcept;

	~PimplPtr();

	T* operator->() noexcept {
		return reinterpret_cast<T*>(buffer_.data());
	}

	XAMP_CHECK_LIFETIME [[nodiscard]] const T* operator->() const noexcept {
		return reinterpret_cast<const T*>(buffer_.data());
	}

	XAMP_CHECK_LIFETIME [[nodiscard]] T& operator*() noexcept {
		return *get();
	}

	XAMP_CHECK_LIFETIME [[nodiscard]] const T& operator*() const noexcept {
		return *get();
	}

	XAMP_CHECK_LIFETIME [[nodiscard]] T* get() noexcept {
		return reinterpret_cast<T*>(buffer_.data());
	}

	XAMP_CHECK_LIFETIME [[nodiscard]] const T* get() const noexcept {
		return reinterpret_cast<const T*>(buffer_.data());
	}
private:
	bool is_moved_{ false };
#ifdef _DEBUG
	T* debug_;
	size_t size_;
#endif
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::array<uint8_t, kMaxBufferSize> buffer_{};
};

template <typename T>
template <typename... Args>
PimplPtr<T>::PimplPtr(Args&&... args) {
	static_assert(sizeof(T) <= kMaxBufferSize);
	new (buffer_.data()) T{ std::forward<Args>(args)... };
#ifdef _DEBUG
	debug_ = get();
	size_ = sizeof(T);
#endif
}

template <typename T>
PimplPtr<T>::~PimplPtr() {
	if (is_moved_) {
		return;
	}
	get()->~T();
}

template <typename T>
PimplPtr<T>::PimplPtr(PimplPtr<T>&& other) noexcept {
	*this = std::move(other);
}

template <typename T>
PimplPtr<T>& PimplPtr<T>::operator=(PimplPtr<T>&& other) noexcept {
	if (this != &other) {
		other.is_moved_ = true;
		buffer_ = std::move(other.buffer_);
		other.buffer_.fill(0);
	}
	return *this;
}

template <typename T, typename... Args>
PimplPtr<T> MakePimpl(Args&&... args) {
	return PimplPtr<T>(std::forward<Args>(args)...);
}

XAMP_BASE_NAMESPACE_END

