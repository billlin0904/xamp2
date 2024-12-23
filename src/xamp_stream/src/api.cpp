#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <stream/basslib.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <stream/ifileencoder.h>
#include <stream/bassaacfileencoder.h>
#include <stream/bassparametriceq.h>
#include <stream/supereqequalizer.h>
#include <stream/mfaacencoder.h>
#include <stream/bassfader.h>
#include <stream/basscddevice.h>
#include <stream/basscompressor.h>
#include <stream/bassequalizer.h>
#include <stream/dspmanager.h>
#include <stream/fftwlib.h>
#include <stream/r8brainlib.h>
#include <stream/soxrlib.h>
#include <stream/srclib.h>
#include <stream/alacencoder.h>
#include <stream/discIdlib.h>
#include <stream/avlib.h>
#include <stream/api.h>

XAMP_STREAM_NAMESPACE_BEGIN
	namespace {
    bool IsDsdFileChunk(const std::string_view & file_chunks) noexcept {
        static constexpr std::array<std::string_view, 2> knows_chunks{
            "DSD ", // .dsd file
            "FRM8"  // .dsdiff file
        };

        for (auto &chunks : knows_chunks) {
            if (file_chunks.find(chunks) != std::string_view::npos) {
                return true;
            }
        }
        return false;
    }

    class FileWriter final : public IFileEncodeWriter {
    public:
        explicit FileWriter(const Path& path) {
            file_.open(path, std::ios::binary);
            if (!file_) {
				throw FileNotFoundException();
            }
        }

        ~FileWriter() override {
            file_.close();
        }

        int64_t Seek(int64_t offset, int whence) override {
			if (!file_.good()) {
				return -1;
			}
			file_.seekp(offset, whence);
			if (!file_) {
				return -1;
			}
			return file_.tellp();
        }

        int32_t Write(const uint8_t* buf, int32_t size) override {
            if (!file_.good()) {
                return -1;
            }
            file_.write(reinterpret_cast<const char*>(buf), size);
            if (!file_) {
                return -1;
            }
            return size;
        }
    private:
        std::ofstream file_;
    };
}

bool IsDsdFile(const Path & path) {
    std::ifstream file(path, std::ios_base::binary);
    if (!file.is_open()) {
        return false;
    }
    std::array<char, 4> buffer{ 0 };
    file.read(buffer.data(), buffer.size());
    if (file.gcount() < 4) {
        return false;
    }
    const std::string_view file_chunks{ buffer.data(), 4 };
    return IsDsdFileChunk(file_chunks);
}

ScopedPtr<FileStream> StreamFactory::MakeFileStream(const Path& file_path, DsdModes dsd_mode) {
    if (!IsCDAFile(file_path)) {
        switch (dsd_mode) {
        case DsdModes::DSD_MODE_NATIVE:
        case DsdModes::DSD_MODE_DOP:
        case DsdModes::DSD_MODE_DSD2PCM:
            return MakeAlign<FileStream, BassFileStream>();
        default:;
        }
        //return MakeAlign<FileStream, AvFileStream>();
        return MakeAlign<FileStream, BassFileStream>();
    } else {
        return MakeAlign<FileStream, BassFileStream>();
    }
}

ScopedPtr<IFileEncoder> StreamFactory::MakeAlacEncoder() {
    return MakeAlign<IFileEncoder, AlacFileEncoder>();
}

ScopedPtr<IFileEncoder> StreamFactory::MakeAACEncoder() {
#ifdef XAMP_OS_WIN
    return MakeAlign<IFileEncoder, MFAACFileEncoder>();
    //return MakeAlign<IFileEncoder, AvFileEncoder>(AvEncodeId::AV_ENCODE_ID_AAC);
#else
    return MakeAlign<IFileEncoder, BassAACFileEncoder>();
#endif
}

ScopedPtr<IAudioProcessor> StreamFactory::MakeParametricEq() {
    return MakeAlign<IAudioProcessor, BassParametricEq>();
}

ScopedPtr<IAudioProcessor> StreamFactory::MakeEqualizer() {
    return MakeAlign<IAudioProcessor, BassEqualizer>();    
}

#ifdef XAMP_OS_WIN
ScopedPtr<IAudioProcessor> StreamFactory::MakeSuperEqEqualizer() {
    return MakeAlign<IAudioProcessor, SuperEqEqualizer>();
}
#endif

ScopedPtr<IAudioProcessor> StreamFactory::MakeCompressor() {
    return MakeAlign<IAudioProcessor, BassCompressor>();
}

ScopedPtr<IAudioProcessor> StreamFactory::MakeFader() {
    return MakeAlign<IAudioProcessor, BassFader>();
}

ScopedPtr<IDSPManager> StreamFactory::MakeDSPManager() {
    return MakeAlign<IDSPManager, DSPManager>();
}

#ifdef XAMP_OS_WIN
ScopedPtr<ICDDevice> StreamFactory::MakeCDDevice(int32_t driver_letter) {
    return MakeAlign<ICDDevice, BassCDDevice>(static_cast<char>(driver_letter));
}
#endif

