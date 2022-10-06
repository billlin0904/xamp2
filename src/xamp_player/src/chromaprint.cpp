#include <vector>

#include <chromaprint.h>

#include <base/singleton.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/base.h>
#include <base/exception.h>
#include <base/dll.h>
#include <base/unique_handle.h>
#include <base/dataconverter.h>
#include <player/chromaprint.h>

namespace xamp::player {

class ChromaprintLib final {
public:
	friend class Singleton<ChromaprintLib>;

	XAMP_DISABLE_COPY(ChromaprintLib)

private:
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
	catch (Exception const& e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

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

#define CPDLL Singleton<ChromaprintLib>::GetInstance()

class Chromaprint::ChromaprintImpl {
public:
	explicit ChromaprintImpl(int32_t algorithm = CHROMAPRINT_ALGORITHM_DEFAULT)
		: algorithm_(algorithm) {
	}

	void Start(uint32_t sample_rate, uint32_t num_channels) {
		handle_.reset(CPDLL.chromaprint_new(algorithm_));
		CPDLL.chromaprint_start(handle_.get(),
			static_cast<int32_t>(sample_rate),
			static_cast<int32_t>(num_channels));
	}

	int32_t Feed(int16_t const* data, uint32_t size) const {
		return CPDLL.chromaprint_feed(handle_.get(), data, static_cast<int32_t>(size));
	}

	int32_t Finish() const {
		return CPDLL.chromaprint_finish(handle_.get());
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
			CPDLL.chromaprint_encode_fingerprint(fprint,
				size,
				algorithm_,
				&encoded,
				&encoded_size,
				1);

			fingerprint.resize(static_cast<size_t>(encoded_size));
			MemoryCopy(fingerprint.data(), encoded, static_cast<size_t>(encoded_size));
		}
		catch (...) {
		}

		CPDLL.chromaprint_dealloc(fprint);
		CPDLL.chromaprint_dealloc(encoded);

		return fingerprint;
	}

private:
	int32_t GetRawFingerprint(uint32_t** fingerprint, int32_t* size) const {
		return CPDLL.chromaprint_get_raw_fingerprint(handle_.get(), fingerprint, size);
	}

	struct ChromaprintContextTraits final {
		static ChromaprintContext* invalid() noexcept {
			return nullptr;
		}

		static void close(ChromaprintContext* value) noexcept {
			CPDLL.chromaprint_free(value);
		}
	};

	using ChromaprintHandle = UniqueHandle<ChromaprintContext*, ChromaprintContextTraits>;

	int32_t algorithm_;
	ChromaprintHandle handle_;
};

Chromaprint::Chromaprint()
	: impl_(MakeAlign<ChromaprintImpl>()) {
}

XAMP_PIMPL_IMPL(Chromaprint)

void Chromaprint::LoadChromaprintLib() {
	(void) Singleton<ChromaprintLib>::GetInstance();
}

void Chromaprint::Start(uint32_t sample_rate, uint32_t num_channels) {
	return impl_->Start(sample_rate, num_channels);
}

int32_t Chromaprint::Feed(int16_t const * data, uint32_t size) const {
	return impl_->Feed(data, size);
}

int32_t Chromaprint::Finish() const {
	return impl_->Finish();
}

std::vector<uint8_t> Chromaprint::GetFingerprint() const {
	return impl_->GetFingerprint();
}

}

