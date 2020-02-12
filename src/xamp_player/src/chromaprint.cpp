#include <vector>

#include <chromaprint.h>

#include <base/memory.h>
#include <base/logger.h>
#include <base/base.h>
#include <base/exception.h>
#include <base/dll.h>
#include <base/dataconverter.h>
#include <player/chromaprint.h>

namespace xamp::player {

class ChromaprintLib final {
public:
	ChromaprintLib() try
#ifdef _WIN32
		: module_(LoadDll("chromaprint.dll"))
#else
		: module_(LoadDll("libchromaprint.dylib"))
#endif
		, chromaprint_new(module_, "chromaprint_new")
		, chromaprint_free(module_, "chromaprint_free")
		, chromaprint_start(module_, "chromaprint_start")
		, chromaprint_feed(module_, "chromaprint_feed")
		, chromaprint_finish(module_, "chromaprint_finish")
		, chromaprint_get_raw_fingerprint(module_, "chromaprint_get_raw_fingerprint")
		, chromaprint_encode_fingerprint(module_, "chromaprint_encode_fingerprint")
		, chromaprint_dealloc(module_, "chromaprint_dealloc") {
	}
	catch (const Exception & e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	static XAMP_ALWAYS_INLINE ChromaprintLib& Instance() {
		static ChromaprintLib lib;
		return lib;
	}

	XAMP_DISABLE_COPY(ChromaprintLib)

private:
	ModuleHandle module_;

public:	
	XAMP_DEFINE_DLL_API(chromaprint_new) chromaprint_new;
	XAMP_DEFINE_DLL_API(chromaprint_free) chromaprint_free;
	XAMP_DEFINE_DLL_API(chromaprint_start) chromaprint_start;
	XAMP_DEFINE_DLL_API(chromaprint_feed) chromaprint_feed;
	XAMP_DEFINE_DLL_API(chromaprint_finish) chromaprint_finish;
	XAMP_DEFINE_DLL_API(chromaprint_get_raw_fingerprint) chromaprint_get_raw_fingerprint;
	XAMP_DEFINE_DLL_API(chromaprint_encode_fingerprint) chromaprint_encode_fingerprint;
	XAMP_DEFINE_DLL_API(chromaprint_dealloc) chromaprint_dealloc;
};

struct ChromaprintContextTraits final {
	void operator()(ChromaprintContext* value) const {
		ChromaprintLib::Instance().chromaprint_free(value);
	}
};

using ChromaprintContextPtr = std::unique_ptr<ChromaprintContext, ChromaprintContextTraits>;

class Chromaprint::ChromaprintImpl {
public:
	explicit ChromaprintImpl(int32_t algorithm = CHROMAPRINT_ALGORITHM_DEFAULT)
		: algorithm_(algorithm) {
	}

	~ChromaprintImpl() {
	}

	void Start(int32_t sample_rate, int32_t num_channels, int32_t num_buffer_frames) {
		context_.reset(ChromaprintLib::Instance().chromaprint_new(algorithm_));		
		buffer_.resize(num_buffer_frames);
		ChromaprintLib::Instance().chromaprint_start(context_.get(), sample_rate, num_channels);
	}

	int Feed(const int16_t* data, int size) const {
		return ChromaprintLib::Instance().chromaprint_feed(context_.get(), data, size);
	}

	int Finish() const {
		return ChromaprintLib::Instance().chromaprint_finish(context_.get());
	}	

	std::vector<uint8_t> GetFingerprint() const {
		std::vector<uint8_t> fingerprint;

		uint32_t* fprint = nullptr;
		int size = 0;

		if (GetRawFingerprint(&fprint, &size) != 1) {
			return fingerprint;
		}

		char* encoded = nullptr;
		int32_t encoded_size = 0;		

		try {			
			ChromaprintLib::Instance().chromaprint_encode_fingerprint(fprint,
				size,
				algorithm_,
				&encoded,
				&encoded_size,
				1);

			fingerprint.resize(encoded_size);
			FastMemcpy(fingerprint.data(), encoded, encoded_size);
		}
		catch (...) {
		}		

		ChromaprintLib::Instance().chromaprint_dealloc(fprint);
		ChromaprintLib::Instance().chromaprint_dealloc(encoded);

		return fingerprint;
	}	

private:
	int GetRawFingerprint(uint32_t** fingerprint, int32_t* size) const {
		return ChromaprintLib::Instance().chromaprint_get_raw_fingerprint(context_.get(), fingerprint, size);
	}

	int32_t algorithm_;
	ChromaprintContextPtr context_;
	std::vector<int16_t> buffer_;
};

Chromaprint::Chromaprint()
	: impl_(MakeAlign<ChromaprintImpl>()) {
}

Chromaprint::~Chromaprint() {
}

void Chromaprint::Start(int32_t sample_rate, int32_t num_channels, int32_t num_buffer_frames) {
	return impl_->Start(sample_rate, num_channels, num_buffer_frames);
}

int Chromaprint::Feed(const int16_t* data, int size) const {
	return impl_->Feed(data, size);
}

int Chromaprint::Finish() const {
	return impl_->Finish();
}

std::vector<uint8_t> Chromaprint::GetFingerprint() const {
	return impl_->GetFingerprint();
}

}
