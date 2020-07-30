#include <stream/basslib.h>
#include <stream/bassequalizer.h>

namespace xamp::stream {

static void EnsureFxLibInit() {
    if (!BassLib::Instance().FxLib) {
        BassLib::Instance().FxLib = MakeAlign<BassFxLib>();
    }
}

class BassEqualizer::BassEqualizerImpl {
public:
	BassEqualizerImpl() {
		fx_handles_.fill(0);
	}

	~BassEqualizerImpl() = default;

	void Start(uint32_t num_channels, uint32_t input_samplerate) {
		EnsureFxLibInit();

        RemoveFx();

		stream_.reset(BassLib::Instance().BASS_StreamCreate(input_samplerate,
			num_channels,
			BASS_STREAM_DECODE,
			STREAMPROC_PUSH,
			nullptr));

		tempo_stream_.reset(BassLib::Instance().FxLib->BASS_FX_TempoCreate(stream_.get(), 0));

		auto i = 0;
		for (auto& fx_handle : fx_handles_) {
			fx_handle = BassLib::Instance().BASS_ChannelSetFX(tempo_stream_.get(), BASS_FX_BFX_PEAKEQ, 0);
			if (!fx_handle) {
				throw BassException(BassLib::Instance().BASS_ErrorGetCode());
			}
            BASS_BFX_PEAKEQ eq{};
			eq.lBand = i;
			eq.fCenter = kEQBands[i];
			eq.fBandwidth = kEQBands[i] / 2;
			eq.fGain = 1.0F;
			eq.lChannel = BASS_BFX_CHANALL;
			BassIfFailedThrow(BassLib::Instance().BASS_FXSetParameters(fx_handle, &eq));
			++i;
		}
	}

	bool Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
        buffer_.resize(num_samples);
		auto temp_stream = BassLib::Instance().FxLib->BASS_FX_TempoGetSource(stream_.get());
        BassLib::Instance().BASS_StreamPutData(temp_stream,
			sample_buffer, 
			num_samples * sizeof(float));
		while (true) {
			const auto bytes_read = BassLib::Instance().BASS_ChannelGetData(stream_.get(),
				buffer_.data(),
				buffer_.size() * sizeof(float));
			if (bytes_read == kBassError) {
				return false;
			}
			else if (bytes_read == 0) {
				return true;
			}
			if (!buffer.TryWrite(reinterpret_cast<int8_t const*>(buffer_.data()), bytes_read)) {
				break;
			}
		}        
		return true;
	}

    void SetEQ(uint32_t band, float gain, float Q) {
		if (band >= fx_handles_.size()) {
			return;
		}

        BASS_BFX_PEAKEQ eq{};
		BassIfFailedThrow(BassLib::Instance().BASS_FXGetParameters(fx_handles_[band], &eq));
		eq.fGain = gain;
        eq.fQ = Q;
		BassIfFailedThrow(BassLib::Instance().BASS_FXSetParameters(fx_handles_[band], &eq));
	}

    void Disable() {
        for (uint32_t i = 0; i < fx_handles_.size(); ++i) {
            SetEQ(i, 0.0, 0.0);
        }
    }

private:
    void RemoveFx() {
        for (auto fx_handle : fx_handles_) {
            BassLib::Instance().BASS_ChannelRemoveFX(tempo_stream_.get(), fx_handle);
        }
        fx_handles_.fill(0);
    }

	BassStreamHandle stream_;	
	BassStreamHandle tempo_stream_;
	std::array<HFX, kMaxBand> fx_handles_;
    std::vector<float> buffer_;
};

BassEqualizer::BassEqualizer()
	: impl_(MakeAlign<BassEqualizerImpl>()) {
}

BassEqualizer::~BassEqualizer() {
}

void BassEqualizer::Start(uint32_t num_channels, uint32_t input_samplerate) {
	impl_->Start(num_channels, input_samplerate);
}

void BassEqualizer::SetEQ(uint32_t band, float gain, float Q) {
    impl_->SetEQ(band, gain, Q);
}

bool BassEqualizer::Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(sample_buffer, num_samples, buffer);
}

void BassEqualizer::Disable() {
    impl_->Disable();
}

}
