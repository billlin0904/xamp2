//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/fs.h>

struct archive_entry;
struct archive;

XAMP_BASE_NAMESPACE_BEGIN

struct ArchivePtrDeleter final {
	static archive* invalid() noexcept;
	static void Close(archive* value);
};

using ArchivePtrHandle = UniqueHandle<archive*, ArchivePtrDeleter>;
using ArchiveProgressCallback = std::function<bool(uint64_t, std::optional<uint64_t>)>;

class XAMP_BASE_API ArchiveEntry {
	std::wstring name;
	Path archive_path;
	long length{};
	archive_entry* entry{};
	ArchivePtrHandle archive_ptr;

public:
	ArchiveEntry() = default;

	ArchiveEntry(const std::wstring &name,
		Path                  archive_path,
		long                  length,
		archive_entry*        entry,
		ArchivePtrHandle      archive_ptr) noexcept
		: name(name)
		, archive_path(std::move(archive_path))
		, length(length)
		, entry(entry)
		, archive_ptr(std::move(archive_ptr)) {
	}

	ArchiveEntry(ArchiveEntry&& other) noexcept {
		*this = std::move(other);
	}

	ArchiveEntry& operator=(ArchiveEntry&& other) noexcept {
		if (this != &other) {
			name         = std::move(other.name);
			archive_path = std::move(other.archive_path);
			entry        = other.entry;
			archive_ptr  = std::move(other.archive_ptr);
			length       = other.length;
			other.entry  = nullptr;
			other.length = 0;			
		}
		return *this;
	}

	std::expected<ptrdiff_t, std::string> Read(char *buffer, long length);

	const std::wstring& Name() const noexcept {
		return name; 
	}

	const Path& ArchivePath() const noexcept { 
		return archive_path;
	}

	long Length() const noexcept {
		return length;
	}

	archive_entry* Entry() const noexcept {
		return entry;
	}

	const ArchivePtrHandle& Handle() const noexcept { 
		return archive_ptr; 
	}
};

class XAMP_BASE_API ArchiveFile {
public:
	ArchiveFile();

	XAMP_PIMPL(ArchiveFile)

	std::expected<std::vector<std::wstring>, std::string> Open(const Path& archive_path);

	std::vector<std::wstring> ListEntries() const;

	std::expected<ArchiveEntry, std::string> GetEntryByName(const std::wstring& entry_name);

	static std::vector<std::expected<ArchiveEntry, std::string>> GetAllEntry(ArchiveFile &file_);
private:
	class ArchiveFileImpl;
	ScopedPtr<ArchiveFileImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
