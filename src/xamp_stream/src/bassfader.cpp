#include <stream/basslib.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <stream/bassfader.h>

namespace xamp::stream {

class BassFader::BassFaderImpl {
public:
    BassFaderImpl() {
        logger_ = Logger::GetInstance().GetLogger(kCompressorLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(output_sample_rate,
                                             kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        MemorySet(&volume_param_, 0, sizeof(volume_param_));
    }

    void Init(float current, float target, float time) {
        ::BASS_FX_VOLUME_PARAM volume_param{0};
        volume_param.fCurrent = current;
        volume_param.fTarget = target;
        volume_param.fTime = time;
        volume_param.lCurve = 0;

        const auto fade_fx = BASS.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_VOLUME,
            0);
        BassIfFailedThrow(fade_fx);
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fade_fx, &volume_param_));
        volume_param_ = volume_param;
        XAMP_LOG_D(logger_, "Fade current:{:.2f} target:{:.2f} time:{:.2f}",
            current,
            target,
            time);
    }

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
        if (out.size() != num_samples) {
            out.maybe_resize(num_samples);
    	}        
        MemoryCopy(out.data(), samples, num_samples * sizeof(float));

        const auto bytes_read = 
            BASS.BASS_ChannelGetData(stream_.get(),
                out.data(),
                num_samples * sizeof(float));
        if (bytes_read == kBassError) {
            return false;
        }
        if (bytes_read == 0) {
            return false;
        }
        const auto frames = bytes_read / sizeof(float);
        out.maybe_resize(frames);
        return true;
    }

private:
    BassStreamHandle stream_;
    ::BASS_FX_VOLUME_PARAM volume_param_{0};
    std::shared_ptr<spdlog::logger> logger_;
};

BassFader::BassFader()
    : impl_(MakeAlign<BassFaderImpl>()) {
}

void BassFader::Start(uint32_t output_sample_rate) {
    impl_->Start(output_sample_rate);
}

XAMP_PIMPL_IMPL(BassFader)

void BassFader::Init(float current, float target, float time) {
    impl_->Init(current, target, time);
}

bool BassFader::Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassFader::GetTypeId() const {
    return Id;
}

std::string_view BassFader::GetDescription() const noexcept {
    return "BassFader";
}

void BassFader::Flush() {
	
}
}
