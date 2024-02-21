#include <base/str_utilts.h>

#include <base/platfrom_handle.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <utf8.h>

#include <array>

namespace xamp::base::String {

std::wstring ToStdWString(const  std::string & utf8) {
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

std::string LocaleStringToUTF8(const std::string& str) {
#ifdef XAMP_OS_WIN
	std::vector<wchar_t> buf(str.length() + 1);
	::MultiByteToWideChar(CP_ACP,
		0,
		str.c_str(),
		-1,
		buf.data(),
		static_cast<int>(str.length()));
	return String::ToUtf8String(buf.data());
#else
	return str;
#endif
}

std::string ToUtf8String(std::wstring const & utf16) {
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

std::string FormatBytes(size_t bytes) {
	static constexpr std::array<std::string_view, 7> kFileSizeUnit {
		" B", " KB", " MB", " GB", " TB", " PB", " EB"
	};
	auto uint = kFileSizeUnit.begin();	
	auto num = static_cast<double>(bytes);
	for (; num >= 1024 && uint != kFileSizeUnit.end(); num /= 1024.0, ++uint) {	}
	return Format("{:.2f}{}", num, *uint);
}

}
