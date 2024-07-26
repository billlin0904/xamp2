#include <widget/chatgpt/librosa.h>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#define slots Q_SLOTS

#include <cstdlib>

namespace py = pybind11;


XAMP_DECLARE_LOG_NAME(LibrosaService);
XAMP_DECLARE_LOG_NAME(LibrosaInterop);

class LibrosaInterop::LibrosaInteropImpl {
public:
    LibrosaInteropImpl() {
        feature_ = py::none();
        logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(LibrosaInterop));
    }

    bool initial() {
        if (!feature_.is_none()) {
            return false;
        }
        module_ = py::module_::import("librosa");
        numpy_ = py::module_::import("librosa");
        feature_ = module_.attr("feature");
        return true;
    }

    std::pair<std::vector<float>, double> load(const std::string& path,
                                               double sr=22050,
                                               bool mono=true,
                                               int offset=0,
                                               int duration=0) {
        py::object result = module_.attr("load")(path,
                                                 sr,
                                                 mono,
                                                 offset,
                                                 duration,
                                                 py::arg("dtype") = numpy_.attr("float32"),
                                                 py::arg("res_type") = "soxr_hq");
        auto audio_data = result.cast<std::pair<py::array_t<float>, double>>();
        std::vector<float> audio_vector(audio_data.first.size());
        std::memcpy(audio_vector.data(), audio_data.first.data(), audio_data.first.size() * sizeof(float));
        return std::make_pair(audio_vector, audio_data.second);
    }

    std::vector<std::vector<float>> melspectrogram(const std::vector<float>& y,
                                                   double sr=22050,
                                                   int n_fft=2048,
                                                   int hop_length=512,
                                                   int n_mels=128,
                                                   std::optional<int> win_length = std::nullopt,
                                                   std::optional<double> fmin = std::nullopt,
                                                   std::optional<double> fmax = std::nullopt) {
        py::array_t<float> y_array(y.size(), y.data());
        py::object result = feature_.attr("melspectrogram")(y_array,
                                                            sr,
                                                            n_fft,
                                                            hop_length,
                                                            win_length,
                                                            n_mels,
                                                            fmin,
                                                            fmax);
        auto spectrogram = result.cast<py::array_t<float>>();
        std::vector<std::vector<float>> spec_vector(spectrogram.shape(0), std::vector<float>(spectrogram.shape(1)));
        for (ssize_t i = 0; i < spectrogram.shape(0); ++i) {
            for (ssize_t j = 0; j < spectrogram.shape(1); ++j) {
                spec_vector[i][j] = *spectrogram.data(i, j);
            }
        }
        return spec_vector;
    }

    std::vector<std::vector<float>> power_to_db(const std::vector<std::vector<float>>& S,
                                                double ref=1.0,
                                                double amin=1e-10,
                                                double top_db=80.0) {
        py::array_t<float> S_array({S.size(), S[0].size()});
        for (size_t i = 0; i < S.size(); ++i) {
            for (size_t j = 0; j < S[i].size(); ++j) {
                *S_array.mutable_data(i, j) = S[i][j];
            }
        }
        py::object result = module_.attr("power_to_db")(S_array, ref, amin, top_db);
        auto db_spectrogram = result.cast<py::array_t<float>>();
        std::vector<std::vector<float>> db_vector(db_spectrogram.shape(0), std::vector<float>(db_spectrogram.shape(1)));
        for (ssize_t i = 0; i < db_spectrogram.shape(0); ++i) {
            for (ssize_t j = 0; j < db_spectrogram.shape(1); ++j) {
                db_vector[i][j] = *db_spectrogram.data(i, j);
            }
        }
        return db_vector;
    }

    py::module module_;
    py::module numpy_;
    py::object feature_;
    LoggerPtr logger_;
};

LibrosaInterop::LibrosaInterop()
    : impl_(MakeAlign<LibrosaInteropImpl>()) {
}

LibrosaInterop::~LibrosaInterop() {

}

bool LibrosaInterop::initial() {
    return impl_->initial();
}

std::pair<std::vector<float>, double> LibrosaInterop::load(const std::string& path,
                                           double s,
                                           bool mono,
                                           int offset,
                                           int duration) {
    return impl_->load(path, s, mono, offset, duration);
}

std::vector<std::vector<float>> LibrosaInterop::melspectrogram(const std::vector<float>& y,
                                                               double sr,
                                                               int n_fft,
                                                               int hop_length,
                                                               int n_mels,
                                                               std::optional<int> win_length,
                                                               std::optional<double> fmin,
                                                               std::optional<double> fmax) {
    return impl_->melspectrogram(y, sr, n_fft, hop_length, n_mels, win_length, fmin, fmax);
}

std::vector<std::vector<float>> LibrosaInterop::power_to_db(const std::vector<std::vector<float>>& S,
                                            double ref,
                                            double amin,
                                            double top_db) {
    return impl_->power_to_db(S, ref, amin, top_db);
}

LibrosaService::LibrosaService(QObject* parent)
    : BaseService(parent) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(LibrosaService));
}

LibrosaInterop* LibrosaService::interop() {
    return interop_.get();
}

QFuture<bool> LibrosaService::initialAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
        return interop()->initial();
    });
}

QFuture<bool> LibrosaService::loadAsync(const QString &path) {
    return invokeAsync([this, path]() {
        py::gil_scoped_acquire guard{};
        auto [y, sample_rate] = interop()->load(path.toStdString(), 88200, false);
        if (sample_rate != 44100) {

        }
        return true;
    });
}
