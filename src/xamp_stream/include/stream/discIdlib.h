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

namespace xamp::stream {

class XAMP_STREAM_API DiscIdLib final {
public:
	DiscIdLib();

	XAMP_DISABLE_COPY(DiscIdLib)

private:
	base::SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL(discid_new) discid_new;
	XAMP_DECLARE_DLL(discid_free) discid_free;
	XAMP_DECLARE_DLL(discid_read) discid_read;
	XAMP_DECLARE_DLL(discid_get_id) discid_get_id;
	XAMP_DECLARE_DLL(discid_get_freedb_id) discid_get_freedb_id;
	XAMP_DECLARE_DLL(discid_get_submission_url) discid_get_submission_url;
	XAMP_DECLARE_DLL(discid_get_default_device) discid_get_default_device;
	XAMP_DECLARE_DLL(discid_get_error_msg) discid_get_error_msg;
};

}

#endif
