//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <metadata/metadata.h>

extern "C" {
#include <chromaprint.h>
}

XAMP_METADATA_NAMESPACE_BEGIN

class ChromaprintLib final {
public:
	friend class Singleton<ChromaprintLib>;

	XAMP_DISABLE_COPY(ChromaprintLib)

private:
	ChromaprintLib();

	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(chromaprint_new);
	XAMP_DECLARE_DLL_NAME(chromaprint_free);
	XAMP_DECLARE_DLL_NAME(chromaprint_start);
	XAMP_DECLARE_DLL_NAME(chromaprint_feed);
	XAMP_DECLARE_DLL_NAME(chromaprint_finish);
	XAMP_DECLARE_DLL_NAME(chromaprint_get_raw_fingerprint);
	XAMP_DECLARE_DLL_NAME(chromaprint_encode_fingerprint);
	XAMP_DECLARE_DLL_NAME(chromaprint_dealloc);
};

#define CHROMAPRINT_LIB Singleton<ChromaprintLib>::GetInstance()

inline ChromaprintLib::ChromaprintLib() try
	: module_(OpenSharedLibrary("chromaprint"))
	, XAMP_LOAD_DLL_API(chromaprint_new)
	, XAMP_LOAD_DLL_API(chromaprint_free)
	, XAMP_LOAD_DLL_API(chromaprint_start)
	, XAMP_LOAD_DLL_API(chromaprint_feed)
	, XAMP_LOAD_DLL_API(chromaprint_finish)
	, XAMP_LOAD_DLL_API(chromaprint_get_raw_fingerprint)
	, XAMP_LOAD_DLL_API(chromaprint_encode_fingerprint)
	, XAMP_LOAD_DLL_API(chromaprint_dealloc) {
}
catch (Exception const& e) {
    //XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

XAMP_METADATA_NAMESPACE_END

