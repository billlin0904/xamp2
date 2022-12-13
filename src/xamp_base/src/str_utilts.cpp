#include <base/str_utilts.h>

#include <base/platfrom_handle.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <utf8.h>

#include <array>

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

#ifdef XAMP_OS_WIN
static std::wstring Normalize(std::wstring const& str, NORM_FORM form) {

	if (::IsNormalizedString(form, str.c_str(), -1)) {
		return str;
	}

	// https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Intl/nls--unicode-normalization-sample.md
	auto size_guess = ::NormalizeString(form, 
		str.c_str(), 
		-1,
		nullptr, 
		0);
	if (size_guess == 0) {
		return str;
	}

	while (size_guess > 0) {
		const auto buffer = std::make_unique<wchar_t[]>(size_guess);
		auto actual_size = NormalizeString(form, str.c_str(),
			-1, 
			buffer.get(), size_guess);
		size_guess = 0;
		if (actual_size <= 0 && ::GetLastError() != ERROR_SUCCESS) {
			if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				size_guess = -actual_size;
			}
			else if (::GetLastError() == ERROR_NO_UNICODE_TRANSLATION) {
				return str;
			}
		} else {
			return buffer.get();
		}
	}
	return str;
}

bool IsStringsEqual(std::wstring const& str1, std::wstring const& str2) {
	return Normalize(str1, NormalizationKD)
	== Normalize(str2, NormalizationKD);
}
#endif

std::string FormatBytes(size_t bytes) noexcept {
	static constexpr std::array<std::string_view, 7> kFileSizeUnit {
		" B", " KB", " MB", " GB", " TB", " PB", " EB"
	};
	auto uint = kFileSizeUnit.begin();	
	std::ostringstream ostr;	
	auto num = static_cast<double>(bytes);
	for (; num >= 1024 && uint != kFileSizeUnit.end(); num /= 1024.0, ++uint) {	}
	ostr << std::setprecision(2) << std::fixed << num << *uint;
    return ostr.str();
}

}
