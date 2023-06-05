#include <base/fs.h>
#include <base/rng.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/exception.h>
#include <base/stl.h>
#include <base/str_utilts.h>

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

bool IsFilePath(std::wstring const& file_path) noexcept {
	const auto lowcase_file_path = String::ToLower(file_path);
	return lowcase_file_path.find(L"https") == std::string::npos
		|| lowcase_file_path.find(L"http") == std::string::npos;
}

Path GetTempFilePath() {
	// Short retry times to avoid cost too much time.
 	constexpr auto kMaxRetryCreateTempFile = 10;
	const auto temp_path = Fs::temp_directory_path();
	
	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(MakeTempFileName() + ".tmp");
		std::error_code ec;
		// Create parent directory if not exist.
		if (Fs::create_directories(path.parent_path(), ec) 
			&& Fs::status(path.parent_path(), ec).type() == Fs::file_type::directory) {
			// Create temp file.
			std::ofstream file(path.native());
			if (file.is_open()) {
				file.close();
				return path;
			}
		}
	}
	throw PlatformException("Can't create temp file.");
}

std::string MakeTempFileName() {
	return MakeUuidString();
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
    if(!::_NSGetExecutablePath(raw_path_name, &raw_path_size)) {
        ::realpath(raw_path_name, real_path_name);
    }
    return Path(real_path_name).parent_path();
#endif
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
    return ToTime_t(Fs::last_write_time(path));
#endif
}

Path GetComponentsFilePath() {
	return GetApplicationFilePath() / Path("components");
}

bool IsCDAFile(Path const& path) {
	return path.extension() == ".cda";
}

bool TryImbue(std::wifstream& file, std::string_view name) {
	try {
		(void)file.imbue(std::locale(name.data()));
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

void ImbueFileFromBom(std::wifstream& file) {
#ifdef XAMP_OS_WIN
	static const Vector<std::string_view> locale_names{
		"en_US.UTF-8",
		"zh_TW.UTF-8",
		"zh_CN.UTF-8",
		"ja_JP.SJIS",
	};

	std::wstring bom;

	if (std::getline(file, bom, L'\r')) {
		// UTF-16 BOM
		if (bom.size() > 2 && bom[0] == 0xFEFF) {
			file.imbue(std::locale(std::locale(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>()));
			file.seekg(2, std::ios_base::beg);
			return;
		}
		// UTF-8 BOM
		if (bom.size() > 3 && bom.substr(0, 3) == L"\xEF\xBB\xBF") {
			file.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::little_endian>()));
			file.seekg(3, std::ios_base::beg);
			return;
		}
		// Other BOM
		for (auto locale_name : locale_names) {			
			if (TryImbue(file, locale_name)) {
				file.seekg(0, std::ios_base::beg);
				break;
			}
		}
	}
#else
	std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>());
	file.imbue(utf8_locale);
#endif
}

XAMP_BASE_NAMESPACE_END
