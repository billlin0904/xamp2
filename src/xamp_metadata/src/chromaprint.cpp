#include <metadata/chromaprintlib.h>
#include <metadata/chromaprint.h>

XAMP_METADATA_NAMESPACE_BEGIN

class Chromaprint::ChromaprintImpl {
public:
	explicit ChromaprintImpl(int32_t algorithm = CHROMAPRINT_ALGORITHM_DEFAULT)
		: algorithm_(algorithm) {
	}

	void SetSampleRate(uint32_t sample_rate) {
		handle_.reset(CHROMAPRINT_LIB.chromaprint_new(algorithm_));
		if (!handle_) {
			throw std::bad_alloc();
		}

		CHROMAPRINT_LIB.chromaprint_start(handle_.get(),
			static_cast<int32_t>(sample_rate),
			static_cast<int32_t>(AudioFormat::kMaxChannel));
	}

	int32_t Process(int16_t const* data, uint32_t size) const {
		return CHROMAPRINT_LIB.chromaprint_feed(handle_.get(), data, static_cast<int32_t>(size));
	}

	int32_t Finish() const {
		return CHROMAPRINT_LIB.chromaprint_finish(handle_.get());
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
			CHROMAPRINT_LIB.chromaprint_encode_fingerprint(fprint,
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

		CHROMAPRINT_LIB.chromaprint_dealloc(fprint);
		CHROMAPRINT_LIB.chromaprint_dealloc(encoded);

		return fingerprint;
	}

private:
	int32_t GetRawFingerprint(uint32_t** fingerprint, int32_t* size) const {
		return CHROMAPRINT_LIB.chromaprint_get_raw_fingerprint(handle_.get(), fingerprint, size);
	}

	struct ChromaprintContextTraits final {
		static ChromaprintContext* invalid() noexcept {
			return nullptr;
		}

		static void close(ChromaprintContext* value) noexcept {
			CHROMAPRINT_LIB.chromaprint_free(value);
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

void Chromaprint::SetSampleRate(uint32_t sample_rate) {
	return impl_->SetSampleRate(sample_rate);
}

int32_t Chromaprint::Process(int16_t const* data, uint32_t size) const {
	return impl_->Process(data, size);
}

int32_t Chromaprint::Finish() const {
	return impl_->Finish();
}

std::vector<uint8_t> Chromaprint::GetFingerprint() const {
	return impl_->GetFingerprint();
}

XAMP_METADATA_NAMESPACE_END