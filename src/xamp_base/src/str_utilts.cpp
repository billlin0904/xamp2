#include <base/str_utilts.h>

#include <base/platfrom_handle.h>
#include <base/text_encoding.h>
#include <base/logger.h>

#include <utf8.h>

#include <array>

XAMP_BASE_NAMESPACE_BEGIN

namespace String {

std::wstring toStdWString(const std::string & utf8) {
	std::wstring utf16;
	try {
		utf16.reserve(utf8.length());
		utf8::utf8to16(utf8.begin(), utf8.end(), std::back_inserter(utf16));
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
		utf8.reserve(utf16.length());
		utf8::utf16to8(utf16.begin(), utf16.end(), std::back_inserter(utf8));
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
