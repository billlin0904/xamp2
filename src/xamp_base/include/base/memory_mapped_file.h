//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/memory.h>
#include <base/pimplptr.h>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API MemoryMappedFile {
public:
    MemoryMappedFile();

    XAMP_PIMPL(MemoryMappedFile)

    void Open(std::wstring const &file_path, bool is_module = false);

    [[nodiscard]] void const * GetData() const noexcept;

    [[nodiscard]] size_t GetLength() const;

	void Close() noexcept;

private:
	class MemoryMappedFileImpl;
	ScopedPtr<MemoryMappedFileImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
