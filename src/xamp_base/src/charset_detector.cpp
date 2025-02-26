#include <base/charset_detector.h>
#include <base/assert.h>
#include <uchardet.h>
#include <base/dll.h>
#include <base/logger_impl.h>

#include <cld3/nnet_language_identifier.h>

XAMP_BASE_NAMESPACE_BEGIN

class UcharDectLib final {
public:
	UcharDectLib();

	XAMP_DISABLE_COPY(UcharDectLib)
private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(uchardet_new);
	XAMP_DECLARE_DLL_NAME(uchardet_handle_data);
	XAMP_DECLARE_DLL_NAME(uchardet_delete);
	XAMP_DECLARE_DLL_NAME(uchardet_data_end);
	XAMP_DECLARE_DLL_NAME(uchardet_get_charset);
	XAMP_DECLARE_DLL_NAME(uchardet_reset);
};

inline UcharDectLib::UcharDectLib() try
	: module_(OpenSharedLibrary("uchardet"))
	, XAMP_LOAD_DLL_API(uchardet_new)
	, XAMP_LOAD_DLL_API(uchardet_handle_data)
	, XAMP_LOAD_DLL_API(uchardet_delete)
	, XAMP_LOAD_DLL_API(uchardet_data_end)
	, XAMP_LOAD_DLL_API(uchardet_get_charset)
	, XAMP_LOAD_DLL_API(uchardet_reset) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

#define UCHARDECT_LIB Singleton<UcharDectLib>::GetInstance()

template <typename T>
struct UcharDetDeleter;

template <>
struct UcharDetDeleter<uchardet> {
	void operator()(uchardet* p) const {		
		UCHARDECT_LIB.uchardet_delete(p);
	}
};

using UcharDetPtr = std::unique_ptr<uchardet, UcharDetDeleter<uchardet>>;

class CharsetDetector::CharsetDetectorImpl {
public:
	CharsetDetectorImpl()
		: indentifier_(0, 1000) {
	}

	std::string Detect(const char* data, size_t size) {
		UcharDetPtr detector;

		// create detector
		detector.reset(UCHARDECT_LIB.uchardet_new());
		if (!detector) {
			throw std::runtime_error("unknown error");
		}

		// handle data
		if (UCHARDECT_LIB.uchardet_handle_data(detector.get(), data, size) != 0) {
			return "";
		}
		
		// end data
		UCHARDECT_LIB.uchardet_data_end(detector.get());

		// get encoding
		std::string encoding = UCHARDECT_LIB.uchardet_get_charset(detector.get());

		// reset detector
		UCHARDECT_LIB.uchardet_reset(detector.get());
		return encoding;
	}

	bool IsJapanese(const std::wstring& text) {
		static const std::string kJapanese = "ja";
		const auto result = indentifier_.FindLanguage(String::ToUtf8String(text));
		return result.is_reliable && result.language == kJapanese;
	}

private:
	chrome_lang_id::NNetLanguageIdentifier indentifier_;
};

CharsetDetector::CharsetDetector() 
	: impl_(MakeAlign<CharsetDetectorImpl>()) {
}

XAMP_PIMPL_IMPL(CharsetDetector)

std::string CharsetDetector::Detect(const char* data, size_t size) {
	return impl_->Detect(data, size);
}

bool CharsetDetector::IsJapanese(const std::wstring& text) {
	return impl_->IsJapanese(text);
}

void LoadUcharDectLib() {
	UCHARDECT_LIB;
}

XAMP_BASE_NAMESPACE_END

