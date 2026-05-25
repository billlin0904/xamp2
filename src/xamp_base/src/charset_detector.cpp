#include <base/charset_detector.h>
#include <base/assert.h>
#include <uchardet.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/unique_handle.h>

#include <opencc.h>
#include <cld3/nnet_language_identifier.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	class UcharDectLib final {
	public:
		XAMP_DECLARE_SINGLETON_NAME()

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

#define UCHARDECT_LIB SharedSingleton<UcharDectLib>::GetInstance()

	template <typename T>
	struct UcharDetDeleter;

	template <>
	struct UcharDetDeleter<uchardet> {
		void operator()(uchardet* p) const {
			UCHARDECT_LIB.uchardet_delete(p);
		}
	};

	using UcharDetPtr = std::unique_ptr<uchardet, UcharDetDeleter<uchardet>>;

	class OpenCCLib final {
	public:
		XAMP_DECLARE_SINGLETON_NAME()

		OpenCCLib()
			: module_(OpenSharedLibrary("opencc"))
			, XAMP_LOAD_DLL_API(opencc_open)
			, XAMP_LOAD_DLL_API(opencc_close)
			, XAMP_LOAD_DLL_API(opencc_convert_utf8)
			, XAMP_LOAD_DLL_API(opencc_convert_utf8_free)
			, XAMP_LOAD_DLL_API(opencc_error) {
		}

		XAMP_DISABLE_COPY(OpenCCLib)
	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(opencc_open);
		XAMP_DECLARE_DLL_NAME(opencc_close);
		XAMP_DECLARE_DLL_NAME(opencc_convert_utf8);
		XAMP_DECLARE_DLL_NAME(opencc_convert_utf8_free);
		XAMP_DECLARE_DLL_NAME(opencc_error);
	};

#define OPENCC_LIB SharedSingleton<OpenCCLib>::GetInstance()

	struct OpenCCHandleTraits final {
		static opencc_t invalid() {
			return reinterpret_cast<opencc_t>(-1);
		}

		static void Close(opencc_t value) {
			if (value != nullptr && value != invalid()) {
				OPENCC_LIB.opencc_close(value);
			}
		}
	};

	using OpenCCHandle = UniqueHandle<opencc_t, OpenCCHandleTraits>;

	std::string GetOpenCCError() {
		const auto* error = OPENCC_LIB.opencc_error();
		if (error == nullptr) {
			return "Unknown OpenCC error";
		}
		return error;
	}

	std::string MakeOpenCCConfigPath(const std::string& file_name, const std::string& file_path) {
		if (file_path.empty()) {
			return file_name;
		}

		auto config_path = file_path;
		const auto last = config_path.back();
		if (last != '/' && last != '\\') {
			config_path.push_back('/');
		}
		config_path.append(file_name);
		return config_path;
	}

	const std::string kJapaneseLanguage = "ja";
	const std::string kSimpleChineseLanguage = "zh";
}

class EncodingDetector::EncodingDetectorImpl {
public:
	EncodingDetectorImpl() = default;

	std::expected<std::string, EncodingDetectorError> Detect(const char* data, size_t size) {
		UcharDetPtr detector;

		// create detector
		detector.reset(UCHARDECT_LIB.uchardet_new());
		if (!detector) {
			return std::unexpected(EncodingDetectorError::ENCODING_ERROR_UNKNOWN);
		}

		// handle data
		if (UCHARDECT_LIB.uchardet_handle_data(detector.get(), data, size) != 0) {
			return std::unexpected(EncodingDetectorError::ENCODING_DATA_ERROR);
		}
		
		// end data
		UCHARDECT_LIB.uchardet_data_end(detector.get());

		// get encoding
		std::string encoding = UCHARDECT_LIB.uchardet_get_charset(detector.get());

		// reset detector
		UCHARDECT_LIB.uchardet_reset(detector.get());
		return encoding;
	}
};

class LanguageDetector::LanguageDetectorImpl {
public:
	LanguageDetectorImpl()
		: indentifier_(0, 1000) {
	}

	bool IsJapanese(const std::wstring& text) {		
		const auto result = FindLanguage(text);
		return result.is_reliable && result.language == kJapaneseLanguage;
	}

	bool IsChinese(const std::wstring& text) {		
		const auto result = FindLanguage(text);
		return result.is_reliable && result.language == kSimpleChineseLanguage;
	}
private:
	chrome_lang_id::NNetLanguageIdentifier::Result FindLanguage(const std::wstring& text) {
		return indentifier_.FindLanguage(String::ToUtf8String(text));
	}
	chrome_lang_id::NNetLanguageIdentifier indentifier_;
};

class OpenCCConvert::OpenCCConvertImpl {
public:
	OpenCCConvertImpl() = default;

	void Load(const std::string &file_name, const std::string &file_path) {
		const auto config_path = MakeOpenCCConfigPath(file_name, file_path);
		const auto handle = OPENCC_LIB.opencc_open(config_path.c_str());
		if (handle == nullptr || handle == OpenCCHandleTraits::invalid()) {
			throw std::runtime_error(GetOpenCCError());
		}
		converter_.reset(handle);
	}

	std::wstring Convert(const std::wstring& text) const {
		if (text.empty() || !converter_) {
			return text;
		}

		auto utf8_str = String::ToUtf8String(text);
		auto* converted = OPENCC_LIB.opencc_convert_utf8(
			converter_.get(),
			utf8_str.c_str(),
			utf8_str.length());
		if (converted == nullptr) {
			throw std::runtime_error(GetOpenCCError());
		}
		XAMP_ON_SCOPE_EXIT(OPENCC_LIB.opencc_convert_utf8_free(converted););

		const auto result = String::ToStdWString(converted);
		return result;
	}

private:
	OpenCCHandle converter_;
};

OpenCCConvert::OpenCCConvert()
	: impl_(MakeAlign<OpenCCConvertImpl>()) {
}

void OpenCCConvert::Load(const std::string& file_name, const std::string& file_path) {
	impl_->Load(file_name, file_path);
}

std::wstring OpenCCConvert::Convert(const std::wstring& text) const {
	return impl_->Convert(text);
}

XAMP_PIMPL_IMPL(OpenCCConvert)

EncodingDetector::EncodingDetector()
	: impl_(MakeAlign<EncodingDetectorImpl>()) {
}

XAMP_PIMPL_IMPL(EncodingDetector)

std::expected<std::string, EncodingDetectorError> EncodingDetector::Detect(const char* data, size_t size) {
	return impl_->Detect(data, size);
}

LanguageDetector::LanguageDetector()
	: impl_(MakeAlign<LanguageDetectorImpl>()) {
}

XAMP_PIMPL_IMPL(LanguageDetector)

bool LanguageDetector::IsJapanese(const std::wstring& text) {
	return impl_->IsJapanese(text);
}

bool LanguageDetector::IsChinese(const std::wstring& text) {
	return impl_->IsChinese(text);
}

void LoadUcharDectLib() {
	UCHARDECT_LIB;
	OPENCC_LIB;
}

XAMP_BASE_NAMESPACE_END

