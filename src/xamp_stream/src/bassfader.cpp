#include <stream/basslib.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <stream/bass_utiltis.h>
#include <base/logger_impl.h>
#include <stream/bassfader.h>

namespace xamp::stream {

XAMP_DECLARE_LOG_NAME(BassFader);

class BassFader::BassFaderImpl {
public:
    BassFaderImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kBassFaderLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(output_sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
    }

    void SetTime(float current, float target, float fdade_time) {
        ::BASS_FX_VOLUME_PARAM volume_param{0};
        volume_param.fCurrent = current;
        volume_param.fTarget = target;
        volume_param.fTime = fdade_time;
        volume_param.lCurve = 0;
        const auto fade_fx = BASS.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_VOLUME,
            0);
        BassIfFailedThrow(fade_fx);
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fade_fx, &volume_param));
        XAMP_LOG_D(logger_, "Fade current:{:.2f} target:{:.2f} time:{:.2f}",
            current,
            target,
            fdade_time);
    }

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
        return BassUtiltis::Process(stream_, samples, num_samples, out);
    }

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) {
        return BassUtiltis::Process(stream_, samples, out, num_samples);
    }
private:
    BassStreamHandle stream_;
    LoggerPtr logger_;
};

BassFader::BassFader()
    : impl_(MakeAlign<BassFaderImpl>()) {
}

void BassFader::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

XAMP_PIMPL_IMPL(BassFader)

void BassFader::Init(const AnyMap& config) {
}

void BassFader::SetTime(float current, float target, float fdade_time) {
    impl_->SetTime(current, target, fdade_time);
}

bool BassFader::Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

uint32_t BassFader::Process(float const* samples, float* out, uint32_t num_samples) {
    return impl_->Process(samples, out, num_samples);
}

Uuid BassFader::GetTypeId() const {
    return XAMP_UUID_OF(BassFader);
}

std::string_view BassFader::GetDescription() const noexcept {
    return "BassFader";
}

void BassFader::Flush() {
	
}
}
