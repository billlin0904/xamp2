//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/base.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#ifdef XAMP_OS_WIN

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API MBDiscId {
public:
	MBDiscId();

	XAMP_PIMPL(MBDiscId)

	[[nodiscard]] std::string GetDiscId(const std::string& drive) const;

	[[nodiscard]] std::string GetFreeDBId(const std::string & drive) const;

	[[nodiscard]] std::string GetSubmissionUrl(const std::string& drive) const;

	[[nodiscard]] std::string GetDiscIdLookupUrl(const std::string& drive) const;
private:
	class MBDiscIdImpl;
	AlignPtr<MBDiscIdImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

#endif
