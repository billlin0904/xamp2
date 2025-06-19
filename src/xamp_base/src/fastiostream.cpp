#include <base/fastiostream.h>
#include <base/platform.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/executor.h>

#include <deque>
#include <variant>

#include <llfio/llfio.hpp>
#include <ntkernel-error-category/ntkernel_category.hpp>

XAMP_BASE_NAMESPACE_BEGIN

namespace llfio = LLFIO_V2_NAMESPACE;


CTemporaryFile::CTemporaryFile()
	: file_(nullptr, fclose) {
	auto [file_ptr, path] = GetTempFile();
	file_ = std::move(file_ptr);
	path_ = std::move(path);
}

CTemporaryFile::~CTemporaryFile() {
	Close();
}

bool CTemporaryFile::Seek(uint64_t off, int32_t origin) noexcept {
#ifdef XAMP_OS_WIN
	return _fseeki64(file(), static_cast<uint64_t>(off), origin) == 0;
#else
	return fseeko64(file(), static_cast<off64_t>(off), origin) == 0;
#endif
}

uint64_t CTemporaryFile::Tell() noexcept {
#ifdef XAMP_OS_WIN
	return static_cast<uint64_t>(_ftelli64(file()));
#else
	return static_cast<uint64_t>(ftello64(file()));
#endif
}

void CTemporaryFile::Close() noexcept {
	if (!file_) {
		return;
	}
	file_.reset();
	try {
		Fs::remove(path_);
	}
	catch (...) {
	}
}

std::tuple<CFilePtr, Path> CTemporaryFile::GetTempFile() {
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(GetSequentialUUID() + ".tmp");
		CFilePtr file(::_wfopen(path.wstring().c_str(), L"wb+"), fclose);
		if (file) {
			return std::make_tuple(std::move(file), path);
		}
	}
	throw PlatformException("Can't create temp file.");
}

constexpr auto kMaxRetryCreateTempFile = 128;

class TemporaryFile::TemporaryFileImpl {
public:
	TemporaryFileImpl() {
		for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
			auto file_name = Fs::path(GetSequentialUUID() + ".tmp");
			auto r = llfio::file_handle::temp_file(file_name,
				llfio::file_handle::mode::write,
				llfio::file_handle::creation::if_needed,
				llfio::file_handle::caching::all);
			if (!r) {
				XAMP_LOG_DEBUG(r.error().message());
			}
			handle_ = std::move(r.value());
			path_ = handle_.current_path().value();
			break;
		}
		if (!handle_.is_valid()) {
			throw PlatformException("Can't create temp file.");
		}
	}

	size_t Write(const void* buffer, size_t size, size_t count) noexcept {
		if (!handle_.is_valid())
			return 0;

		llfio::file_handle::const_buffer_type buf{
			static_cast<const llfio::byte*>(buffer),
			size * count };

		llfio::file_handle::const_buffers_type bufs(&buf, 1);
		llfio::file_handle::io_request<decltype(bufs)> req(bufs, pos_);

		auto io_res = handle_.write(req);
		if (!io_res)
			return 0;

		auto bytes = io_res.bytes_transferred();
		pos_ += bytes;
		return bytes / size;
	}

	size_t Read(void* buffer, size_t size, size_t count) noexcept {
		if (!handle_.is_valid())
			return 0;

		llfio::file_handle::buffer_type buf{
			static_cast<llfio::byte*>(buffer),
			size * count };

		llfio::file_handle::buffers_type bufs(&buf, 1);
		llfio::file_handle::io_request<decltype(bufs)> req(bufs, pos_);

		auto io_res = handle_.read(req);
		if (!io_res)
			return 0;

		auto bytes = io_res.bytes_transferred();
		pos_ += bytes;
		return bytes / size;
	}

	bool Seek(uint64_t off, int origin) noexcept {
		switch (origin) {
		case SEEK_SET: pos_ = off;                 break;
		case SEEK_CUR: pos_ += off;                 break;
		case SEEK_END:
			if (auto r = handle_.maximum_extent(); r.has_value())
				pos_ = r.value() + off;
			else return false;
			break;
		default: return false;
		}
		return true;
	}

	uint64_t Tell() const noexcept {
		return pos_;
	}

	void Close() noexcept {
		auto res = handle_.close();
	}

	uint64_t pos_{};
	llfio::file_handle handle_;
	Path path_;
};

TemporaryFile::TemporaryFile()
	: impl_(MakeAlign<TemporaryFileImpl>()) {
}

XAMP_PIMPL_IMPL(TemporaryFile)

size_t TemporaryFile::Read(void* buffer, size_t size, size_t count) noexcept {
	return impl_->Read(buffer, size, count);
}

size_t TemporaryFile::Write(const void* buffer, size_t size, size_t count) noexcept {
	return impl_->Write(buffer, size, count);
}

bool TemporaryFile::Seek(uint64_t off, int32_t origin) noexcept {
	return impl_->Seek(off, origin);
}

uint64_t TemporaryFile::Tell() noexcept {
	return impl_->Tell();
}

void TemporaryFile::Close() noexcept {
	return impl_->Close();
}

class FastIOStream::FastIOStreamImpl {
public:
	FastIOStreamImpl() = default;

