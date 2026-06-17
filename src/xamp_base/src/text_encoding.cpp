#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/text_encoding.h>
#include <base/charset_detector.h>

#ifdef XAMP_OS_WIN
#include <Windows.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	const auto kUTF8Encoding = std::string("UTF-8");

	bool IsUtf8Encoding(std::string_view encoding) {
		const auto lower_encoding = String::ToLower(std::string(encoding));
		return lower_encoding == "utf-8"
			|| lower_encoding == "utf8"
			|| lower_encoding == "us-ascii"
			|| lower_encoding == "ascii";
	}

#ifdef XAMP_OS_WIN
	std::expected<uint32_t, TextEncodeingError> WindowsCodePageFromString(const std::string& encoding) {
		// From source code uchardet/src/nsMBCSGroupProber.cpp
		// Windows code page
		// https://learn.microsoft.com/zh-tw/windows/win32/intl/code-page-identifiers
		static const OrderedMap<std::string_view, uint32_t> windows_code_page_lut{
			{"utf-8",        CP_UTF8 },
			{"utf8",         CP_UTF8 },
			{"cp65001",      CP_UTF8 },
			{"windows-65001", CP_UTF8 },
			{"us-ascii",     CP_UTF8 },
			{"ascii",        CP_UTF8 },
			{"acp",          CP_ACP },
			{"ansi",         CP_ACP },
			{"locale",       CP_ACP },
			{"system",       CP_ACP },

			{"shift_jis",    932 },
			{"shift-jis",    932 },
			{"sjis",         932 },
			{"ms932",        932 },
			{"cp932",        932 },
			{"windows-31j",  932 },
			{"euc-jp",       51932 },
			{"eucjp",        51932 },
			{"x-euc-jp",     51932 },
			{"cp51932",      51932 },
			{"cp20932",      20932 },

			{"gb2312",       936 },
			{"gbk",          936 },
			{"cp936",        936 },
			{"windows-936",  936 },
			{"gb18030",      54936 },
			{"cp54936",      54936 },
			{"big5",         950 },
			{"big5-hkscs",   950 },
			{"cp950",        950 },
			{"windows-950",  950 },
			{"euc-tw",       51950 },

			{"euc-kr",       51949 },
			{"ks_c_5601-1987", 949 },
			{"cp949",        949 },
			{"windows-949",  949 },

			{"windows-1250", 1250 },
			{"cp1250",       1250 },
			{"windows-1251", 1251 },
			{"cp1251",       1251 },
			{"windows-1252", 1252 },
			{"cp1252",       1252 },
			{"windows-1253", 1253 },
			{"cp1253",       1253 },
			{"windows-1254", 1254 },
			{"cp1254",       1254 },
			{"windows-1255", 1255 },
			{"cp1255",       1255 },
			{"windows-1256", 1256 },
			{"cp1256",       1256 },
			{"windows-1257", 1257 },
			{"cp1257",       1257 },
			{"windows-1258", 1258 },
			{"cp1258",       1258 },

			{"iso-8859-1",   28591 },
			{"latin1",       28591 },
			{"iso-8859-2",   28592 },
			{"iso-8859-5",   28595 },
			{"iso-8859-7",   28597 },
			{"iso-8859-9",   28599 },
			{"iso-8859-15",  28605 },
		};

		const auto lower_enc = String::ToLower(encoding);
		const auto itr = windows_code_page_lut.find(lower_enc);
		if (itr != windows_code_page_lut.end()) {
			return (*itr).second;
		}

		return std::unexpected(TextEncodeingError::TEXT_ENCODING_UNKNOWN_ENCDOING);
	}

	DWORD MultiByteToWideFlags(UINT code_page, bool ignore_error) {
		if (ignore_error) {
			return 0;
		}
		return MB_ERR_INVALID_CHARS;
	}

	DWORD WideToMultiByteFlags(UINT code_page, bool ignore_error) {
		if (ignore_error) {
			return 0;
		}
		if (code_page == CP_UTF8 || code_page == 54936) {
			return WC_ERR_INVALID_CHARS;
		}
		return WC_NO_BEST_FIT_CHARS;
	}

	std::expected<std::wstring, TextEncodeingError> MultiByteToWide(const std::string& input, UINT code_page, bool ignore_error) {
		if (input.empty()) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_INPUT_STRING_EMPTY);
		}

		const auto flags = MultiByteToWideFlags(code_page, ignore_error);

		int wide_size = ::MultiByteToWideChar(
			code_page,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			nullptr,
			0
		);

		if (wide_size == 0) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
		}

		std::wstring output;
		output.resize(wide_size);

		int result = ::MultiByteToWideChar(
			code_page,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			&output[0],
			wide_size
		);

		if (result == 0) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
		}

		return output;
	}

	std::expected<std::string, TextEncodeingError> WideToMultiByte(const std::wstring& input, UINT code_page, bool ignore_error) {
		if (input.empty()) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_INPUT_STRING_EMPTY);
		}

		const auto flags = WideToMultiByteFlags(code_page, ignore_error);
		BOOL used_default_char = FALSE;
		const auto can_use_default_char = code_page != CP_UTF8 && code_page != 54936;
		auto* used_default_char_ptr = !ignore_error && can_use_default_char ? &used_default_char : nullptr;

		int mb_size = ::WideCharToMultiByte(
			code_page,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			nullptr,
			0,
			nullptr,
			used_default_char_ptr
		);

		if (mb_size == 0 || used_default_char) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
		}

		std::string output;
		output.resize(mb_size);
		used_default_char = FALSE;

		int result = ::WideCharToMultiByte(
			code_page,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			output.data(),
			mb_size,
			nullptr,
			used_default_char_ptr
		);

		if (result == 0 || used_default_char) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
		}

		return output;
	}