Vector<EncodingProfile> StreamFactory::GetAvailableEncodingProfile() {
#ifdef XAMP_OS_WIN
    using AACFileEncoder = MFAACFileEncoder;
#else
    using AACFileEncoder = BassAACFileEncoder;
#endif

    return AACFileEncoder::GetAvailableEncodingProfile();
}

IDsdStream* AsDsdStream(ScopedPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* AsFileStream(ScopedPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* AsDsdStream(FileStream* stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream);
}

ScopedPtr<FileStream> MakeFileStream(const Path& file_path, DsdModes dsd_mode) {
    static const std::string kApeFileExtension = ".ape";

    auto file_stream = StreamFactory::MakeFileStream(file_path, dsd_mode);

    if (dsd_mode != DsdModes::DSD_MODE_PCM) {
        if (auto* dsd_stream = AsDsdStream(file_stream)) {
            switch (dsd_mode) {
            case DsdModes::DSD_MODE_DOP:
                ThrowIf<NotSupportFormatException>(
                    dsd_stream->SupportDOP(),
                    "Stream not support mode: {}", dsd_mode);
                break;
            case DsdModes::DSD_MODE_DOP_AA:
                ThrowIf<NotSupportFormatException>(
                    dsd_stream->SupportDOP_AA(),
                    "Stream not support mode: {}", dsd_mode);
                break;
            case DsdModes::DSD_MODE_NATIVE:
                ThrowIf<NotSupportFormatException>(
                    dsd_stream->SupportNativeSD(),
                    "Stream not support mode: {}", dsd_mode);
                break;
            case DsdModes::DSD_MODE_DSD2PCM:
                break;
            case DsdModes::DSD_MODE_AUTO:
                break;
            case DsdModes::DSD_MODE_PCM:
                break;
            default:
                Throw<NotSupportFormatException>(
                    "Not support dsd-mode: {}.", dsd_mode);
                break;
            }
            dsd_stream->SetDSDMode(dsd_mode);
        }
    }

    if (file_path.extension() != kApeFileExtension) {
        for (auto i = 0; i < 1; ++i) {
            try
            {
                file_stream->OpenFile(file_path);
            }
            catch (const FileNotFoundException&) {
                throw;
            }
            catch (const Exception& e) {
                // Fallback other stream
                if (file_stream->GetTypeId() == XAMP_UUID_OF(AvFileStream)) {
                    file_stream = MakeAlign<FileStream, BassFileStream>();
                }
                else {
                    file_stream = MakeAlign<FileStream, AvFileStream>();
                }
                file_stream->OpenFile(file_path);
            }
        }
    }
    else {
        file_stream = MakeAlign<FileStream, BassFileStream>();
        file_stream->OpenFile(file_path);
    }

    return file_stream;
}

void LoadBassLib() {
    if (!BASS_LIB.IsLoaded()) {
        Singleton<BassLib>::GetInstance().Load();
    }
    BASS_LIB.MixLib = MakeAlign<BassMixLib>();
    BASS_LIB.DSDLib = MakeAlign<BassDSDLib>();
    BASS_LIB.FxLib = MakeAlign<BassFxLib>();
#ifdef XAMP_OS_WIN
    BASS_LIB.CDLib = MakeAlign<BassCDLib>();
    try {
        BASS_LIB.EncLib = MakeAlign<BassEncLib>();
    }  catch (const Exception &e) {
        XAMP_LOG_DEBUG("Load EncLib error: {}", e.what());
    }
#else
    BASS_LIB.CAEncLib = MakeAlign<BassCAEncLib>();
#endif
    BASS_LIB.FLACEncLib = MakeAlign<BassFLACEncLib>();
    BASS_LIB.LoadVersionInfo();
    for (const auto& info : BASS_LIB.GetVersions()) {
        XAMP_LOG_DEBUG("DLL {} version: {}", info.first, info.second);
    }
}

std::shared_ptr<IFileEncodeWriter> MakFileEncodeWriter(const Path& file_path) {
	return std::make_shared<FileWriter>(file_path);
}

OrderedMap<std::string, std::string> GetBassDLLVersion() {
    return BASS_LIB.GetVersions();
}

void LoadFFTLib() {
    SharedSingleton<FFTWLib>::GetInstance();
    SharedSingleton<FFTWFLib>::GetInstance();
}

#ifdef XAMP_OS_WIN
void LoadR8brainLib() {
    SharedSingleton<R8brainLib>::GetInstance();
}
void LoadMBDiscIdLib() {
    SharedSingleton<DiscIdLib>::GetInstance();
}
#endif

void LoadAvLib() {
    SharedSingleton<AvLib>::GetInstance();
}

void FreeAvLib() {
    SharedSingleton<AvLib>::GetInstance().Free();
}

void LoadSoxrLib() {
    SharedSingleton<SoxrLib>::GetInstance();
}

void LoadSrcLib() {
    SharedSingleton<SrcLib>::GetInstance();
}

XAMP_STREAM_NAMESPACE_END
