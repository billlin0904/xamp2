#include <base/logger.h>
#include <stream/basslib.h>
#include <stream/eqsettings.h>
#include <stream/bassequalizer.h>

namespace xamp::stream {

class BassEqualizer::BassEqualizerImpl {
public:
    BassEqualizerImpl() {
        fx_handles_.fill(0);
        preamp_ = 0;
    }

    void Start(uint32_t sample_rate) {
        RemoveFx();

        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
                                             kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));

        preamp_ = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);

        auto i = 0;
        for (auto& fx_handle : fx_handles_) {
            fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_PEAKEQ, 1);
            if (!fx_handle) {
                throw BassException(BASS.BASS_ErrorGetCode());
            }
            BASS_BFX_PEAKEQ eq{};
            eq.lBand = i;
            eq.fCenter = kEQBands[i];
            eq.fBandwidth = kEQBands[i] / 2;
            eq.fGain = 0.0F;
            eq.lChannel = BASS_BFX_CHANALL;
            BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handle, &eq));
            ++i;
        }
    }

    void Process(float const* samples, uint32_t num_samples, Buffer<float>& out) {
        if (out.size() < num_samples) {
            out.resize(num_samples);
        }
        MemoryCopy(out.data(), samples, num_samples * sizeof(float));

        const auto bytes_read =
            BASS.BASS_ChannelGetData(stream_.get(),
                                     out.data(),
                                     num_samples * sizeof(float));
        if (bytes_read == kBassError) {
            return;
        }
        if (bytes_read == 0) {
            return;
        }
        const auto frames = bytes_read / sizeof(float);
        out.resize(frames);
    }

    void SetEQ(EQSettings const &settings) {
        uint32_t i = 0;
        for (auto settings : settings.bands) {
            SetEQ(i++, settings.gain, settings.Q);
        }
        SetPreamp(settings.preamp);
    }

    void SetEQ(uint32_t band, float gain, float Q) {
        if (band >= fx_handles_.size()) {
            return;
        }

        BASS_BFX_PEAKEQ eq{};
        BassIfFailedThrow(BASS.BASS_FXGetParameters(fx_handles_[band], &eq));
        eq.fGain = gain;
        eq.fQ = Q;
        eq.fBandwidth = kEQBands[band] / 2;
        eq.fCenter = kEQBands[band];
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handles_[band], &eq));
    }

    void SetPreamp(float preamp) {
        BASS_BFX_VOLUME fv;
        fv.lChannel = 0;
        fv.fVolume = static_cast<float>(std::pow(10, (preamp / 20)));
        BassIfFailedThrow(BASS.BASS_FXSetParameters(preamp_, &fv));
    }

    void Disable() {
        for (uint32_t i = 0; i < fx_handles_.size(); ++i) {
            SetEQ(i, 0.0, 0.0);
        }
        SetPreamp(0);
    }

private:
    void RemoveFx() {
        for (auto fx_handle : fx_handles_) {
            BASS.BASS_ChannelRemoveFX(stream_.get(), fx_handle);
        }
        fx_handles_.fill(0);

        BASS.BASS_ChannelRemoveFX(stream_.get(), preamp_);
        preamp_ = 0;
    }

    HFX preamp_;
    BassStreamHandle stream_;
    std::array<HFX, kMaxBand> fx_handles_;
};

BassEqualizer::BassEqualizer()
    : impl_(MakeAlign<BassEqualizerImpl>()) {
}

XAMP_PIMPL_IMPL(BassEqualizer)

void BassEqualizer::Start(uint32_t samplerate) {
    impl_->Start(samplerate);
}

void BassEqualizer::SetEQ(uint32_t band, float gain, float Q) {
    impl_->SetEQ(band, gain, Q);
}

void BassEqualizer::SetEQ(EQSettings const &settings) {
    impl_->SetEQ(settings);
}

void BassEqualizer::SetPreamp(float preamp) {
    impl_->SetPreamp(preamp);
}

void BassEqualizer::Process(float const* samples, uint32_t num_samples, Buffer<float>& out)  {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassEqualizer::GetTypeId() const {
    return Id;
}

}
