#include <stream/basslib.h>

#include <base/logger_impl.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <base/math.h>
#include <stream/bass_utiltis.h>

#include <stream/bassvolume.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassVolume);

class BassVolume::BassVolumeImpl {
public:
    BassVolumeImpl()
        : volume_handle_(0) {
        logger_ = XampLoggerFactory.GetLogger(kBassVolumeLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS_LIB.BASS_StreamCreate(output_sample_rate,
            AudioFormat::kMaxChannel,
            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
            STREAMPROC_DUMMY,
            nullptr));
        volume_handle_ = BASS_LIB.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
    }

    void Initialize(double volume_db) {
        BASS_BFX_VOLUME volume{ 0 };
        volume.lChannel = -1;
        volume.fVolume = static_cast<float>(std::pow(10, (volume_db / 20)));
        XAMP_LOG_D(logger_, "Set volume:{} dB level:{}", Round(volume_db, 2), static_cast<int32_t>(volume.fVolume * 100));
        BassIfFailedThrow(BASS_LIB.BASS_FXSetParameters(volume_handle_, &volume));
    }

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) {
        return bass_utiltis::Process(stream_, samples, num_samples, out);
    }

private:
    BassStreamHandle stream_;
    HFX volume_handle_;
    LoggerPtr logger_;
};

BassVolume::BassVolume()
    : impl_(MakeAlign<BassVolumeImpl>()) {
}

void BassVolume::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

XAMP_PIMPL_IMPL(BassVolume)

void BassVolume::Initialize(const AnyMap& config) {
    const auto volume = config.Get<double>(DspConfig::kVolume);
    impl_->Initialize(volume);
}

bool BassVolume::Process(float const * samples, size_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassVolume::GetTypeId() const {
    return XAMP_UUID_OF(BassVolume);
}

std::string_view BassVolume::GetDescription() const noexcept {
    return Description;
}

XAMP_STREAM_NAMESPACE_END

