#include <utf8.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

namespace xamp::base::String {

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

std::string FormatBytes(size_t bytes) noexcept {
	static constexpr std::array<std::string_view, 4> uint_base {
		"KB", "MB", "GB", "TB"
	};

	auto uint = uint_base.begin();
	
	std::ostringstream ostr;
	
	auto num = static_cast<double>(bytes);

	if (num < 1024.0) {
		ostr << std::setprecision(2) << num << "B";
	} else {
		while (uint != uint_base.end()) {			
			num /= 1024.0;		
			if (num >= 1024.0) {
				++uint;
			}
			else {
				break;
			}
		}		
		ostr << std::setprecision(2) << std::fixed << num << *uint;
	}
    return ostr.str();
}

}
