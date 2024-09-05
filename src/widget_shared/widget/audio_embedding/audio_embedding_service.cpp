#include <widget/audio_embedding/audio_embedding_service.h>

#if 0
#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#define slots Q_SLOTS

#include <cstdlib>

namespace py = pybind11;

namespace {
    void dumpObject(LoggerPtr logger, const py::object& obj) {
        XAMP_LOG_D(logger, "{}", py::str(obj).cast<std::string>());
    }
}

XAMP_DECLARE_LOG_NAME(AudioEmbeddingService);
XAMP_DECLARE_LOG_NAME(AudioEmbeddingDataBaseInterop);

class AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInteropImpl {
public:
    AudioEmbeddingDataBaseInteropImpl();

    bool initial();

    bool embedAndSave(const std::string& path);

    std::vector<AudioEmbeddingQueryResult> queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results);
private:
    std::string api_key_;
    py::module embedding_;
    py::object embedding_database_;
    LoggerPtr logger_;
};

AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInteropImpl::AudioEmbeddingDataBaseInteropImpl() {
    embedding_database_ = py::none();
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(AudioEmbeddingDataBaseInterop));
}

bool AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInteropImpl::initial() {
    if (!embedding_database_.is_none()) {
        return false;
    }

    /*auto llvmlite = py::module_::import("llvmlite");
    auto numba = py::module_::import("numba");
    auto librosa = py::module_::import("librosa");
    auto numpy = py::module_::import("numpy");
    auto panns_inference = py::module_::import("panns_inference");
    auto chromadb = py::module_::import("chromadb");
	auto audio_tagging = panns_inference.attr("AudioTagging")();*/
    embedding_ = py::module_::import("embedding");
    embedding_database_ = embedding_.attr("AudioEmbeddingDatabase")();
    return true;
}

bool AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInteropImpl::embedAndSave(const std::string& path) {
	embedding_database_.attr("embedAndSave")(path);
    return true;
}

std::vector<AudioEmbeddingQueryResult> AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInteropImpl::queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results) {
    std::vector<AudioEmbeddingQueryResult> res;
    py::object results = embedding_database_.attr("query_embeddings")(py::make_tuple(paths));
    auto result_list = results.cast<std::vector<std::pair<std::string, float>>>();
    for (auto item : result_list) {
		res.push_back({ item.first, item.second });
    }
    return res;
}

AudioEmbeddingDataBaseInterop::AudioEmbeddingDataBaseInterop()
    : impl_(MakeAlign<AudioEmbeddingDataBaseInteropImpl>()) {
}

XAMP_PIMPL_IMPL(AudioEmbeddingDataBaseInterop)

bool AudioEmbeddingDataBaseInterop::initial() {
	return impl_->initial();
}

bool AudioEmbeddingDataBaseInterop::embedAndSave(const std::string& path) {
    return impl_->embedAndSave(path);
}

std::vector<AudioEmbeddingQueryResult> AudioEmbeddingDataBaseInterop::queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results) {
    return impl_->queryEmbeddings(paths, n_results);
}

AudioEmbeddingService::AudioEmbeddingService(QObject* parent)
    : BaseService(parent) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(AudioEmbeddingService));
}

AudioEmbeddingDataBaseInterop* AudioEmbeddingService::interop() {
    return interop_.get();
}

QFuture<bool> AudioEmbeddingService::initialAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
		return interop()->initial();
        });
}

QFuture<bool> AudioEmbeddingService::embedAndSave(const std::string& path) {
    return invokeAsync([this, path]() {
        py::gil_scoped_acquire guard{};
        return interop()->embedAndSave(path);
        });
}

QFuture<std::vector<AudioEmbeddingQueryResult>> AudioEmbeddingService::queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results) {
    return invokeAsync([this, paths, n_results]() {
        py::gil_scoped_acquire guard{};
        return interop()->queryEmbeddings(paths, n_results);
        });
}

void AudioEmbeddingService::cancelRequested() {
    is_stop_ = true;
}

QFuture<bool> AudioEmbeddingService::cleanupAsync() {
    return invokeAsync([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        py::gil_scoped_acquire guard{};
        interop_.reset();
        return true;
        }, InvokeType::INVOKE_IMMEDIATELY);
}

#endif

#include <widget/util/json_util.h>

AudioEmbeddingService::AudioEmbeddingService(QObject* parent)
    : QObject(parent) {
}

AudioEmbeddingService::~AudioEmbeddingService() {
}

void AudioEmbeddingService::embedAndSave(const QString& path, int32_t audio_id) {
    QVariantMap content;
    content[qTEXT("file_path")] = path;
    content[qTEXT("audio_id")] = qFormat("%1").arg(audio_id);
    const auto json = json_util::serialize(content);
    http::HttpClient(qTEXT("http://127.0.0.1:8000/embed_and_save"))
        .json(json)
        .post();
}

void AudioEmbeddingService::queryEmbeddings(const QList<QString> &paths) {
    QMultiMap<QString, QVariant> params;
    Q_FOREACH(auto path, paths) {
        params.insert(qTEXT("paths"), path);
    }
    http::HttpClient(qTEXT("http://127.0.0.1:8000/query_embeddings"))
        .params(params)
        .get();
}
