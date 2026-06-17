//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/base.h>
#include <base/memory.h>

#ifdef XAMP_OS_WIN

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API MBDiscId {
public:
	MBDiscId();

	XAMP_PIMPL(MBDiscId)

	[[nodiscard]] std::string getDiscId(const std::string& drive) const;

	[[nodiscard]] std::string getFreeDBId(const std::string & drive) const;

	[[nodiscard]] std::string getSubmissionUrl(const std::string& drive) const;

	[[nodiscard]] std::string getDiscIdLookupUrl(const std::string& drive) const;
private:
	class MBDiscIdImpl;
	ScopedPtr<MBDiscIdImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

#endif
