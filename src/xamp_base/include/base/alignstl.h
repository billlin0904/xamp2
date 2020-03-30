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

#include <base/alignallocator.h>
#include <base/hash.h>
#include <base/id.h>

#include <robin_hood.h>

namespace xamp::base {

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using Queue = std::deque<T>;

template <typename T>
using List = std::list<T>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using RobinHoodHashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>>
using RobinHoodSet = robin_hood::unordered_set<T, H, E>;

}

