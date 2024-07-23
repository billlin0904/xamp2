#include <fvad.h>
#include <widget/widget_shared.h>
#include <base/dll.h>
#include <widget/chatgpt/speechdetected.h>

struct FVadHandleDeleter final {
    static Fvad* invalid() noexcept {
        return nullptr;
    }

    static void close(Fvad* value) {
        ::fvad_free(value);
    }
};

using FvadHandle = UniqueHandle<Fvad*, FVadHandleDeleter>;

class SpeechDetected::SpeechDetectedImpl {
public:
    enum Models {
        QUALITY = 0,
        LOW_BITRATE = 1,
        AGGRESSIVE = 2,
        VERY_AGGRESSIVE = 3
    };

    SpeechDetectedImpl(int32_t sample_rate) {
        handle_.reset(::fvad_new());
        ::fvad_set_sample_rate(handle_.get(), sample_rate);
    }

    void setModel(Models model = Models::QUALITY) {
        ::fvad_set_mode(handle_.get(), model);
    }

    bool isSpeech(const std::vector<int16_t>& data) {
        return ::fvad_process(handle_.get(), data.data(), data.size()) > 0;
    }

private:
    FvadHandle handle_;
};

SpeechDetected::SpeechDetected(QObject *parent)
    : QObject(parent)
    , impl_(MakeAlign<SpeechDetectedImpl>(16000)) {
}

SpeechDetected::~SpeechDetected() {
}

bool SpeechDetected::isSpeech(const std::vector<int16_t>& data) {
    return impl_->isSpeech(data);
}

void SpeechDetected::reset() {
    
}
