#include <errno.h>
#include <iconv.h>
#include <vector>

#include <stdexcept>

#include <base/unique_handle.h>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/text_encoding.h>
#include <base/charset_detector.h>

XAMP_BASE_NAMESPACE_BEGIN

#define USE_LIBICONV 0

namespace {
	static const auto kUTF8Encoding = std::string("UTF-8");

#if USE_LIBICONV
	class LibIconvLib final {
	public:
		LibIconvLib();

		XAMP_DISABLE_COPY(LibIconvLib)
	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(libiconv_open);
		XAMP_DECLARE_DLL_NAME(libiconv_close);
		XAMP_DECLARE_DLL_NAME(libiconv);
	};

	inline LibIconvLib::LibIconvLib() try
		: module_(OpenSharedLibrary("libiconv"))
		, XAMP_LOAD_DLL_API(libiconv_open)
		, XAMP_LOAD_DLL_API(libiconv_close)
		, XAMP_LOAD_DLL_API(libiconv) {
	}
	catch (const Exception& e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

#define LIBICONV_LIB Singleton<LibIconvLib>::GetInstance()

	struct IconvDeleter final {
		static iconv_t invalid() noexcept {
			return (iconv_t)(-1);
		}
		static void close(iconv_t value) {
			LIBICONV_LIB.iconv_close(value);
		}
	};

	using IconvPtr = UniqueHandle<iconv_t, IconvDeleter>;
#else
	static UINT WindowsCodePageFromString(const std::string& encoding) {
		// From source code uchardet/src/nsMBCSGroupProber.cpp
		// Windows code page
		// https://learn.microsoft.com/zh-tw/windows/win32/intl/code-page-identifiers
		static const OrderedMap<std::string_view, UINT> windows_code_page_lut{
			{"utf-8",     65001 },
			{"shift_jis", 932 },
			{"sjis",      932 },
			{"enu-jp",    20932 },
			{"gbk",       936 },
			{"cp936",     936 },
			{"gb18030",   54936 },
			{"big5",      950 },
			{"cp950",     950 },
			{"euc-tw",    51950 },
			{"euc-kr",    51949 },
		};

		auto lower_enc = String::ToLower(encoding);
		auto itr = windows_code_page_lut.find(lower_enc);
		if (itr != windows_code_page_lut.end()) {
			return (*itr).second;
		}

		// Default windows code page (ACP)
		return CP_ACP;
	}

	static std::expected<std::wstring, TextEncodeingError> MultiByteToWide(const std::string& input, UINT codePage, bool ignoreError) {
		if (input.empty()) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_INPUT_STRING_EMPTY);
		}

		// 計算轉換後的 wchar_t 長度
		DWORD flags = 0;
		if (!ignoreError) {
			// 若要在遇到無效字元時產生錯誤，可加此旗標
			// flags = MB_ERR_INVALID_CHARS; 
			// 依實際需求而定
		}

		int wide_size = ::MultiByteToWideChar(
			codePage,
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

		// 真正做轉換
		int result = ::MultiByteToWideChar(
			codePage,
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

	// 封裝：將 wstring -> 多位元字串 (某 code page)
	static std::expected<std::string, TextEncodeingError> WideToMultiByte(const std::wstring& input, UINT codePage, bool ignoreError) {
		if (input.empty()) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_INPUT_STRING_EMPTY);
		}

		DWORD flags = 0;
		// 若要在無法對應的 wchar 時，改成 '?', 可在 WideCharToMultiByte 裡設定 WC_NO_BEST_FIT_CHARS
		// 同樣地, ignoreError = false 時，也可能需要 WC_ERR_INVALID_CHARS
		// 依需求調整

		int mb_size = ::WideCharToMultiByte(
			codePage,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			nullptr,
			0,
			nullptr,
			nullptr
		);

		if (mb_size == 0) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_API_ERROR);
		}

		std::string output;
		output.resize(mb_size);

		int result = ::WideCharToMultiByte(
			codePage,
			flags,
			input.data(),
			static_cast<int>(input.size()),
			&output[0],
			mb_size,
			nullptr,
			nullptr
		);

		if (result == 0) {
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
		UINT from_code_page = WindowsCodePageFromString(input_encoding);
		auto wide = MultiByteToWide(input, from_code_page, ignore_error);
		if (!wide) {
			return std::unexpected(TextEncodeingError::TEXT_ENCODING_TO_WIDE_ERROR);
		}

		UINT to_code_page = WindowsCodePageFromString(output_encoding);
		return WideToMultiByte(wide.value(), to_code_page, ignore_error);
	}

	std::expected<std::string, TextEncodeingError> ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		return ConvertTo8String(input_encoding, input, kUTF8Encoding, buf_size, ignore_error);
	}

	bool IsUtf8(const std::string& input) {		
		const auto encoding_name = detector_.Detect(input);
		if (!encoding_name) {
			return false;
		}
		return encoding_name.value() == kUTF8Encoding;
	}

	std::expected<std::string, TextEncodeingError> ToUtf8String(const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		const auto encoding_name = detector_.Detect(input);
		if (encoding_name) {
			return input;
		}
		if (encoding_name != kUTF8Encoding) {
			return ConvertToUtf8String(encoding_name.value(), input, buf_size, ignore_error);
		}
		return std::unexpected(TextEncodeingError::TEXT_ENCODING_INPUT_STRING_UTF8);
	}

	EncodingDetector detector_;
};

std::expected<std::string, TextEncodeingError> TextEncoding::ToUtf8String(const std::string& input_encoding,
	const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->ConvertToUtf8String(input_encoding,
		input, 
		buf_size, 
		ignore_error);
}

bool TextEncoding::IsUtf8(const std::string& input) {
	return impl_->IsUtf8(input);
}

std::expected<std::string, TextEncodeingError> TextEncoding::ToUtf8String(const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->ToUtf8String(input, buf_size, ignore_error);
}

TextEncoding::TextEncoding()
	: impl_(MakeAlign<TextEncodingImpl>()) {
}

XAMP_PIMPL_IMPL(TextEncoding)

void LoadLibIconvLib() {
#if USE_LIBICONV
	LIBICONV_LIB;
#else
#endif
}

XAMP_BASE_NAMESPACE_END