#endif
}

class TextEncoding::TextEncodingImpl {
public:
	TextEncodingImpl() = default;

	std::expected<std::string, TextEncodeingError> ConvertTo8String(
		const std::string& input_encoding,
		const std::string& input,
		const std::string& output_encoding,
		size_t buf_size,
		bool ignore_error) {
#ifdef XAMP_OS_WIN
		auto from_code_page = WindowsCodePageFromString(input_encoding);
		if (!from_code_page) {
			return std::unexpected(from_code_page.error());
		}

		auto wide = MultiByteToWide(input, from_code_page.value(), ignore_error);
		if (!wide) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_TO_WIDE_ERROR);
		}

		auto to_code_page = WindowsCodePageFromString(output_encoding);
		if (!to_code_page) {
			return std::unexpected(to_code_page.error());
		}

		return WideToMultiByte(wide.value(), to_code_page.value(), ignore_error);
#else
		(void)output_encoding;
		(void)buf_size;
		(void)ignore_error;
		if (IsUtf8Encoding(input_encoding)) {
			return input;
		}
		return input;
#endif
	}

	std::expected<std::string, TextEncodeingError> ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		return ConvertTo8String(input_encoding, input, kUTF8Encoding, buf_size, ignore_error);
	}

	bool isUtf8(const std::string& input) {		
		const auto encoding_name = detector_.detect(input);
		if (!encoding_name) {
			return false;
		}
		return IsUtf8Encoding(encoding_name.value());
	}

	std::expected<std::string, TextEncodeingError> toUtf8String(const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		const auto encoding_name = detector_.detect(input);
		if (!encoding_name) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_DETECT_ERROR);
		}

		auto detected_encoding = encoding_name.value();
		if (detected_encoding.empty()) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_UNKNOWN_ENCDOING);
		}

		if (IsUtf8Encoding(detected_encoding)) {
#ifdef XAMP_OS_WIN
			if (!ignore_error && !MultiByteToWide(input, CP_UTF8, false)) {
				return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
			}
#else
			(void)ignore_error;
#endif
			return input;
		}
		return ConvertToUtf8String(detected_encoding, input, buf_size, ignore_error);
	}

	EncodingDetector detector_;
};

std::expected<std::string, TextEncodeingError> TextEncoding::toUtf8String(const std::string& input_encoding,
	const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->ConvertToUtf8String(input_encoding,
		input, 
		buf_size, 
		ignore_error);
}

bool TextEncoding::isUtf8(const std::string& input) {
	return impl_->isUtf8(input);
}

std::expected<std::string, TextEncodeingError> TextEncoding::toUtf8String(const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->toUtf8String(input, buf_size, ignore_error);
}

TextEncoding::TextEncoding()
	: impl_(makeAlign<TextEncodingImpl>()) {
}

XAMP_PIMPL_IMPL(TextEncoding)

XAMP_BASE_NAMESPACE_END
