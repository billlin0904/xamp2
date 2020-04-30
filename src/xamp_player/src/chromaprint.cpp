#include <vector>

#include <chromaprint.h>

#include <base/memory.h>
#include <base/logger.h>
#include <base/base.h>
#include <base/exception.h>
#include <base/dll.h>
#include <base/unique_handle.h>
#include <base/dataconverter.h>
#include <player/chromaprint.h>

namespace xamp::player {

class ChromaprintLib final {
public:
	ChromaprintLib() try
#ifdef XAMP_OS_WIN
		: module_(LoadModule("chromaprint.dll"))
#else
        : module_(LoadModule("libchromaprint.dylib"))
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
    XAMP_DECLARE_DLL(chromaprint_new) chromaprint_new;
    XAMP_DECLARE_DLL(chromaprint_free) chromaprint_free;
    XAMP_DECLARE_DLL(chromaprint_start) chromaprint_start;
    XAMP_DECLARE_DLL(chromaprint_feed) chromaprint_feed;
    XAMP_DECLARE_DLL(chromaprint_finish) chromaprint_finish;
    XAMP_DECLARE_DLL(chromaprint_get_raw_fingerprint) chromaprint_get_raw_fingerprint;
    XAMP_DECLARE_DLL(chromaprint_encode_fingerprint) chromaprint_encode_fingerprint;
    XAMP_DECLARE_DLL(chromaprint_dealloc) chromaprint_dealloc;
};

struct ChromaprintContextTraits final {
    static ChromaprintContext* invalid() noexcept {
        return nullptr;
    }

    static void close(ChromaprintContext* value) noexcept {
        ChromaprintLib::Instance().chromaprint_free(value);
    }
};

using ChromaprintContextPtr = UniqueHandle<ChromaprintContext*, ChromaprintContextTraits>;

class Chromaprint::ChromaprintImpl {
public:
	explicit ChromaprintImpl(int32_t algorithm = CHROMAPRINT_ALGORITHM_DEFAULT)
		: algorithm_(algorithm) {
	}

	~ChromaprintImpl() {
	}

    void Start(uint32_t sample_rate, uint32_t num_channels, uint32_t num_buffer_frames) {
        context_.reset(ChromaprintLib::Instance().chromaprint_new(algorithm_));
        buffer_.resize(static_cast<size_t>(num_buffer_frames));
        ChromaprintLib::Instance().chromaprint_start(context_.get(), static_cast<int32_t>(sample_rate), static_cast<int32_t>(num_channels));
	}

    int32_t Feed(const int16_t* data, uint32_t size) const {
        return ChromaprintLib::Instance().chromaprint_feed(context_.get(), data, static_cast<int32_t>(size));
	}

    int32_t Finish() const {
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

            fingerprint.resize(static_cast<size_t>(encoded_size));
            (void)FastMemcpy(fingerprint.data(), encoded, static_cast<size_t>(encoded_size));
		}
		catch (...) {
		}		

		ChromaprintLib::Instance().chromaprint_dealloc(fprint);
		ChromaprintLib::Instance().chromaprint_dealloc(encoded);

		return fingerprint;
	}	

private:
    int32_t GetRawFingerprint(uint32_t** fingerprint, int32_t* size) const {
		return ChromaprintLib::Instance().chromaprint_get_raw_fingerprint(context_.get(), fingerprint, size);
	}

	int32_t algorithm_;
	ChromaprintContextPtr context_;
	std::vector<int16_t> buffer_;
};

Chromaprint::Chromaprint()
	: impl_(MakeAlign<ChromaprintImpl>()) {
}

void Chromaprint::LoadChromaprintLib() {
	ChromaprintLib::Instance();
}

Chromaprint::~Chromaprint() {
}

void Chromaprint::Start(uint32_t sample_rate, uint32_t num_channels, uint32_t num_buffer_frames) {
	return impl_->Start(sample_rate, num_channels, num_buffer_frames);
}

int32_t Chromaprint::Feed(const int16_t* data, uint32_t size) const {
	return impl_->Feed(data, size);
}

int32_t Chromaprint::Finish() const {
	return impl_->Finish();
}

std::vector<uint8_t> Chromaprint::GetFingerprint() const {
	return impl_->GetFingerprint();
}

}

