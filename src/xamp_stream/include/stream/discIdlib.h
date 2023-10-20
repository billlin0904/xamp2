//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/dll.h>
#include <stream/stream.h>
#include <discid/discid.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API DiscIdLib final {
public:
	DiscIdLib();

	XAMP_DISABLE_COPY(DiscIdLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(discid_new);
	XAMP_DECLARE_DLL_NAME(discid_free);
	XAMP_DECLARE_DLL_NAME(discid_read);
	XAMP_DECLARE_DLL_NAME(discid_get_id);
	XAMP_DECLARE_DLL_NAME(discid_get_freedb_id);
	XAMP_DECLARE_DLL_NAME(discid_get_submission_url);
	XAMP_DECLARE_DLL_NAME(discid_get_default_device);
	XAMP_DECLARE_DLL_NAME(discid_get_error_msg);
};

XAMP_STREAM_NAMESPACE_END

#endif

