#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <base/singleton.h>
#include <QFile>
#include <QTextCodec>

#include <base/dll.h>
#include <widget/chatgpt/whisperservice.h>

class WhisperLib {
public:
    WhisperLib()
        : module_(OpenSharedLibrary(""))
        , XAMP_LOAD_DLL_API(whisper_full_default_params)
        , XAMP_LOAD_DLL_API(whisper_init_from_buffer)
        , XAMP_LOAD_DLL_API(whisper_free)
        , XAMP_LOAD_DLL_API(whisper_model_ftype)
        , XAMP_LOAD_DLL_API(whisper_model_type)
        , XAMP_LOAD_DLL_API(whisper_full_parallel)
        , XAMP_LOAD_DLL_API(whisper_full_get_segment_text)
        , XAMP_LOAD_DLL_API(whisper_full_n_segments) {
    }
private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(whisper_full_default_params);
    XAMP_DECLARE_DLL_NAME(whisper_init_from_buffer);
    XAMP_DECLARE_DLL_NAME(whisper_free);
    XAMP_DECLARE_DLL_NAME(whisper_model_ftype);
    XAMP_DECLARE_DLL_NAME(whisper_model_type);
    XAMP_DECLARE_DLL_NAME(whisper_full_parallel);
    XAMP_DECLARE_DLL_NAME(whisper_full_get_segment_text);
    XAMP_DECLARE_DLL_NAME(whisper_full_n_segments);
};

#define WHISPER_LIB Singleton<WhisperLib>::GetInstance()

struct WhisperContextDeleter final {
    static whisper_context* invalid() noexcept {
        return nullptr;
    }

    static void close(whisper_context* value) {
        WHISPER_LIB.whisper_free(value);
    }
};

using WhisperContext = UniqueHandle<whisper_context*, WhisperContextDeleter>;

class WhisperService::Whisper {
public:
    Whisper() {
    }

    void collectInfo() {
        float_type_ = WHISPER_LIB.whisper_model_ftype(ctx_.get());
        model_type_ = WHISPER_LIB.whisper_model_type(ctx_.get());
    }

    void unloadModel() {
        ctx_.reset();
        float_type_ = 0;
        model_type_ = 0;
    }

    void loadModel(const QString &file_path) {
        _params = WHISPER_LIB.whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        _params.progress_callback = [] (whisper_context *ctx, whisper_state *state, int progress, void *user_data){
        };

        QFile file{ file_path };
        file.open(QIODeviceBase::ReadOnly);
        QByteArray bytes;

        bytes = file.readAll();
        ctx_.reset(WHISPER_LIB.whisper_init_from_buffer(bytes.data(), bytes.size()));
        collectInfo();
    }

    QString process(const std::vector<float> &samples) {
        auto num_thread = std::thread::hardware_concurrency();
        WHISPER_LIB.whisper_full_parallel(ctx_.get(),
                                          _params,
                                          samples.data(),
                                          static_cast<int>(samples.size()),
                                          num_thread);

        auto* codec = QTextCodec::codecForName("UTF-8");
        QString s;
        const int n_seg = WHISPER_LIB.whisper_full_n_segments(ctx_.get());
        for (int i = 0; i < n_seg; i++) {
            const char *text = WHISPER_LIB.whisper_full_get_segment_text(ctx_.get(), i);
            s.append(codec->toUnicode(text));
        }
        return s;
    }

    int float_type_=0;
    int model_type_=0;
    whisper_full_params _params;
    WhisperContext ctx_;
};

WhisperService::WhisperService(QObject *parent)
    : QObject(parent) {
}

WhisperService::~WhisperService() {
}

void WhisperService::loadModel(const QString &file_path) {
    whisper_.reset(new Whisper());
    whisper_->loadModel(file_path);
}

void WhisperService::unloadModel() {
    whisper_->unloadModel();
}

void WhisperService::readSamples(const std::vector<float> &samples) {
    emit resultReady(whisper_->process(samples));
}

