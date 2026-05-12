#include <archive.h>
#include <archive_entry.h>

#include <base/unique_handle.h>

#include <base/dll.h>
#include <base/shared_singleton.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/fastiostream.h>

#include <fstream>

#include <base/archivelib.h>
#include <base/archivefile.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	std::string GetLinArchiveErrorMessage(const ArchivePtrHandle& archive_ptr) {
		auto *msg = LIBARCHIVE_LIB.archive_error_string(archive_ptr.get());
		if (!msg) {
			return "";
		}
		return msg;
	}

	bool IsRegularFile(archive_entry* e) {
		return (LIBARCHIVE_LIB.archive_entry_filetype(e) == AE_IFREG);
	}

	ArchivePtrHandle MakeArchivePtr() {
		ArchivePtrHandle archive_ptr;
		archive_ptr.reset(LIBARCHIVE_LIB.archive_read_new());
		if (!archive_ptr) {
			throw std::runtime_error("Failed to archive_read_new");
		}
		LIBARCHIVE_LIB.archive_read_support_format_all(archive_ptr.get());
		LIBARCHIVE_LIB.archive_read_support_filter_all(archive_ptr.get());
		return archive_ptr;
	}
}

archive* ArchivePtrDeleter::invalid() {
	return nullptr;
}

void ArchivePtrDeleter::Close(archive* value) {
	if (value) {
		LIBARCHIVE_LIB.archive_read_free(value);
	}
}

std::expected<ptrdiff_t, std::string> ArchiveEntry::Read(char* buffer, long length) {
	auto ret = LIBARCHIVE_LIB.archive_read_data(archive_ptr.get(), buffer, length);
	if (ret < 0) {
		return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
	}
	return ret;
}

class ArchiveFile::ArchiveFileImpl {
public:
	static constexpr size_t kArchiveBlockSize = 10240;

	std::expected<std::vector<std::wstring>, std::string> Open(const Path& archive_path) {
		try {
			auto archive_ptr = MakeArchivePtr();			
			PrefetchFile(archive_path);

			if (LIBARCHIVE_LIB.archive_read_open_filename_w(archive_ptr.get(),
				archive_path.wstring().c_str(), kArchiveBlockSize) != ARCHIVE_OK) {
				return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
			}

			entries_.clear();

			archive_entry* entry = nullptr;
			while (LIBARCHIVE_LIB.archive_read_next_header(archive_ptr.get(), &entry) == ARCHIVE_OK) {
				const wchar_t* name = LIBARCHIVE_LIB.archive_entry_pathname_w(entry);
				if (name && IsRegularFile(entry))
					entries_.push_back(name);
				LIBARCHIVE_LIB.archive_read_data_skip(archive_ptr.get());
			}
		}
		catch (...) {
			return std::unexpected("Failed to open archive file");
		}
		
		archive_path_ = archive_path;
		return entries_;
	}

	std::vector<std::wstring> ListEntries() const {
		return entries_;
	}

	std::expected<ArchiveEntry, std::string> GetEntryByName(const std::wstring& entry_name) {
		auto entry = FindEntry(entry_name);
		if (!entry) {
			return entry;
		}
		return std::move(entry.value());
	}

	std::expected<ArchiveEntry, std::string> FindEntry(const std::wstring& entry_name) {
		try {
			auto archive_ptr = MakeArchivePtr();
			if (LIBARCHIVE_LIB.archive_read_open_filename_w(archive_ptr.get(),
				archive_path_.wstring().c_str(),
				kArchiveBlockSize) != ARCHIVE_OK) {
				return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
			}

			archive_entry* entry = nullptr;
			while (LIBARCHIVE_LIB.archive_read_next_header(archive_ptr.get(), &entry) == ARCHIVE_OK) {
				const wchar_t* name = LIBARCHIVE_LIB.archive_entry_pathname_w(entry);
				if (!name) {
					continue;
				}
				auto length = LIBARCHIVE_LIB.archive_entry_size(entry);
				if (name == entry_name && IsRegularFile(entry)) {
					ArchiveEntry result(entry_name, archive_path_, length, entry, std::move(archive_ptr));
					return result;
				}
			}
		}
		catch (...) {
			return std::unexpected("Failed to open archive entry");
		}
		return std::unexpected("Not found entry name");
	}

private:
	Path archive_path_;
	std::vector<std::wstring> entries_;
};

XAMP_PIMPL_IMPL(ArchiveFile)

ArchiveFile::ArchiveFile()
	: impl_(MakeAlign<ArchiveFileImpl>()) {
}

std::expected<std::vector<std::wstring>, std::string> ArchiveFile::Open(const Path& archive_path) {
	return impl_->Open(archive_path);
}

std::vector<std::wstring> ArchiveFile::ListEntries() const {
	return impl_->ListEntries();
}

std::expected<ArchiveEntry, std::string> ArchiveFile::GetEntryByName(const std::wstring& entry_name) {
	return impl_->GetEntryByName(entry_name);
}

std::vector<std::expected<ArchiveEntry, std::string>> ArchiveFile::GetAllEntry(ArchiveFile& file_) {
	std::vector<std::expected<ArchiveEntry, std::string>> entries;

	for (const auto& entry : file_.ListEntries()) {
		entries.push_back(file_.GetEntryByName(entry));
	}
	return entries;
}

XAMP_BASE_NAMESPACE_END