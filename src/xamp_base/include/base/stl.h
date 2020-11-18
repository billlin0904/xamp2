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

#if 0
template <typename E, typename T, int N>
std::basic_istream<E, T>& operator>>(std::basic_istream<E, T>& in, const E(&sliteral)[N]) {
	std::array<E, N - 1> buffer; //get buffer
	in >> buffer[0]; //skips whitespace
	if (N > 2)
		in.read(&buffer[1], N - 2); //read the rest
	if (strncmp(&buffer[0], sliteral, N - 1)) //if it failed
		in.setstate(in.rdstate() | std::ios::failbit); //set the state
	return in;
}

template <typename E, typename T>
std::basic_istream<E, T>& operator>>(std::basic_istream<E, T>& in, const E& cliteral) {
	E buffer;  //get buffer
	in >> buffer; //read data
	if (buffer != cliteral) //if it failed
		in.setstate(in.rdstate() | std::ios::failbit); //set the state
	return in;
}

// Redirect mutable char arrays to their normal function
template <typename E, typename T, int N>
std::basic_istream<E, T>& operator>>(std::basic_istream<E, T>& in, E(&carray)[N]) {
	return std::operator>>(in, carray);
}
#endif

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

