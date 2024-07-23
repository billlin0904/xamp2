#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <base/singleton.h>
#include <QFile>
#include <QTextCodec>
#include <whisper.h>

#include <base/base.h>
#include <base/logger_impl.h>
#include <base/dll.h>
#include <widget/util/str_util.h>
#include <widget/chatgpt/whisperservice.h>

class WhisperLib {
public:
    WhisperLib()
        : module_(OpenSharedLibrary("whisper"))
        , XAMP_LOAD_DLL_API(whisper_full_default_params)
        , XAMP_LOAD_DLL_API(whisper_init_from_buffer_with_params)
        , XAMP_LOAD_DLL_API(whisper_free)
        , XAMP_LOAD_DLL_API(whisper_model_ftype)
        , XAMP_LOAD_DLL_API(whisper_model_type)
        , XAMP_LOAD_DLL_API(whisper_full_parallel)
        , XAMP_LOAD_DLL_API(whisper_full_get_segment_text)
        , XAMP_LOAD_DLL_API(whisper_full_n_segments)
        , XAMP_LOAD_DLL_API(whisper_full)
        , XAMP_LOAD_DLL_API(whisper_context_default_params)
        , XAMP_LOAD_DLL_API(whisper_init_from_file_with_params) {
    }
private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(whisper_full_default_params);
    XAMP_DECLARE_DLL_NAME(whisper_init_from_buffer_with_params);
    XAMP_DECLARE_DLL_NAME(whisper_free);
    XAMP_DECLARE_DLL_NAME(whisper_model_ftype);
    XAMP_DECLARE_DLL_NAME(whisper_model_type);
    XAMP_DECLARE_DLL_NAME(whisper_full_parallel);
    XAMP_DECLARE_DLL_NAME(whisper_full_get_segment_text);
    XAMP_DECLARE_DLL_NAME(whisper_full_n_segments);
    XAMP_DECLARE_DLL_NAME(whisper_full);
    XAMP_DECLARE_DLL_NAME(whisper_context_default_params);
    XAMP_DECLARE_DLL_NAME(whisper_init_from_file_with_params);
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
        context_params_ = WHISPER_LIB.whisper_context_default_params();
        context_params_.use_gpu = true;

        /*QFile file{ file_path };
        if (!file.open(QIODeviceBase::ReadOnly)) {
            return;
        }
        QByteArray bytes;
        bytes = file.readAll();
        if (bytes.size() == 0) {
            file.close();
            return;
        }
        ctx_.reset(WHISPER_LIB.whisper_init_from_buffer_with_params(bytes.data(), bytes.size(), context_params_));
        collectInfo();
        file.close();*/
        auto utf8_file_path = file_path.toStdString();
        ctx_.reset(WHISPER_LIB.whisper_init_from_file_with_params(utf8_file_path.c_str(), context_params_));
        collectInfo();
    }

    QString process(const std::vector<float> &samples) {
        auto params = WHISPER_LIB.whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

        params.language = "chinese";
        params.duration_ms = 10000;
        params.translate = false;
        params.progress_callback = [](whisper_context* ctx, whisper_state* state, int progress, void* user_data) {
            XAMP_LOG_DEBUG("Process {}", progress);
            };

        if (WHISPER_LIB.whisper_full_parallel(ctx_.get(),
            params,
            samples.data(),
            static_cast<int>(samples.size()),
            params.n_threads) != 0) {
            return kEmptyString;
        }

        /*if (WHISPER_LIB.whisper_full(ctx_.get(),
            params,
            (const float*)samples.data(),
            static_cast<int>(samples.size())) != 0) {
            return kEmptyString;
        }*/

        auto* codec = QTextCodec::codecForName("UTF-8");
        QString s;
        const int n_seg = WHISPER_LIB.whisper_full_n_segments(ctx_.get());
        for (int i = 0; i < n_seg; i++) {
            const char *text = WHISPER_LIB.whisper_full_get_segment_text(ctx_.get(), i);
            s.append(codec->toUnicode(text));
        }

        XAMP_LOG_DEBUG("{}", s.toStdString());
        return s;
    }

    int float_type_=0;
    int model_type_=0;
    whisper_context_params context_params_;
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

