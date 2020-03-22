//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <type_traits>

#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

template <typename T, size_t kAlignment = XAMP_MALLOC_ALGIGN_SIZE>
class AlignedAllocator {
public:
	using value_type = T;
	using size_type = size_t;
	using pointer = typename std::add_pointer<value_type>::type;
	using const_pointer = typename std::add_pointer<const value_type>::type;

	template<class U>
	struct rebind {
		using other = AlignedAllocator<U, kAlignment>;
	};

	AlignedAllocator() noexcept {
	}

	template <typename U>
	AlignedAllocator(const AlignedAllocator<U, kAlignment>&) noexcept {
	}

	pointer allocate(size_type n, const_pointer /* hint */ = nullptr) const {
		auto p = AlignedMallocCountOf<value_type>(n, kAlignment);
		if (p == nullptr) {
			throw std::bad_alloc{};
		}
		return p;
	}

	void deallocate(pointer p, size_type /* n */) const noexcept {
		AlignedFree(p);
	}
};

template <typename T, size_t kAlignment1, typename U, size_t kAlignment2>
static bool operator==(const AlignedAllocator<T, kAlignment1>&, const AlignedAllocator<U, kAlignment2>&) noexcept {
	return kAlignment1 == kAlignment2;
}

template <typename T, size_t kAlignment1, typename U, size_t kAlignment2>
static bool operator!=(const AlignedAllocator<T, kAlignment1>& lhs, const AlignedAllocator<U, kAlignment2>& rhs) noexcept {
	return !(lhs == rhs);
}

}

