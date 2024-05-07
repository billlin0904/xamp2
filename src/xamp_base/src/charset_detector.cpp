#include <base/charset_detector.h>
#include <base/assert.h>
#include <uchardet.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
struct UcharDetDeleter;

template <>
struct UcharDetDeleter<uchardet> {
	void operator()(uchardet* p) const {
		XAMP_EXPECTS(p != nullptr);
		::uchardet_delete(p);
	}
};

using UcharDetPtr = std::unique_ptr<uchardet, UcharDetDeleter<uchardet>>;

class CharsetDetector::CharsetDetectorImpl {
public:
	CharsetDetectorImpl() {
		detector_.reset(::uchardet_new());
	}

	std::string Detect(const char* data, size_t size) {
		if (::uchardet_handle_data(detector_.get(), data, size) != 0) {
			return "";
		}

		::uchardet_data_end(detector_.get());
		std::string encoding = ::uchardet_get_charset(detector_.get());
		::uchardet_reset(detector_.get());
		return encoding;
	}
private:
	UcharDetPtr detector_;
};

CharsetDetector::CharsetDetector() 
	: impl_(MakeAlign<CharsetDetectorImpl>()) {
}

XAMP_PIMPL_IMPL(CharsetDetector)

std::string CharsetDetector::Detect(const char* data, size_t size) {
	return impl_->Detect(data, size);
}

XAMP_BASE_NAMESPACE_END