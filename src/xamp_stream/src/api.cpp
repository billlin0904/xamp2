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
#include <stream/basswavfileencoder.h>
#include <stream/bassflacfileencoder.h>
#include <stream/bassaacfileencoder.h>
#include <stream/bassparametriceq.h>
#include <stream/supereqequalizer.h>
#include <stream/mfaacencoder.h>
#include <stream/bassfader.h>
#include <stream/basscddevice.h>
#include <stream/basscompressor.h>
#include <stream/bassequalizer.h>
#include <stream/bassvolume.h>
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

AlignPtr<FileStream> StreamFactory::MakeFileStream(const Path& file_path, DsdModes dsd_mode) {
    if (!IsCDAFile(file_path)) {
        switch (dsd_mode) {
        case DsdModes::DSD_MODE_NATIVE:
        case DsdModes::DSD_MODE_DOP:
        case DsdModes::DSD_MODE_DSD2PCM:
            return MakeAlign<FileStream, BassFileStream>();
        default:;
        }
        return MakeAlign<FileStream, AvFileStream>();
        //return MakeAlign<FileStream, BassFileStream>();
    } else {
        return MakeAlign<FileStream, BassFileStream>();
    }
}

AlignPtr<IFileEncoder> StreamFactory::MakeFlacEncoder() {
    return MakeAlign<IFileEncoder, BassFlacFileEncoder>();
}

AlignPtr<IFileEncoder> StreamFactory::MakeAlacEncoder() {
    return MakeAlign<IFileEncoder, AlacFileEncoder>();
}

AlignPtr<IFileEncoder> StreamFactory::MakeAACEncoder() {
#ifdef XAMP_OS_WIN
    return MakeAlign<IFileEncoder, MFAACFileEncoder>();
    //return MakeAlign<IFileEncoder, AvFileEncoder>(AvEncodeId::AV_ENCODE_ID_AAC);
#else
    return MakeAlign<IFileEncoder, BassAACFileEncoder>();
#endif
}

AlignPtr<IFileEncoder> StreamFactory::MakeWaveEncoder() {
    return MakeAlign<IFileEncoder, BassWavFileEncoder>();
}

AlignPtr<IAudioProcessor> StreamFactory::MakeParametricEq() {
    return MakeAlign<IAudioProcessor, BassParametricEq>();
}

AlignPtr<IAudioProcessor> StreamFactory::MakeEqualizer() {
    return MakeAlign<IAudioProcessor, BassEqualizer>();    
}

#ifdef XAMP_OS_WIN
AlignPtr<IAudioProcessor> StreamFactory::MakeSuperEqEqualizer() {
    return MakeAlign<IAudioProcessor, SuperEqEqualizer>();
}
#endif

AlignPtr<IAudioProcessor> StreamFactory::MakeCompressor() {
    return MakeAlign<IAudioProcessor, BassCompressor>();
}

AlignPtr<IAudioProcessor> StreamFactory::MakeFader() {
    return MakeAlign<IAudioProcessor, BassFader>();
}

AlignPtr<IAudioProcessor> StreamFactory::MakeVolume() {
    return MakeAlign<IAudioProcessor, BassVolume>();
}

AlignPtr<IDSPManager> StreamFactory::MakeDSPManager() {
    return MakeAlign<IDSPManager, DSPManager>();
}

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> StreamFactory::MakeCDDevice(int32_t driver_letter) {
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

IDsdStream* AsDsdStream(AlignPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* AsDsdStream(FileStream* stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream);
}

AlignPtr<FileStream> MakeFileStream(const Path& file_path, DsdModes dsd_mode) {
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
