#include <errno.h>
#include <iconv.h>
#include <vector>

#include <stdexcept>
#include <base/text_encoding.h>
#include <base/charset_detector.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
struct IconvDeleter;

template <>
struct IconvDeleter<void> {
	void operator()(void* p) const {
		::iconv_close(p);
	}
};

using IconvPtr = std::unique_ptr<void, IconvDeleter<void>>;

class TextEncoding::TextEncodingImpl {
public:
	TextEncodingImpl() = default;

	std::string ConvertToUtf8String(const std::string& input_encoding,
		const std::string& input,
		size_t buf_size,
		bool ignore_error) {
		IconvPtr handle(::iconv_open("UTF-8", input_encoding.c_str()));
		if (handle.get() == (iconv_t)(-1)) {
			throw std::runtime_error("unknown error");
		}

		std::string output;

		auto check_convert_error = []() {
			switch (errno) {
			case EILSEQ:
			case EINVAL:
				throw std::runtime_error("invalid multibyte chars");
			default:
				throw std::runtime_error("unknown error");
			}
			};

		// copy the string to a buffer as iconv function requires a non-const char
		// pointer.
		std::vector<char> in_buf(input.begin(), input.end());
		char* src_ptr = &in_buf[0];
		size_t src_size = input.size();

		std::vector<char> buf(buf_size);
		std::string dst;

		while (0 < src_size) {
			char* dst_ptr = &buf[0];
			size_t dst_size = buf.size();
			size_t res = ::iconv(handle.get(), &src_ptr, &src_size, &dst_ptr, &dst_size);
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
					check_convert_error();
				}
			}
			dst.append(&buf[0], buf.size() - dst_size);
		}
		dst.swap(output);
		return output;
	}
};

std::string TextEncoding::ToUtf8String(const std::string& encoding_name,
	const std::string& input,
	size_t buf_size,
	bool ignore_error) {
	return impl_->ConvertToUtf8String(encoding_name, input, buf_size, ignore_error);
}

std::string TextEncoding::ToUtf8String(const std::string& input, size_t buf_size, bool ignore_error) {
	CharsetDetector detector;
	const auto encoding_name = detector.Detect(input);
	if (encoding_name.empty()) {
		throw std::runtime_error("unknown encoding");
	}
	return ToUtf8String(encoding_name, input, buf_size, ignore_error);
}

TextEncoding::TextEncoding()
	: impl_(MakeAlign<TextEncodingImpl>()) {
}

XAMP_PIMPL_IMPL(TextEncoding)

XAMP_BASE_NAMESPACE_END