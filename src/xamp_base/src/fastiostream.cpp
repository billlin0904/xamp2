#include <base/fastiostream.h>
#include <base/platform.h>

#include <base/logger.h>

#include <deque>
#include <variant>
#include <fstream>

#ifdef XAMP_OS_WIN
#include <llfio/llfio.hpp>
#include <ntkernel-error-category/ntkernel_category.hpp>
#endif

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
namespace llfio = LLFIO_V2_NAMESPACE;
#endif


CTemporaryFile::CTemporaryFile()
	: file_(nullptr, fclose) {
	auto [file_ptr, path] = GetTempFile();
	file_ = std::move(file_ptr);
	path_ = std::move(path);
}

CTemporaryFile::~CTemporaryFile() {
	Close();
}

bool CTemporaryFile::Seek(uint64_t off, int32_t origin) {
#ifdef XAMP_OS_WIN
	return _fseeki64(file(), static_cast<uint64_t>(off), origin) == 0;
#else
	return fseeko64(file(), static_cast<off64_t>(off), origin) == 0;
#endif
}

uint64_t CTemporaryFile::Tell() {
#ifdef XAMP_OS_WIN
	return static_cast<uint64_t>(_ftelli64(file()));
#else
	return static_cast<uint64_t>(ftello64(file()));
#endif
}

void CTemporaryFile::Close() {
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
#ifdef XAMP_OS_WIN
		CFilePtr file_(::_wfopen(path.wstring().c_str(), L"wb+"), fclose);
#else
		CFilePtr file_(::fopen(path.string().c_str(), "wb+"), fclose);
#endif
		if (file_) {
			return std::make_tuple(std::move(file_), path);
		}
	}
	throw PlatformException("Can't create temp file.");
}

constexpr auto kMaxRetryCreateTempFile = 128;

class TemporaryFile::TemporaryFileImpl {
public:
	TemporaryFileImpl() {
#ifdef XAMP_OS_WIN
		for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
			auto file_name = Fs::path(GetSequentialUUID() + ".tmp");
			auto r = llfio::file_handle::temp_file(file_name,
				llfio::file_handle::mode::write,
				llfio::file_handle::creation::if_needed,
				llfio::file_handle::caching::all);
			if (!r) {
				XAMP_LOG_DEBUG(r.error().message());
				continue;
			}
			handle_ = std::move(r.value());
			path_ = handle_.current_path().value();
			break;
		}
		if (!handle_.is_valid()) {
			throw PlatformException("Can't create temp file.");
		}
#else
		auto [file, path] = xamp::base::GetTempFile();
		file_ = std::move(file);
		path_ = std::move(path);
#endif
	}

	size_t Write(const void* buffer, size_t size, size_t count) {
#ifdef XAMP_OS_WIN
		if (!handle_.is_valid() || size == 0 || count == 0)
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
#else
		if (!file_.is_open() || size == 0 || count == 0) {
			return 0;
		}
		file_.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(size * count));
		return file_ ? count : 0;
#endif
	}

	size_t Read(void* buffer, size_t size, size_t count) {
#ifdef XAMP_OS_WIN
		if (!handle_.is_valid() || size == 0 || count == 0)
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
#else
		if (!file_.is_open() || size == 0 || count == 0) {
			return 0;
		}
		file_.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size * count));
		return static_cast<size_t>(file_.gcount()) / size;
#endif
	}

	bool Seek(uint64_t off, int origin) {
#ifdef XAMP_OS_WIN
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
#else
		if (!file_.is_open()) {
			return false;
		}
		const auto dir = origin == SEEK_SET
			? std::ios::beg
			: origin == SEEK_CUR ? std::ios::cur : std::ios::end;
		file_.clear();
		file_.seekg(static_cast<std::streamoff>(off), dir);
		file_.seekp(static_cast<std::streamoff>(off), dir);
		return !file_.fail();
#endif
	}

	uint64_t Tell() const {
#ifdef XAMP_OS_WIN
		return pos_;
#else
		return static_cast<uint64_t>(file_.tellg());
#endif
	}

	void Close() {
#ifdef XAMP_OS_WIN
		auto res = handle_.close();
		pos_ = 0;
#else
		file_.close();
		Fs::remove(path_);
#endif
	}

	uint64_t pos_{};
