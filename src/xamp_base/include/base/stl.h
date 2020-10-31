//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <list>

#include <robin_hood.h>

namespace xamp::base {

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>>
using HashSet = robin_hood::unordered_set<T, H, E>;

template <typename ForwardIt, typename T, typename Compare = std::less<>>
ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp = {}) {
	first = std::lower_bound(first, last, value, comp);
	return first != last && !comp(*first, value) ? first : last;
}


}

