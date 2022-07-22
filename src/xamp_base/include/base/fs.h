//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <filesystem>

namespace xamp::base {

namespace Fs = std::filesystem;
using RecursiveDirectoryIterator = Fs::recursive_directory_iterator;
using DirectoryIterator = Fs::directory_iterator;
using DirectoryEntry = Fs::directory_entry;
using Path = Fs::path;

inline constexpr auto kIteratorOptions{
	std::filesystem::directory_options::follow_directory_symlink |
	std::filesystem::directory_options::skip_permission_denied
};

}