#ifdef XAMP_OS_WIN
	llfio::file_handle handle_;
#else
	mutable std::fstream file_;
#endif
	Path path_;
};

TemporaryFile::TemporaryFile()
	: impl_(MakeAlign<TemporaryFileImpl>()) {
}

XAMP_PIMPL_IMPL(TemporaryFile)

size_t TemporaryFile::Read(void* buffer, size_t size, size_t count) {
	return impl_->Read(buffer, size, count);
}

size_t TemporaryFile::Write(const void* buffer, size_t size, size_t count) {
	return impl_->Write(buffer, size, count);
}

bool TemporaryFile::Seek(uint64_t off, int32_t origin) {
	return impl_->Seek(off, origin);
}

uint64_t TemporaryFile::Tell() {
	return impl_->Tell();
}

void TemporaryFile::Close() {
	return impl_->Close();
}

#ifdef XAMP_OS_WIN
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
		pos_ = 0;

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
		pos_ = 0;
	}
private:
	bool               readonly_{ true };
	uint64_t           pos_{ 0 };
	llfio::file_handle fh_;
	Path               path_;
};
#else
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

		std::ios::openmode mode = std::ios::binary;
		if (m == Mode::Read) {
			mode |= std::ios::in;
		}
		else if (m == Mode::ReadWriteOnlyExisting) {
			mode |= std::ios::in | std::ios::out;
		}
		else {
			mode |= std::ios::in | std::ios::out | std::ios::trunc;
		}

		stream_.open(path_, mode);
		if (!stream_.is_open()) {
			throw PlatformException("Can't open file.");
		}
	}

	size_t read(void* dst, size_t len) {
		if (!stream_.is_open() || len == 0) {
			return 0;
		}
		stream_.read(static_cast<char*>(dst), static_cast<std::streamsize>(len));
		return static_cast<size_t>(stream_.gcount());
	}

	size_t write(const void* src, size_t len) {
		if (readonly_ || !stream_.is_open() || len == 0) {
			return 0;
		}
		stream_.write(static_cast<const char*>(src), static_cast<std::streamsize>(len));
		if (!stream_) {
			throw PlatformException("Can't write file.");
		}
		return len;
	}

	void seek(int64_t off, int whence) {
		if (!stream_.is_open()) {
			return;
		}
		const auto dir = whence == SEEK_SET
			? std::ios::beg
			: whence == SEEK_CUR ? std::ios::cur : std::ios::end;
		stream_.clear();
		stream_.seekg(static_cast<std::streamoff>(off), dir);
		stream_.seekp(static_cast<std::streamoff>(off), dir);
	}

	uint64_t tell() const {
		return static_cast<uint64_t>(stream_.tellg());
	}

	uint64_t size() const {
		if (!stream_.is_open()) {
			return 0;
		}
		const auto current = stream_.tellg();
		stream_.seekg(0, std::ios::end);
		const auto file_size = stream_.tellg();
		stream_.seekg(current, std::ios::beg);
		return static_cast<uint64_t>(file_size);
	}

	void truncate(uint64_t new_size) {
		stream_.flush();
		Fs::resize_file(path_, new_size);
	}

	bool is_open() const {
		return stream_.is_open();
	}

	bool read_only() const {
		return readonly_;
	}

	const Path& path() const {
		return path_;
	}

	void close() {
		if (stream_.is_open()) {
			stream_.close();
		}
	}

private:
	bool readonly_{ true };
	mutable std::fstream stream_;
	Path path_;
};
#endif

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
