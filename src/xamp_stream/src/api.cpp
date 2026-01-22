#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/fastiostream.h>

#include <stream/basslib.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/mqafilestream.h>
#include <stream/ifileencoder.h>
#include <stream/bassaacfileencoder.h>
#include <stream/bassparametriceq.h>
#include <stream/supereqequalizer.h>
#include <stream/bassfader.h>
#include <stream/basscddevice.h>
#include <stream/basscompressor.h>
#include <stream/bassequalizer.h>
#include <stream/dspmanager.h>
#include <stream/fftwlib.h>
#include <stream/r8brainlib.h>
#include <stream/soxrlib.h>
#include <stream/srclib.h>
#include <stream/libavencoder.h>
#include <stream/discIdlib.h>
#include <stream/avlib.h>
#include <stream/ebur128scanner.h>
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
}

bool IsDsdFile(const Path & path) {
    FastIOStream file_(path);
    std::array<char, 4> buffer{ 0 };
    auto readbytes = file_.read(buffer.data(), buffer.size());
    if (readbytes < 4) {
        return false;
    }
    const std::string_view file_chunks{ buffer.data(), 4 };
    return IsDsdFileChunk(file_chunks);
}

ScopedPtr<FileStream> StreamFactory::MakeFileStream(const Path& filePath, float rate) {
    auto dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
    if (!IsDsdFile(filePath)) {
        dsd_mode = DsdModes::DSD_MODE_PCM;
    }
	return MakeFileStream(filePath, dsd_mode, rate);
}

ScopedPtr<FileStream> StreamFactory::MakeFileStream(const Path& file_path, DsdModes dsd_mode, float rate) {
    ScopedPtr<FileStream> file_stream;

    try {
        MqaIdentifier identifier(file_path);
        if (identifier.Detect()) {
            if (identifier.IsMQA()) {
                file_stream = MakeAlign<FileStream, MqaFileStream>();
            }
            else {
                file_stream = MakeAlign<FileStream, BassFileStream>();
            }
        }
    }
    catch (...) {
        file_stream = MakeAlign<FileStream, BassFileStream>();
    }

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
    file_stream->OpenFile(file_path, rate);
    return file_stream;
}

ScopedPtr<IFileEncoder> StreamFactory::MakeFileEncoder() {
#ifdef XAMP_OS_WIN
    return MakeAlign<IFileEncoder, LibAbFileEncoder>();
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

IDsdStream* AsDsdStream(ScopedPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* AsFileStream(ScopedPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* AsDsdStream(FileStream* stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream);
}

std::expected<ArchiveFileStream, std::string> StreamFactory::MakeArchiveFileStream(const Path& archive_path, const std::wstring& archive_entry_name, float rate) {
    ArchiveFile file_;
    
    auto enitities = file_.Open(archive_path);
    if (enitities.has_value()) {
        auto archive_entiry = file_.GetEntryByName(archive_entry_name);
        if (archive_entiry.has_value()) {
            auto file_stream = StreamFactory::MakeFileStream(archive_path);
            file_stream->Open(std::move(archive_entiry.value()), rate);
			ArchiveFileStream result;
			result.archive_file = std::move(file_);
			result.file_stream = std::move(file_stream);
			return result;
        }
        return std::unexpected(archive_entiry.error());
    }
    return std::unexpected(enitities.error());
}

ScopedPtr<FileStream> StreamFactory::MakeFileStream(ArchiveEntry archive_entry, DsdModes dsd_mode, float rate) {
    auto file_stream = MakeAlign<FileStream, BassFileStream>();

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
    file_stream->Open(std::move(archive_entry), rate);
    return file_stream;
}

void LoadBassLib() {
    if (!BassLibDLL.IsLoaded()) {
        SharedSingleton<BassLib>::GetInstance().Load();
    }
    BassLibDLL.MixLib = MakeAlign<BassMixLib>();
    BassLibDLL.DSDLib = MakeAlign<BassDSDLib>();
    BassLibDLL.FxLib = MakeAlign<BassFxLib>();
#ifdef XAMP_OS_WIN
    BassLibDLL.CDLib = MakeAlign<BassCDLib>();
    try {
        BassLibDLL.EncLib = MakeAlign<BassEncLib>();
    }  catch (const Exception &e) {
        XAMP_LOG_DEBUG("Load EncLib error: {}", e.what());
    }
#else
    BASS_LIB.CAEncLib = MakeAlign<BassCAEncLib>();
#endif
    BassLibDLL.FLACEncLib = MakeAlign<BassFLACEncLib>();
    BassLibDLL.LoadVersionInfo();
    for (const auto& info : BassLibDLL.GetVersions()) {
        XAMP_LOG_DEBUG("DLL {} version: {}", info.first, info.second);
    }
}

OrderedMap<std::string, std::string> GetBassDLLVersion() {
    return BassLibDLL.GetVersions();
}

void LoadFFTLib() {
#if (USE_INTEL_MKL_LIB)
    SharedSingleton<MKLLib>::GetInstance();
#else
    SharedSingleton<FFTWLib>::GetInstance();
    SharedSingleton<FFTWFLib>::GetInstance();
#endif
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

void LoadEbur128Lib() {
    Ebur128Scanner::LoadEbur128Lib();
}

XAMP_STREAM_NAMESPACE_END
