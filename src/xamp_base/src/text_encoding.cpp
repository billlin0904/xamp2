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
	// 簡單做個大小寫不敏感的比對
	auto lower_enc = String::ToLower(encoding);

	if (lower_enc == "utf-8") {
		return CP_UTF8;
	}
	else if (lower_enc == "shift_jis" || lower_enc == "sjis") {
		return 932; // Shift-JIS
	}
	else if (lower_enc == "gbk" || lower_enc == "cp936") {
		return 936;
	}
	else if (lower_enc == "big5" || lower_enc == "cp950") {
		return 950;
	}
	// 預設採用系統 ACP
	return CP_ACP;
}

static std::wstring MultiByteToWide(const std::string& input, UINT codePage, bool ignoreError) {
	if (input.empty()) {
		return std::wstring();
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
		// 轉換失敗
		DWORD err = ::GetLastError();
		// 依需求處理：若 ignoreError=true，可嘗試替換或略過；此處直接拋例外
		throw std::runtime_error("MultiByteToWideChar failed with error: " + std::to_string(err));
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
		DWORD err = ::GetLastError();
		throw std::runtime_error("MultiByteToWideChar failed (2nd pass) with error: " + std::to_string(err));
	}

	return output;
}

// 封裝：將 wstring -> 多位元字串 (某 code page)
static std::string WideToMultiByte(const std::wstring& input, UINT codePage, bool ignoreError) {
	if (input.empty()) {
		return std::string();
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
		DWORD err = ::GetLastError();
		throw std::runtime_error("WideCharToMultiByte failed with error: " + std::to_string(err));
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
		DWORD err = ::GetLastError();
		throw std::runtime_error("WideCharToMultiByte failed (2nd pass) with error: " + std::to_string(err));
	}

	return output;
}
#endif

class TextEncoding::TextEncodingImpl {
public:
	TextEncodingImpl() = default;

#if USE_LIBICONV
	std::string ConvertTo8String(
		const std::string& input_encoding,
		const std::string& input,
		const std::string& toencode,
		size_t buf_size,
		bool ignore_error) {
		auto check_convert_error = [](auto check_error) {			
			switch (check_error) {
			case EILSEQ:
			case EINVAL:
				throw std::runtime_error("invalid multibyte chars");
			default:
				throw std::runtime_error("unknown error");
			}
			};

		IconvPtr handle(LIBICONV_LIB.libiconv_open(toencode.c_str(),
			input_encoding.c_str()));
		if (!handle) {
			auto check_error = errno;
			check_convert_error(check_error);
		}

		std::string output;

		// copy the string to a buffer as iconv function requires a non-const char
		// pointer.
		std::vector<char> in_buf(input.begin(), input.end());

		char* src_ptr = &in_buf[0];
		size_t src_size = input.size();

		std::vector<char> buf(buf_size);
		std::string dst;

		while (0 < src_size) {
			// reset the buffer
			char* dst_ptr = &buf[0];
			size_t dst_size = buf.size();

			// convert the string
			size_t res = LIBICONV_LIB.libiconv(handle.get(),
				&src_ptr,
				&src_size,
				&dst_ptr,
				&dst_size);
			auto check_error = errno;
			if (res == (size_t)-1) {
				if (errno == E2BIG) {
					// ignore this error
				}
				else if (ignore_error) {
					// skip character
					++src_ptr;
					--src_size;
				}
				else {
					check_convert_error(check_error);
				}
			}

			// append the converted string
			dst.append(&buf[0], buf.size() - dst_size);
		}

		// swap the result
		dst.swap(output);
		return output;
	}

	std::string ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		return ConvertTo8String(input_encoding,
			input, 
			"UTF-8", 
			buf_size,
			ignore_error);
	}
#else
	std::string ConvertTo8String(
		const std::string& input_encoding,
		const std::string& input,
		const std::string& toencode,
		size_t buf_size,
		bool ignore_error) {
		UINT fromCP = WindowsCodePageFromString(input_encoding);
		std::wstring wide = MultiByteToWide(input, fromCP, ignore_error);

		UINT toCP = WindowsCodePageFromString(toencode);
		std::string output = WideToMultiByte(wide, toCP, ignore_error);
		return output;
	}

	std::string ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		return ConvertTo8String(input_encoding, input, "UTF-8", buf_size, ignore_error);
	}
#endif
};

std::string TextEncoding::ToUtf8String(const std::string& input_encoding,
	const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->ConvertToUtf8String(input_encoding,
		input, 
		buf_size, 
		ignore_error);
}

bool TextEncoding::IsUtf8(const std::string& input) {
	EncodingDetector detector;
	const auto encoding_name = detector.Detect(input);
	if (encoding_name.empty()) {
		return false;
	}
	return encoding_name == "UTF-8";
}

std::string TextEncoding::ToUtf8String(const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	EncodingDetector detector;
	const auto encoding_name = detector.Detect(input);
	if (encoding_name.empty()) {
		throw std::runtime_error("Detect unknown encoding");
	}
	if (encoding_name != "UTF-8") {
		return ToUtf8String(encoding_name, input, buf_size, ignore_error);		
	}
	return input;
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
