//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/align_ptr.h>
#include <base/memory.h>
#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API MemoryMappedFile {
public:
	MemoryMappedFile();	

	XAMP_PIMPL(MemoryMappedFile)

	void Open(const std::wstring &file_path);

	void Open(FILE* file);

	const void * GetData() const;

	int64_t GetLength() const;

	void Close();

private:
	class MemoryMappedFileImpl;
	AlignPtr<MemoryMappedFileImpl> impl_;
};

}

