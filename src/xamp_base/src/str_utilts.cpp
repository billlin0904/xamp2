#include <base/str_utilts.h>

#include <base/platfrom_handle.h>
#include <base/text_encoding.h>
#include <base/logger.h>

#include <simdutf.h>

#include <array>
#include <limits>

XAMP_BASE_NAMESPACE_BEGIN

namespace String {

std::wstring toStdWString(const std::string & utf8) {
	std::wstring utf16;
	try {
		if constexpr (sizeof(wchar_t) == sizeof(char16_t)) {
			utf16.resize(utf8.size());
			const auto result = simdutf::convert_utf8_to_utf16_with_errors(
				utf8.data(),
				utf8.size(),
				reinterpret_cast<char16_t*>(utf16.data()));
			if (result.error != simdutf::SUCCESS) {
				XAMP_LOG_DEBUG("simdutf convert_utf8_to_utf16 failed: {}, {}",
					static_cast<int>(result.error),
					result.count);
				return {};
			}
			utf16.resize(result.count);
		}
		else if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
			utf16.resize(utf8.size());
			const auto result = simdutf::convert_utf8_to_utf32_with_errors(
				utf8.data(),
				utf8.size(),
				reinterpret_cast<char32_t*>(utf16.data()));
			if (result.error != simdutf::SUCCESS) {
				XAMP_LOG_DEBUG("simdutf convert_utf8_to_utf32 failed: {}, {}",
					static_cast<int>(result.error),
					result.count);
				return {};
			}
			utf16.resize(result.count);
		}
		else {
			static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t),
				"Unsupported wchar_t size");
		}
	}
	catch (const std::exception & e) {
        XAMP_LOG_DEBUG("{}", e.what());
	}	
	return utf16;
}

std::string localeStringToUTF8(const std::string& str) noexcept {
#ifdef XAMP_OS_WIN
	try {
		auto utf8 = TextEncoding().toUtf8String("acp", str, str.size(), true);
		if (utf8) {
			return utf8.value();
		}
	}
	catch (...) {
	}
#else
	return str;
#endif
	return str;
}

std::string toUtf8String(std::wstring const & utf16) {
	std::string utf8;
	try {
		if constexpr (sizeof(wchar_t) == sizeof(char16_t)) {
			if (utf16.size() > std::numeric_limits<size_t>::max() / 3) {
				return {};
			}
			utf8.resize(utf16.size() * 3);
			const auto result = simdutf::convert_utf16_to_utf8_with_errors(
				reinterpret_cast<const char16_t*>(utf16.data()),
				utf16.size(),
				utf8.data());
			if (result.error != simdutf::SUCCESS) {
				XAMP_LOG_DEBUG("simdutf convert_utf16_to_utf8 failed: {}, {}",
					static_cast<int>(result.error),
					result.count);
				return {};
			}
			utf8.resize(result.count);
		}
		else if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
			if (utf16.size() > std::numeric_limits<size_t>::max() / 4) {
				return {};
			}
			utf8.resize(utf16.size() * 4);
			const auto result = simdutf::convert_utf32_to_utf8_with_errors(
				reinterpret_cast<const char32_t*>(utf16.data()),
				utf16.size(),
				utf8.data());
			if (result.error != simdutf::SUCCESS) {
				XAMP_LOG_DEBUG("simdutf convert_utf32_to_utf8 failed: {}, {}",
					static_cast<int>(result.error),
					result.count);
				return {};
			}
			utf8.resize(result.count);
		}
		else {
			static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t),
				"Unsupported wchar_t size");
		}
	}
	catch (const std::exception & e) {
        XAMP_LOG_DEBUG("{}", e.what());
	}
	return utf8;
}

std::string formatBytes(size_t bytes) {
	static constexpr std::array<std::string_view, 7> kFileSizeUnit {
		" B", " KB", " MB", " GB", " TB", " PB", " EB"
	};
	auto uint = kFileSizeUnit.begin();	
	auto num = static_cast<double>(bytes);
	for (; num >= 1024 && uint != kFileSizeUnit.end(); num /= 1024.0, ++uint) {	}
	return String::format("{:.2f}{}", num, *uint);
}

}

XAMP_BASE_NAMESPACE_END
