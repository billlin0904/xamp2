//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

#ifdef XAMP_OS_WIN

namespace xamp::stream {

class XAMP_STREAM_API MBDiscId {
public:
	MBDiscId();

	XAMP_PIMPL(MBDiscId)

	std::string GetDiscId(const std::string& drive) const;

	std::string GetFreeDBId(const std::string & drive) const;

	std::string GetSubmissionUrl(const std::string& drive) const;

	std::string GetDiscIdLookupUrl(const std::string& drive) const;
private:
	class MBDiscIdImpl;
	PimplPtr<MBDiscIdImpl> impl_;
};

}

#endif
