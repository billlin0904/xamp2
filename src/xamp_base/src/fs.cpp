#include <base/fs.h>
#include <base/rng.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/logger.h>
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
#include <unistd.h>
#endif

#ifdef XAMP_OS_MAC
#include <mach-o/dyld.h>
#endif

#include <regex>
#include <fstream>
#include <cwctype>
#include <cctype>

XAMP_BASE_NAMESPACE_BEGIN

namespace {

std::string PathToLogString(const Path& path) {
#ifdef XAMP_OS_WIN
	return String::toUtf8String(path.wstring());
#else
	return path.string();
#endif
}

} // namespace

bool isFilePath(const Path& file_path) {
	return file_path.has_extension();
}

std::tuple<std::fstream, Path> getTempFile() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(getSequentialUuid() + ".tmp");
		std::fstream file_(path.native(),
			std::ios::in
			| std::ios::out
			| std::ios::binary
			| std::ios::trunc);
		if (file_.is_open()) {
			return std::make_tuple(std::move(file_), path);
		}
		XAMP_LOG_DEBUG("{} {}", PathToLogString(path), GetLastErrorMessage());
	}
	throw PlatformException("Can't create temp file.");
}

Path getTempFileNamePath() {
	// Short retry times to avoid cost too much time.
	constexpr auto kMaxRetryCreateTempFile = 128;
	const auto temp_path = Fs::temp_directory_path();

	for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
		auto path = temp_path / Fs::path(getSequentialUuid() + ".tmp");
		std::ofstream file_(path.native());
		if (file_.is_open()) {
			file_.close();
			return path;
		}
		XAMP_LOG_DEBUG("{} {}", PathToLogString(path), GetLastErrorMessage());
	}
	throw PlatformException("Can't create temp file.");
}

Path getApplicationFilePath() {
	// https://stackoverflow.com/questions/1528298/get-path-of-executable
#ifdef XAMP_OS_WIN
	wchar_t buffer[MAX_PATH]{};
	::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	return Path(buffer).parent_path();
#elif defined(XAMP_OS_MAC)
	char raw_path_name[PATH_MAX]{};
	char real_path_name[PATH_MAX]{};
	uint32_t raw_path_size = (uint32_t)sizeof(raw_path_name);
	if (!::_NSGetExecutablePath(raw_path_name, &raw_path_size)) {
		::realpath(raw_path_name, real_path_name);
	}
	return Path(real_path_name).parent_path();
#elif defined(XAMP_OS_LINUX)
	char raw_path_name[PATH_MAX]{};
	const auto length = ::readlink("/proc/self/exe", raw_path_name, sizeof(raw_path_name) - 1);
	if (length <= 0) {
		return Fs::current_path();
	}
	raw_path_name[length] = '\0';
	return Path(raw_path_name).parent_path();
#else
	return Fs::current_path();
#endif
}

std::string getSharedLibraryName(const std::string_view& name) {
	std::string library_name(name);
#ifdef XAMP_OS_WIN
	return library_name + ".dll";
#elif defined(XAMP_OS_MAC)
	const std::string prefix = library_name.starts_with("lib") ? "" : "lib";
	return prefix + library_name + ".dylib";
#else
	const std::string prefix = library_name.starts_with("lib") ? "" : "lib";
	const auto dash_pos = library_name.find_last_of('-');
	if (dash_pos != std::string::npos && dash_pos + 1 < library_name.size()) {
		const auto version = library_name.substr(dash_pos + 1);
		const auto is_version = std::all_of(version.begin(), version.end(), [](unsigned char ch) {
			return std::isdigit(ch) != 0;
			});
		if (is_version) {
			library_name.replace(dash_pos, 1, ".so.");
			return prefix + library_name;
		}
	}
	return prefix + library_name + ".so";
#endif
}

Path getComponentsFilePath() {
	return getApplicationFilePath() / Path("components");
}

bool IsCDAFile(Path const& path) {
	return path.extension() == ".cda";
}

std::expected<std::string, TextEncodeingError> readFileToUtf8String(const Path& path) {
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
	return encoding.toUtf8String(input_str, length, false);
}

std::expected<std::wstring, Errors> normalizePathToWideString(const Path& path) {
#ifdef XAMP_OS_WIN
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
#else
	auto normalized = path.lexically_normal().wstring();
	while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/')) {
		normalized.pop_back();
	}
	std::transform(normalized.begin(), normalized.end(), normalized.begin(),
		[](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
	return normalized;
#endif
}

XAMP_BASE_NAMESPACE_END
