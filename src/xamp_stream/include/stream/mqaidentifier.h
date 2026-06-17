//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/stl.h>

#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API MqaIdentifier {
public:
	explicit MqaIdentifier(const Path& file_path);

	bool detect();

	bool isMQA() const;

	bool isMQAStudio() const;

	uint32_t getOriginalSampleRate() const;

	XAMP_PIMPL(MqaIdentifier)

private:
	class MqaIdentifierImpl;
	ScopedPtr<MqaIdentifierImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
