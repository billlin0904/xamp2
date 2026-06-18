//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <expected>

#include <base/memory.h>
#include <base/memory.h>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API MemoryMappedFile {
public:
    MemoryMappedFile();

    XAMP_PIMPL(MemoryMappedFile)

    [[nodiscard]] bool open(std::wstring const &file_path, bool is_module = false);

    [[nodiscard]] void const * data() const ;

    [[nodiscard]] size_t length() const;

	void close() ;

private:
	class MemoryMappedFileImpl;
	ScopedPtr<MemoryMappedFileImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
