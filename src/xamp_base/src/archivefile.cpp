#include <archive.h>
#include <archive_entry.h>

#include <base/unique_handle.h>

#include <base/dll.h>
#include <base/shared_singleton.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <fstream>

#include <base/archivelib.h>
#include <base/archivefile.h>

#if 0
#include <llfio/llfio.hpp>
#include <ntkernel-error-category/ntkernel_category.hpp>
#endif

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	std::string GetLinArchiveErrorMessage(const ArchivePtrHandle& archive_ptr) {
		auto *msg = LIBARCHIVE_LIB.archive_error_string(archive_ptr.get());
		if (!msg) {
			return "";
		}
		return msg;
	}
}

archive* ArchivePtrDeleter::invalid() noexcept {
	return nullptr;
}

void ArchivePtrDeleter::Close(archive* value) {
	LIBARCHIVE_LIB.archive_read_free(value);
}

std::expected<long long, std::string> ArchiveEntry::Read(char* buffer, long length) {
	auto ret = LIBARCHIVE_LIB.archive_read_data(archive_ptr.get(), buffer, length);
	if (ret < 0) {
		return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
	}
	return ret;
}

#if !USE_CSTDIO
#if 0
namespace llfio = LLFIO_V2_NAMESPACE;

using ssize_t = long long;

struct LlfioStream {
	llfio::file_handle fh;
	uint64_t           offset = 0;

	static int open_cb(struct archive* a, void* client) {
		auto* s = static_cast<LlfioStream*>(client);
		(void)a;
		return ARCHIVE_OK;
	}

	static ssize_t read_cb(struct archive* a, void* client,
		const void** buff) {
		auto* s = static_cast<LlfioStream*>(client);

		static thread_local std::array<char, 64 * 1024> tlbuf;
		llfio::file_handle::buffer_type b{ tlbuf.data(), tlbuf.size() };
		llfio::file_handle::buffers_type bufs(&b, 1);
		llfio::file_handle::io_request<decltype(bufs)> req(bufs, s->offset);

		auto r = s->fh.read(req);
		if (!r) {
			archive_set_error(a, int(r.error().value()),
				r.error().message().c_str());
			return -1;
		}
		const auto n = r.bytes_transferred();
		if (n == 0) return 0;                     // EOF

		s->offset += n;
		*buff = tlbuf.data();
		return static_cast<ssize_t>(n);
	}
};
#endif
#endif

class ArchiveFile::ArchiveFileImpl {
public:
	static constexpr size_t kArchiveBlockSize = 10240;

	std::expected<std::vector<std::wstring>, std::string> Open(const Path& archive_path) {
		ArchivePtrHandle archive_ptr;
		archive_ptr.reset(LIBARCHIVE_LIB.archive_read_new());
		if (!archive_ptr) {
			return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
		}

		PrefetchFile(archive_path);

		archive_path_ = archive_path;
		LIBARCHIVE_LIB.archive_read_support_format_all(archive_ptr.get());
		LIBARCHIVE_LIB.archive_read_support_filter_all(archive_ptr.get());

		if (LIBARCHIVE_LIB.archive_read_open_filename_w(archive_ptr.get(),
			archive_path_.wstring().c_str(), kArchiveBlockSize) != ARCHIVE_OK) {
			return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
		}

		entries_.clear();
		archive_entry* entry = nullptr;
		while (LIBARCHIVE_LIB.archive_read_next_header(archive_ptr.get(), &entry) == ARCHIVE_OK) {
			auto length = LIBARCHIVE_LIB.archive_entry_size(entry);
			const wchar_t* name = LIBARCHIVE_LIB.archive_entry_pathname_w(entry);
			if (name && length > 0)
				entries_.push_back(name);
			LIBARCHIVE_LIB.archive_read_data_skip(archive_ptr.get());
		}

		archive_ptr.reset();
		archive_path_ = archive_path;
		return entries_;
	}

	std::vector<std::wstring> ListEntries() const {
		return entries_;
	}

	std::expected<ArchiveEntry, std::string> OpenEntry(const std::wstring& entry_name) {
		auto entry = FindEntry(entry_name);
		if (!entry) {
			return entry;
		}
		return std::move(entry.value());
	}

	std::expected<ArchiveEntry, std::string> FindEntry(const std::wstring& entry_name) {
		ArchivePtrHandle archive_ptr;
		archive_ptr.reset(LIBARCHIVE_LIB.archive_read_new());
		if (!archive_ptr) {
			return std::unexpected("Failed to archive_read_new");
		}

		LIBARCHIVE_LIB.archive_read_support_format_all(archive_ptr.get());
		LIBARCHIVE_LIB.archive_read_support_filter_all(archive_ptr.get());
		if (LIBARCHIVE_LIB.archive_read_open_filename_w(archive_ptr.get(),
			archive_path_.wstring().c_str(), kArchiveBlockSize) != ARCHIVE_OK) {
			return std::unexpected(GetLinArchiveErrorMessage(archive_ptr));
		}

		archive_entry* entry = nullptr;
		while (LIBARCHIVE_LIB.archive_read_next_header(archive_ptr.get(), &entry) == ARCHIVE_OK) {
			const wchar_t* name = LIBARCHIVE_LIB.archive_entry_pathname_w(entry);
			if (!name) {
				continue;
			}
			auto length = LIBARCHIVE_LIB.archive_entry_size(entry);
			if (name == entry_name && length > 0) {
				ArchiveEntry result(entry_name, archive_path_, length, entry, std::move(archive_ptr));
				return result;
			}
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

std::expected<ArchiveEntry, std::string> ArchiveFile::OpenEntry(const std::wstring& entry_name) {
	return impl_->OpenEntry(entry_name);
}

std::vector<std::expected<ArchiveEntry, std::string>> ArchiveFile::GetAllEntry(ArchiveFile& file) {
	std::vector<std::expected<ArchiveEntry, std::string>> entries;

	for (const auto& entry : file.ListEntries()) {
		entries.push_back(file.OpenEntry(entry));
	}
	return entries;
}

XAMP_BASE_NAMESPACE_END