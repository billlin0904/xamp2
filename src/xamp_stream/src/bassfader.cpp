#include <stream/basslib.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <stream/bass_util.h>
#include <base/logger_impl.h>
#include <stream/bassfader.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassFader);

class BassFader::BassFaderImpl {
public:
    BassFaderImpl() {
        logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BassFader));
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS_LIB.BASS_StreamCreate(output_sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
    }

    void SetTime(float current, float target, float fade_time) {
        ::BASS_FX_VOLUME_PARAM volume_param{0};
        volume_param.fCurrent = current;
        volume_param.fTarget = target;
        volume_param.fTime = fade_time;
        volume_param.lCurve = 0;
        const auto fade_fx = BASS_LIB.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_VOLUME,
            0);
        BassIfFailedThrow(fade_fx);
        BassIfFailedThrow(BASS_LIB.BASS_FXSetParameters(fade_fx, &volume_param));
        XAMP_LOG_D(logger_, "Fade current:{:.2f} target:{:.2f} time:{:.2f}",
            current,
            target,
            fade_time);
    }

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) {
        return bass_util::ReadStream(stream_, samples, num_samples, out);
    }
private:
    BassStreamHandle stream_;
    LoggerPtr logger_;
};

BassFader::BassFader()
    : impl_(MakeAlign<BassFaderImpl>()) {
}

XAMP_PIMPL_IMPL(BassFader)

void BassFader::Initialize(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

void BassFader::SetTime(float current, float target, float fade_time) {
    impl_->SetTime(current, target, fade_time);
}

Uuid BassFader::GetTypeId() const {
    return XAMP_UUID_OF(BassFader);
}

std::string_view BassFader::GetDescription() const noexcept {
    return Description;
}

bool BassFader::Process(float const* samples, size_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

XAMP_STREAM_NAMESPACE_END

