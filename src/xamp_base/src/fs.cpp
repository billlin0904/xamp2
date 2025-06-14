#include <base/fs.h>
#include <base/rng.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/logger_impl.h>
#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/charset_detector.h>
#include <base/text_encoding.h>

#ifdef XAMP_OS_WIN
#include <codecvt>
#include <base/windows_handle.h>
#include <winioctl.h>
#else
#include <codecvt>
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

#include <llfio/llfio.hpp>
#include <ntkernel-error-category/ntkernel_category.hpp>

#include <regex>
#include <fstream>

XAMP_BASE_NAMESPACE_BEGIN

namespace llfio = LLFIO_V2_NAMESPACE;

bool IsFilePath(const Path& file_path) noexcept {
	return file_path.has_extension();
}

std::tuple<std::fstream, Path> GetTempFile() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(GetSequentialUUID() + ".tmp");
		std::fstream file(path.native(),
			std::ios::in
			| std::ios::out
			| std::ios::binary
			| std::ios::trunc);
		if (file.is_open()) {
			return std::make_tuple(std::move(file), path);
		}
		XAMP_LOG_DEBUG("{} {}", path, GetLastErrorMessage());
	}
	throw PlatformException("Can't create temp file.");
}

Path GetTempFileNamePath() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(GetSequentialUUID() + ".tmp");
		std::ofstream file(path.native());
		if (file.is_open()) {
			file.close();
			return path;
		}
		XAMP_LOG_DEBUG("{} {}", path, GetLastErrorMessage());
	}
	throw PlatformException("Can't create temp file.");
}

Path GetApplicationFilePath() {
	// https://stackoverflow.com/questions/1528298/get-path-of-executable
#ifdef XAMP_OS_WIN
	wchar_t buffer[MAX_PATH]{};
	::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	return Path(buffer).parent_path();
#else
	char raw_path_name[PATH_MAX]{};
	char real_path_name[PATH_MAX]{};
	uint32_t raw_path_size = (uint32_t)sizeof(raw_path_name);
	if (!::_NSGetExecutablePath(raw_path_name, &raw_path_size)) {
		::realpath(raw_path_name, real_path_name);
	}
	return Path(real_path_name).parent_path();
#endif
}

std::string GetSharedLibraryName(const std::string_view& name) {
	std::string library_name(name);
#ifdef XAMP_OS_WIN
	return library_name + ".dll";
#else
	return "lib" + library_name + ".dylib";
#endif
}

Path GetComponentsFilePath() {
	return GetApplicationFilePath() / Path("components");
}

#ifdef XAMP_OS_WIN
HANDLE GetVolumeHandleForFile(const wchar_t* filePath) {
	wchar_t volume_path[MAX_PATH];
	if (!::GetVolumePathNameW(filePath, volume_path, ARRAYSIZE(volume_path)))
		return nullptr;

	wchar_t volume_name[MAX_PATH];
	if (!::GetVolumeNameForVolumeMountPointW(volume_path,
		volume_name, ARRAYSIZE(volume_name)))
		return nullptr;

	auto length = wcslen(volume_name);
	if (length && volume_name[length - 1] == L'\\')
		volume_name[length - 1] = L'\0';

	return ::CreateFileW(volume_name, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
}

bool IsFileOnSsd(const Path& path) {
	FileHandle volume(GetVolumeHandleForFile(path.wstring().c_str()));
	if (!volume) {
		return false;
	}

	STORAGE_PROPERTY_QUERY query{};
	query.PropertyId = StorageDeviceSeekPenaltyProperty;
	query.QueryType = PropertyStandardQuery;
	DWORD count;
	bool is_ssd{ false };
	DEVICE_SEEK_PENALTY_DESCRIPTOR result{};
	if (::DeviceIoControl(volume.get(), IOCTL_STORAGE_QUERY_PROPERTY,
		&query, sizeof(query), &result, sizeof(result), &count, nullptr)) {
		is_ssd = !result.IncursSeekPenalty;
	}
	return is_ssd;
}

#else
bool IsFileOnSsd(const Path& path) {
	return true;
}
#endif

bool IsCDAFile(Path const& path) {
	return path.extension() == ".cda";
}

class FileCountTraverseVisitor : public llfio::algorithm::traverse_visitor {
	size_t file_count_{ 0 };
	std::vector<std::wregex> regexes_;

	std::vector<std::wregex> MakeRegexFilters(const std::vector<std::wstring> &filters) {
		std::vector<std::wregex> v;
		v.reserve(filters.size());
		for (auto f : filters) {
			std::wstring r;
			r.reserve(f.size() * 2);
			for (char c : f) {
				switch (c) {
				case L'*': r += L".*"; break;
				case L'?': r += L'.';  break;
				case L'.': r += L"\\."; break;
				default: r += c;    break;
				}
			}
			v.emplace_back(r, std::regex::icase);
		}
		return v;
	}
public:
	explicit FileCountTraverseVisitor(const std::vector<std::wstring>& filters) {
		regexes_ = MakeRegexFilters(filters);
	}

	size_t GetFileCount() const {
		return file_count_;
	}

	virtual llfio::result<void> post_enumeration(void*,
		const llfio::directory_handle& dirh, 
		llfio::directory_handle::buffers_type& contents,
		size_t) noexcept override final {
		llfio::file_handle::extent_type bytes = 0;
		
		for (const llfio::directory_entry& entry : contents) {
			const Path leafname(entry.leafname.path());
			auto name = leafname.native();
			for (auto& re : regexes_) {
				if (std::regex_match(name, re)) {
					++file_count_;
					break;
				}
			}
		}
		return llfio::success();
	}
};

size_t GetFileCount(const std::wstring &dir, const std::vector<std::wstring>& filters) {
	FileCountTraverseVisitor visitor(filters);
	llfio::path_view to_traverse_path(dir);
	if (dir.rfind(L".") != std::wstring::npos) {
		return 1;
	}
	auto result = llfio::path_handle::path(to_traverse_path);
	if (!result) {
		throw PlatformException(result.error().value());		
	}
	auto to_traverse = std::move(result.value());
	llfio::algorithm::traverse(to_traverse, &visitor, std::thread::hardware_concurrency()).value();
	return visitor.GetFileCount();
}

std::expected<std::string, TextEncodeingError> ReadFileToUtf8String(const Path& path) {
	std::ifstream file;
	file.open(path, std::ios::binary);

	if (!file.is_open()) {
		return std::unexpected(TextEncodeingError::TEXT_ENCODING_NOT_FOUND_FILE);
	}

	file.seekg(0, std::ios::end);
	auto length = file.tellg();
	file.seekg(0, std::ios::beg);

	if (length <= 0) {
		return std::unexpected(TextEncodeingError::TEXT_ENCODING_EMPTY_FILE);
	}

	std::vector<char> buffer(length);
	file.read(&buffer[0], length);
	std::string input_str(buffer.data(), length);

	TextEncoding encoding;
	return encoding.ToUtf8String(input_str, length, false);
}

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
