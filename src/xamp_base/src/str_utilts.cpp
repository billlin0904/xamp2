#include <utf8.h>

#include <base/str_utilts.h>

namespace xamp::base {

std::wstring ToStdWString(const std::string& utf8) {
	std::wstring utf16;
	try {
		utf8::utf8to16(utf8.begin(), utf8.end(), std::back_inserter(utf16));
	}
	catch (const std::exception & e) {
	}	
	return utf16;
}

std::string ToUtf8String(const std::wstring& utf16) {
	std::string utf8;
	try {
		utf8::utf16to8(utf16.begin(), utf16.end(), std::back_inserter(utf8));
	}
	catch (const std::exception & e) {
	}
	return utf8;
}

}