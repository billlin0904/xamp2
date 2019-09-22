//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <sstream>
#include <string>
#include <iomanip>

#include <base/base.h>

namespace xamp::base {

static std::string ByteSizeToString(int32_t bytes) {
	static const int32_t GB = (1024 * 1024 * 1024);
	static const int32_t MB = (1024 * 1024);
	static const int32_t KB = 1024;

	std::stringstream buffer;
	buffer.setf(std::ios::fixed);
	buffer << std::setprecision(1);

	if (bytes <= 0) 
		buffer << "0 B";
	else if (bytes > 0 && bytes < KB)
		buffer << bytes << " B";
	else if (bytes >= KB)
		buffer << bytes / KB << " KB";
	else if (bytes >= MB)
		buffer << bytes / MB << " MB";
	else if (bytes >= GB)
		buffer << bytes / GB << " GB";

	return buffer.str();
}

template <typename CharType>
std::basic_string<CharType> ToUpper(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

template <typename CharType>
std::basic_string<CharType> ToLower(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}
	
}
