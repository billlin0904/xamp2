#include <stream/basslib.h>
#include <stream/bassequalizer.h>

namespace xamp::stream {

class BassEqualizer::BassEqualizerImpl {
public:
	BassEqualizerImpl() {
	}

	bool Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
		BassLib::Instance().BASS_StreamPutData(stream_.get(), sample_buffer, num_samples);
		return false;
	}

	void SetEQ(uint32_t band, float gain) {
		if (band >= fx_handles_.size()) {
			return;
		}
		BASS_DX8_PARAMEQ eq;
		if (BassLib::Instance().BASS_FXGetParameters(fx_handles_[band], &eq)) {
			eq.fGain = gain;
			BassIfFailedThrow(BassLib::Instance().BASS_FXSetParameters(fx_handles_[band], &eq));
		}
	}

	void Start(uint32_t num_channels, uint32_t input_samplerate) {
		stream_.reset(BassLib::Instance().BASS_StreamCreate(input_samplerate,
			num_channels, 
			0,
			STREAMPROC_PUSH,
			this));

		fx_handles_.fill(0);

		auto i = 0;
		for (auto &fx_handle : fx_handles_) {			
			fx_handle = BassLib::Instance().BASS_ChannelSetFX(stream_.get(), BASS_FX_DX8_PARAMEQ, 0);
			if (!fx_handle) {
				throw BassException(BassLib::Instance().BASS_ErrorGetCode());
			}
			BASS_DX8_PARAMEQ eq;
			eq.fBandwidth = kEQBands[i].bandwidth;
			eq.fCenter = kEQBands[i].center;
			eq.fGain = 0;
			BassIfFailedThrow(BassLib::Instance().BASS_FXSetParameters(fx_handle, &eq));
			++i;
		}
	}

private:	
	BassStreamHandle stream_;	
	std::array<HFX, kMaxBand> fx_handles_;
};

BassEqualizer::BassEqualizer()
	: impl_(MakeAlign<BassEqualizerImpl>()) {
}

XAMP_PIMPL_IMPL(BassEqualizer)

void BassEqualizer::Start(uint32_t num_channels, uint32_t input_samplerate) {
	impl_->Start(num_channels, input_samplerate);
}

void BassEqualizer::SetEQ(uint32_t band, float gain) {
	impl_->SetEQ(band, gain);
}

bool BassEqualizer::Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(sample_buffer, num_samples, buffer);
}

}