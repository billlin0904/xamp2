#include <base/windows_handle.h>
#include <base/unicode.h>

namespace xamp::base {

std::wstring ToStdWString(const std::string &utf8) {
    if (utf8.empty()) {
        return L"";
    }
    auto len = utf8.length() + 1;
    std::wstring ret(len, 0);
#ifdef _WIN32
    auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &utf8[0], utf8.size(), &ret[0], len);
    ret.resize(size);
#else
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t retval = _mbstowcs_s_l(&size, &ret[0], len, &str[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
#endif
    return ret;
}

std::string ToUtf8String(const std::wstring &utf16) {
    if (utf16.empty()) {
        return "";
    }
#ifdef _WIN32
	const UINT UTF8_CODE_PAGE = 65001;
    auto size = WideCharToMultiByte(UTF8_CODE_PAGE, WC_ERR_INVALID_CHARS, &utf16[0], utf16.size(), nullptr, 0, nullptr, nullptr);
    std::string ret(size, 0);
    WideCharToMultiByte(UTF8_CODE_PAGE, WC_ERR_INVALID_CHARS, &utf16[0], utf16.size(), &ret[0], size, nullptr, nullptr);
#else
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t err = _wcstombs_s_l(&size, NULL, 0, &wstr[0], _TRUNCATE, lc);
    std::string ret = std::string(size, 0);
    err = _wcstombs_s_l(&size, &ret[0], size, &wstr[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
#endif
    return ret;
}

}
