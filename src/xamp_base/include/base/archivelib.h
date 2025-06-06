//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <base/shared_singleton.h>
#include <archive.h>
#include <archive_entry.h>

XAMP_BASE_NAMESPACE_BEGIN

class ArchiveLib final {
public:
	ArchiveLib();

	XAMP_DISABLE_COPY(ArchiveLib)
private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(archive_read_new);
	XAMP_DECLARE_DLL_NAME(archive_read_free);
	XAMP_DECLARE_DLL_NAME(archive_read_support_format_all);
	XAMP_DECLARE_DLL_NAME(archive_read_support_filter_all);
	XAMP_DECLARE_DLL_NAME(archive_read_open_filename_w);
	XAMP_DECLARE_DLL_NAME(archive_entry_pathname_w);
	XAMP_DECLARE_DLL_NAME(archive_read_data_skip);
	XAMP_DECLARE_DLL_NAME(archive_read_data);
	XAMP_DECLARE_DLL_NAME(archive_entry_size);
	XAMP_DECLARE_DLL_NAME(archive_read_next_header);
	XAMP_DECLARE_DLL_NAME(archive_errno);
	XAMP_DECLARE_DLL_NAME(archive_read_set_options);
	XAMP_DECLARE_DLL_NAME(archive_error_string);
};

inline ArchiveLib::ArchiveLib() try
	: module_(OpenSharedLibrary("archive"))
	, XAMP_LOAD_DLL_API(archive_read_new)
	, XAMP_LOAD_DLL_API(archive_read_free)
	, XAMP_LOAD_DLL_API(archive_read_support_format_all)
	, XAMP_LOAD_DLL_API(archive_read_support_filter_all)
	, XAMP_LOAD_DLL_API(archive_read_open_filename_w)
	, XAMP_LOAD_DLL_API(archive_entry_pathname_w)
	, XAMP_LOAD_DLL_API(archive_read_data_skip)
	, XAMP_LOAD_DLL_API(archive_read_data)
	, XAMP_LOAD_DLL_API(archive_entry_size)
	, XAMP_LOAD_DLL_API(archive_read_next_header)
	, XAMP_LOAD_DLL_API(archive_errno)
	, XAMP_LOAD_DLL_API(archive_read_set_options)
	, XAMP_LOAD_DLL_API(archive_error_string) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

#define LIBARCHIVE_LIB SharedSingleton<ArchiveLib>::GetInstance()

XAMP_BASE_NAMESPACE_END