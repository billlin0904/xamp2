#include <stream/basslib.h>
#include <stream/bassequalizer.h>

namespace xamp::stream {

static void EnsureFxLibInit() {
    if (!BassLib::Instance().FxLib) {
        BassLib::Instance().FxLib = MakeAlign<BassFxLib>();
    }
}

#define BASS_BFX_CHANALL	-1

typedef struct {
    int   lBand;							// [0...............n] more bands means more memory & cpu usage
    float fBandwidth;						// [0.1...........<10] in octaves - fQ is not in use (Bandwidth has a priority over fQ)
    float fQ;								// [0...............1] the EE kinda definition (linear) (if Bandwidth is not in use)
    float fCenter;							// [1Hz..<info.freq/2] in Hz
    float fGain;							// [-15dB...0...+15dB] in dB (can be above/below these limits)
    int   lChannel;							// BASS_BFX_CHANxxx flag/s
} BASS_BFX_PEAKEQ;

// DSP effects
enum {
    BASS_FX_BFX_ROTATE = 0x10000,			// A channels volume ping-pong	/ multi channel
    BASS_FX_BFX_ECHO,						// Echo							/ 2 channels max	(deprecated)
    BASS_FX_BFX_FLANGER,					// Flanger						/ multi channel		(deprecated)
    BASS_FX_BFX_VOLUME,						// Volume						/ multi channel
    BASS_FX_BFX_PEAKEQ,						// Peaking Equalizer			/ multi channel
    BASS_FX_BFX_REVERB,						// Reverb						/ 2 channels max	(deprecated)
    BASS_FX_BFX_LPF,						// Low Pass Filter 24dB			/ multi channel		(deprecated)
    BASS_FX_BFX_MIX,						// Swap, remap and mix channels	/ multi channel
    BASS_FX_BFX_DAMP,						// Dynamic Amplification		/ multi channel
    BASS_FX_BFX_AUTOWAH,					// Auto Wah						/ multi channel
    BASS_FX_BFX_ECHO2,						// Echo 2						/ multi channel		(deprecated)
    BASS_FX_BFX_PHASER,						// Phaser						/ multi channel
    BASS_FX_BFX_ECHO3,						// Echo 3						/ multi channel		(deprecated)
    BASS_FX_BFX_CHORUS,						// Chorus/Flanger				/ multi channel
    BASS_FX_BFX_APF,						// All Pass Filter				/ multi channel		(deprecated)
    BASS_FX_BFX_COMPRESSOR,					// Compressor					/ multi channel		(deprecated)
    BASS_FX_BFX_DISTORTION,					// Distortion					/ multi channel
    BASS_FX_BFX_COMPRESSOR2,				// Compressor 2					/ multi channel
    BASS_FX_BFX_VOLUME_ENV,					// Volume envelope				/ multi channel
    BASS_FX_BFX_BQF,						// BiQuad filters				/ multi channel
    BASS_FX_BFX_ECHO4,						// Echo	4						/ multi channel
    BASS_FX_BFX_PITCHSHIFT,					// Pitch shift using FFT		/ multi channel		(not available on mobile)
    BASS_FX_BFX_FREEVERB					// Reverb using "Freeverb" algo	/ multi channel
};

class BassEqualizer::BassEqualizerImpl {
public:
	BassEqualizerImpl() {
	}

    ~BassEqualizerImpl() {
    }

	bool Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
        buffer_.resize(num_samples);
        BassLib::Instance().BASS_StreamPutData(stream_.get(), sample_buffer, num_samples * sizeof(float));
        const auto bytes_read = BassLib::Instance().BASS_ChannelGetData(stream_.get(), buffer_.data(), buffer_.size() * sizeof(float));
        if (bytes_read == kBassError) {
            return false;
        }
        return buffer.TryWrite(reinterpret_cast<int8_t const*>(buffer_.data()), buffer_.size() * sizeof(float));
	}

	void SetEQ(uint32_t band, float gain) {
		if (band >= fx_handles_.size()) {
			return;
		}

        BASS_DX8_PARAMEQ eq;
        if (BassLib::Instance().FxLib->BASS_FXGetParameters(fx_handles_[band], &eq)) {
			eq.fGain = gain;
            BassIfFailedThrow(BassLib::Instance().FxLib->BASS_FXSetParameters(fx_handles_[band], &eq));
		}
	}

    void Close() {
        for (auto fx_handle : fx_handles_) {
            BassLib::Instance().FxLib->BASS_ChannelRemoveFX(stream_.get(), fx_handle);
        }
    }

	void Start(uint32_t num_channels, uint32_t input_samplerate) {
        EnsureFxLibInit();

		stream_.reset(BassLib::Instance().BASS_StreamCreate(input_samplerate,
			num_channels, 
            BASS_SAMPLE_FLOAT,
			STREAMPROC_PUSH,
			this));

		fx_handles_.fill(0);

		auto i = 0;
		for (auto &fx_handle : fx_handles_) {			
            fx_handle = BassLib::Instance().FxLib->BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_PEAKEQ, 0);
			if (!fx_handle) {
				throw BassException(BassLib::Instance().BASS_ErrorGetCode());
			}
            BASS_BFX_PEAKEQ eq;
			eq.fBandwidth = kEQBands[i].bandwidth;
			eq.fCenter = kEQBands[i].center;
            eq.fGain = 1.0F;
            eq.lChannel = BASS_BFX_CHANALL;
            BassIfFailedThrow(BassLib::Instance().FxLib->BASS_FXSetParameters(fx_handle, &eq));
			++i;
		}
	}

private:	
	BassStreamHandle stream_;	
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

void BassEqualizer::SetEQ(uint32_t band, float gain) {
	impl_->SetEQ(band, gain);
}

bool BassEqualizer::Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(sample_buffer, num_samples, buffer);
}

}
