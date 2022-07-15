#include <stream/basslib.h>

#include <base/logger_impl.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <base/volume.h>

#include <stream/bassvolume.h>

namespace xamp::stream {

class BassVolume::BassVolumeImpl {
public:
    BassVolumeImpl() {
        logger_ = Logger::GetInstance().GetLogger(kVolumeLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(output_sample_rate,
                                             kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        MemorySet(&volume_, 0, sizeof(volume_));
        volume_handle_ = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
    }

    void Init(double volume) {
        volume_.lChannel = -1;
        volume_.fVolume = static_cast<float>(std::pow(10, (volume / 20)));
        XAMP_LOG_D(logger_, "Volume level: {}", static_cast<int32_t>(volume_.fVolume * 100));
        BassIfFailedThrow(BASS.BASS_FXSetParameters(volume_handle_, &volume_));
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
    HFX volume_handle_;
    ::BASS_BFX_VOLUME volume_{0};
    std::shared_ptr<LoggerWriter> logger_;
};

BassVolume::BassVolume()
    : impl_(MakeAlign<BassVolumeImpl>()) {
}

void BassVolume::Start(uint32_t output_sample_rate) {
    impl_->Start(output_sample_rate);
}

XAMP_PIMPL_IMPL(BassVolume)

void BassVolume::Init(double volume) {
    impl_->Init(volume);
}

bool BassVolume::Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassVolume::GetTypeId() const {
    return Id;
}

std::string_view BassVolume::GetDescription() const noexcept {
    return "Volume";
}

void BassVolume::Flush() {
}


}

