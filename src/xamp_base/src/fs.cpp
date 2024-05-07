#include <base/fs.h>
#include <base/rng.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/exception.h>
#include <base/logger_impl.h>
#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/charset_detector.h>

#include <errno.h>
#include <iconv.h>

#ifdef XAMP_OS_WIN
#include <codecvt>
#include <base/windows_handle.h>
#else
#include <codecvt>
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

#include <fstream>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	template <typename T>
	struct IconvDeleter;

	template <>
	struct IconvDeleter<void> {
		void operator()(void* p) const {
			::iconv_close(p);
		}
	};

	using IconvPtr = std::unique_ptr<void, IconvDeleter<void>>;

	std::string ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size = 4096,
		bool ignore_error = false) {
		IconvPtr handle(::iconv_open("UTF-8", input_encoding.c_str()));
		if (handle.get() == (iconv_t)(-1)) {
			throw std::runtime_error("unknown error");
		}

		std::string output;

		auto check_convert_error = []() {
			switch (errno) {
			case EILSEQ:
			case EINVAL:
				throw std::runtime_error("invalid multibyte chars");
			default:
				throw std::runtime_error("unknown error");
			}
			};

		// copy the string to a buffer as iconv function requires a non-const char
		// pointer.
		std::vector<char> in_buf(input.begin(), input.end());
		char* src_ptr = &in_buf[0];
		size_t src_size = input.size();

		std::vector<char> buf(buf_size);
		std::string dst;

		while (0 < src_size) {
			char* dst_ptr = &buf[0];
			size_t dst_size = buf.size();
			size_t res = ::iconv(handle.get(), &src_ptr, &src_size, &dst_ptr, &dst_size);
			if (res == (size_t)-1) {
				if (errno == E2BIG) {
					// ignore this error
				}
				else if (ignore_error) {
					// skip character
					++src_ptr;
					--src_size;
				}
				else {
					check_convert_error();
				}
			}
			dst.append(&buf[0], buf.size() - dst_size);
		}
		dst.swap(output);
		return output;
	}
}

bool IsFilePath(const Path& file_path) noexcept {
	return file_path.has_extension();
}

Path GetTempFileNamePath() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 10;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(MakeTempFileName() + ".tmp");
		std::ofstream file(path.native());
		if (file.is_open()) {
			file.close();
			return path;
		}
		XAMP_LOG_DEBUG("{} {}", path, GetLastErrorMessage());
	}
	throw PlatformException("Can't create temp file.");
}

std::string MakeTempFileName() {
	char buffer[MAX_PATH]{};
#ifdef XAMP_OS_WIN
	::GetTempFileNameA(buffer, "xamp", 0, buffer);
#else
#endif
	return buffer;
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

int64_t GetLastWriteTime(const Path& path) {
#ifdef XAMP_OS_WIN
	const auto file_path = path.wstring();

	FileHandle file(::CreateFileW(file_path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr));
	if (!file) {
		throw PlatformException();
	}

	SYSTEMTIME st_utc, st_local;
	FILETIME ft_create, ft_access, ft_write;
	if (!::GetFileTime(file.get(), &ft_create, &ft_access, &ft_write)) {
		throw PlatformException();
	}

	if (!::FileTimeToSystemTime(&ft_write, &st_utc)) {
		throw PlatformException();
	}

	if (!::SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st_local)) {
		throw PlatformException();
	}

	std::tm tm;
	tm.tm_sec = st_local.wSecond;
	tm.tm_min = st_local.wMinute;
	tm.tm_hour = st_local.wHour;
	tm.tm_mday = st_local.wDay;
	tm.tm_mon = st_local.wMonth - 1;
	tm.tm_year = st_local.wYear - 1900;
	tm.tm_isdst = -1;
	return std::mktime(&tm);
#else
	//return ToTime_t(Fs::last_write_time(path));
	return 0;
#endif
}

Path GetComponentsFilePath() {
	return GetApplicationFilePath() / Path("components");
}

bool IsCDAFile(Path const& path) {
	return path.extension() == ".cda";
}

std::string ReadFileToUtf8String(const Path& path) {
	std::ifstream file;
	file.open(path, std::ios::binary);

	if (!file.is_open()) {
		throw FileNotFoundException();
	}

	file.seekg(0, std::ios::end);
	auto length = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(length);
	file.read(&buffer[0], length);
	std::string input_str(buffer.data(), length);

	CharsetDetector detector;
	const auto encoding = detector.Detect(input_str.data(), input_str.length());
	if (encoding.empty()) {
		return input_str;
	}
	return ConvertToUtf8String(encoding, input_str, length);
}

XAMP_BASE_NAMESPACE_END
