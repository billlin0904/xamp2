//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <metadata/api.h>

extern "C" {
#include <libcue.h>
}

XAMP_METADATA_NAMESPACE_BEGIN

class LibCueLib final {
public:
	LibCueLib();

	XAMP_DISABLE_COPY(LibCueLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(cd_delete);
	XAMP_DECLARE_DLL_NAME(cue_parse_string);
	XAMP_DECLARE_DLL_NAME(cd_get_ntrack);
	XAMP_DECLARE_DLL_NAME(cd_get_track);
	XAMP_DECLARE_DLL_NAME(track_get_filename);
	XAMP_DECLARE_DLL_NAME(cd_get_cdtext);
	XAMP_DECLARE_DLL_NAME(cdtext_get);
	XAMP_DECLARE_DLL_NAME(rem_get);
	XAMP_DECLARE_DLL_NAME(cd_get_rem);
	XAMP_DECLARE_DLL_NAME(track_get_start);
	XAMP_DECLARE_DLL_NAME(track_get_length);
	XAMP_DECLARE_DLL_NAME(track_get_cdtext);
	XAMP_DECLARE_DLL_NAME(track_get_rem);
};

#define LIBCUE_LIB Singleton<LibCueLib>::GetInstance()

inline LibCueLib::LibCueLib() try
	: module_(OpenSharedLibrary("libcue"))
	, XAMP_LOAD_DLL_API(cd_delete)
	, XAMP_LOAD_DLL_API(cue_parse_string)
	, XAMP_LOAD_DLL_API(cd_get_ntrack)
	, XAMP_LOAD_DLL_API(cd_get_track)
	, XAMP_LOAD_DLL_API(track_get_filename)
	, XAMP_LOAD_DLL_API(cd_get_cdtext)
	, XAMP_LOAD_DLL_API(cdtext_get)
	, XAMP_LOAD_DLL_API(rem_get)
	, XAMP_LOAD_DLL_API(cd_get_rem)
	, XAMP_LOAD_DLL_API(track_get_start)
	, XAMP_LOAD_DLL_API(track_get_length)
	, XAMP_LOAD_DLL_API(track_get_cdtext)
	, XAMP_LOAD_DLL_API(track_get_rem) {
}
catch (const Exception& e) {
    //XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

XAMP_METADATA_NAMESPACE_END