	FastIOStreamImpl(const Path& file_path, FastIOStream::Mode m) {
		open(file_path, m);
	}

	~FastIOStreamImpl() {
		close();
	}

	void open(const Path& file_path, Mode m = Mode::Read) {
		close();

		path_ = file_path;
		readonly_ = (m == Mode::Read);

		auto mode = readonly_
			? llfio::file_handle::mode::read
			: llfio::file_handle::mode::write;

		llfio::file_handle::creation creation;

		if (m == FastIOStream::Mode::ReadWriteOnlyExisting) {
			creation = llfio::file_handle::creation::open_existing;
		}
		else {
			creation = readonly_
				? llfio::file_handle::creation::open_existing
				: llfio::file_handle::creation::if_needed;
		}

		auto r = llfio::file_handle::file(
			{},
			path_.native(),
			mode,
			creation,
			llfio::file_handle::caching::all);

		if (!r)
			throw PlatformException(r.error().value());
		fh_ = std::move(r.value());
	}

	size_t read(void* dst, size_t len) {
		if (!fh_.is_valid() || len == 0)
			return 0;

		llfio::file_handle::buffer_type  b{ static_cast<llfio::byte*>(dst), len };
		llfio::file_handle::buffers_type bufs(&b, 1);
		llfio::file_handle::io_request<decltype(bufs)> req(bufs, pos_);

		auto res = fh_.read(req);
		if (!res) {
			constexpr int STATUS_END_OF_FILE = 0xC0000011;
			if (res.error().value() == STATUS_END_OF_FILE) {
				return 0;
			}
			throw PlatformException(res.error().value());
		}

		pos_ += res.bytes_transferred();
		return res.bytes_transferred();
	}

	size_t write(const void* src, size_t len) {
		if (readonly_ || !fh_.is_valid() || len == 0)
			return 0;

		const llfio::byte* p = static_cast<const llfio::byte*>(src);
		size_t total = 0;
		while (total < len) {
			size_t chunk = len - total;
			llfio::file_handle::const_buffer_type b{ p + total, chunk };
			llfio::file_handle::const_buffers_type bufs(&b, 1);
			llfio::file_handle::io_request req(bufs, pos_ + total);

			auto res = fh_.write(req);
			if (!res || res.bytes_transferred() == 0)
				throw PlatformException(res ? -1 : res.error().value());

			total += res.bytes_transferred();
		}
		pos_ += total;
		return total;
	}

	void seek(int64_t off, int whence) {
		uint64_t newpos = 0;

		switch (whence) {
		case SEEK_SET:
			newpos = (off < 0) ? 0ULL : static_cast<uint64_t>(off);
			break;
		case SEEK_CUR: {
			int64_t temp = static_cast<int64_t>(pos_) + off;
			if (temp < 0)
				temp = 0;
			newpos = static_cast<uint64_t>(temp);
			break;
		}
		case SEEK_END: {
			int64_t temp = static_cast<int64_t>(size()) + off;
			if (temp < 0)
				temp = 0;
			newpos = static_cast<uint64_t>(temp);
			break;
		}
		}
		pos_ = newpos;
	}

	uint64_t tell() const {
		return pos_;
	}

	uint64_t size() const {
		return fh_.is_valid()
			? fh_.maximum_extent().value()
			: 0ULL;
	}

	void truncate(uint64_t new_size) {
		if (readonly_ || !fh_.is_valid())
			return;

		auto res = fh_.truncate(new_size);
		if (!res)
			throw PlatformException(res.error().value());

		if (pos_ > new_size)
			pos_ = new_size;
	}

	bool is_open()  const {
		return fh_.is_valid();
	}

	bool read_only()const {
		return readonly_;
	}

	const Path& path() const {
		return path_;
	}

	void close() {
		if (!fh_.is_valid())
			return;
		auto res = fh_.close();
	}
private:
	bool               readonly_{ true };
	uint64_t           pos_{ 0 };
	llfio::file_handle fh_;
	Path               path_;
};

FastIOStream::FastIOStream()
	: impl_(MakeAlign<FastIOStreamImpl>()) {
}

FastIOStream::FastIOStream(const Path& file_path, Mode m)
	: impl_(MakeAlign<FastIOStreamImpl>(file_path, m)) {
}

void FastIOStream::open(const Path& file_path, Mode m) {
	impl_->open(file_path, m);
}

XAMP_PIMPL_IMPL(FastIOStream)

size_t FastIOStream::read(void* dst, size_t len) {
	return impl_->read(dst, len);
}

size_t FastIOStream::write(const void* src, size_t len) {
	return impl_->write(src, len);
}

void FastIOStream::seek(int64_t off, int whence) {
	return impl_->seek(off, whence);
}

uint64_t FastIOStream::tell() const {
	return impl_->tell();
}

uint64_t FastIOStream::size() const {
	return impl_->size();
}

void FastIOStream::truncate(uint64_t new_size) {
	return impl_->truncate(new_size);
}

bool FastIOStream::is_open() const {
	return impl_->is_open();
}

bool FastIOStream::read_only() const {
	return impl_->read_only();
}

const Path& FastIOStream::path() const {
	return impl_->path();
}

void FastIOStream::close() {
	return impl_->close();
}

XAMP_BASE_NAMESPACE_END