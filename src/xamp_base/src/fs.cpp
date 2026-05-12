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

#include <regex>
#include <fstream>

XAMP_BASE_NAMESPACE_BEGIN

bool IsFilePath(const Path& file_path) {
	return file_path.has_extension();
}

std::tuple<std::fstream, Path> GetTempFile() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(GetSequentialUUID() + ".tmp");
		std::fstream file_(path.native(),
			std::ios::in
			| std::ios::out
			| std::ios::binary
			| std::ios::trunc);
		if (file_.is_open()) {
			return std::make_tuple(std::move(file_), path);
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
		std::ofstream file_(path.native());
		if (file_.is_open()) {
			file_.close();
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

std::expected<std::string, TextEncodeingError> ReadFileToUtf8String(const Path& path) {
	std::ifstream file_;
	file_.open(path, std::ios::binary);

	if (!file_.is_open()) {
		return std::unexpected(TextEncodeingError::TEXT_ENCODING_NOT_FOUND_FILE);
	}

	file_.seekg(0, std::ios::end);
	auto length = file_.tellg();
	file_.seekg(0, std::ios::beg);

	if (length <= 0) {
		return std::unexpected(TextEncodeingError::TEXT_ENCODING_EMPTY_FILE);
	}

	std::vector<char> buffer(length);
	file_.read(&buffer[0], length);
	std::string input_str(buffer.data(), length);

	TextEncoding encoding;
	return encoding.ToUtf8String(input_str, length, false);
}

std::expected<std::wstring, Errors> NormalizePathToWideString(const Path& path) {
	const auto raw = path.wstring();

	// GetFullPathNameW：先問長度再配置，避免 MAX_PATH 問題
	DWORD needed = ::GetFullPathNameW(raw.c_str(), 0, nullptr, nullptr);
	if (needed == 0) return std::unexpected(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR);

	std::wstring full;
	full.resize(needed);
	DWORD written = ::GetFullPathNameW(raw.c_str(), needed, full.data(), nullptr);
	if (written == 0 || written >= needed) 
		return std::unexpected(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR);
	full.resize(written);

	// normalize：去尾端 slash + 小寫化（用來去重）
	while (!full.empty() && (full.back() == L'\\' || full.back() == L'/')) {
		full.pop_back();
	}
	std::transform(full.begin(), full.end(), full.begin(),
		[](wchar_t c) { return (wchar_t)::towlower(c); });
	return full;
}

XAMP_BASE_NAMESPACE_END
