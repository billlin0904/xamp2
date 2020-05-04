//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/align_ptr.h>
#include <base/memory.h>
#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API MemoryMappedFile final {
public:
	MemoryMappedFile();	

	XAMP_PIMPL(MemoryMappedFile)

	void Open(const std::wstring &file_path);

    const void * GetData() const;

    size_t GetLength() const;

	void Close() noexcept;

private:
	class MemoryMappedFileImpl;
	AlignPtr<MemoryMappedFileImpl> impl_;
};

}

