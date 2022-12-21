#include <base/fs.h>
#include <base/rng.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/exception.h>
#include <base/stl.h>
#include <base/str_utilts.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#endif

#include <fstream>

namespace xamp::base {

bool IsFilePath(std::wstring const& file_path) noexcept {
	const auto lowcase_file_path = String::ToLower(file_path);
	return lowcase_file_path.find(L"https") == std::string::npos
		|| lowcase_file_path.find(L"http") == std::string::npos;
}

 Path GetTempFilePath() {
 	constexpr auto kMaxRetryCreateTempFile = 100;
	const auto temp_path = Fs::temp_directory_path();
	
	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(MakeTempFileName() + ".tmp");
		std::ofstream file(path.native());
		if (file.is_open()) {
			file.close();
			return path;
		}
	}
	throw PlatformSpecException("Can't create temp file.");
}

std::string MakeTempFileName() {
	return MakeUuidString();
}

std::string GetSharedLibraryName(const std::string_view &name) {
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
		throw PlatformSpecException();
	}

	SYSTEMTIME st_utc, st_local;
	FILETIME ft_create, ft_access, ft_write;
	if (!::GetFileTime(file.get(), &ft_create, &ft_access, &ft_write)) {
		throw PlatformSpecException();
	}

	if (!::FileTimeToSystemTime(&ft_write, &st_utc)) {
		throw PlatformSpecException();
	}

	if (!::SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st_local)) {
		throw PlatformSpecException();
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
	return GetTime_t();
#endif
}

}
