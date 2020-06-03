#include <utf8.h>

#include <base/logger.h>
#include <base/str_utilts.h>

namespace xamp::base {

std::wstring ToStdWString(std::string const & utf8) {
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

}
