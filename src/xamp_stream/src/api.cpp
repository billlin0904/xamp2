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
#include <stream/avfileencoder.h>
#include <stream/bassparametriceq.h>
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
#include <stream/discIdlib.h>
#include <stream/avlib.h>
#include <stream/api.h>

XAMP_STREAM_NAMESPACE_BEGIN
	static bool IsDsdFileChunk(std::string_view const & file_chunks) noexcept {
    static constexpr std::array<std::string_view, 2> knows_chunks{
        "DSD ", // .dsd file
        "FRM8"  // .dsdiff file
    };	
    return std::find(knows_chunks.begin(), knows_chunks.end(), file_chunks)
        != knows_chunks.end();
}

bool IsDsdFile(Path const& path) {
#ifdef XAMP_OS_WIN
    std::ifstream file(path, std::ios_base::binary);
#else
    std::ifstream file(path, std::ios_base::binary);
#endif
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

AlignPtr<FileStream> StreamFactory::MakeFileStream(DsdModes dsd_mode, const Path& file_path) {
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

void LoadBassLib() {
    if (!BASS.IsLoaded()) {
        Singleton<BassLib>::GetInstance().Load();
    }
    BASS.MixLib = MakeAlign<BassMixLib>();
    BASS.DSDLib = MakeAlign<BassDSDLib>();
    BASS.FxLib = MakeAlign<BassFxLib>();
#ifdef XAMP_OS_WIN
    BASS.CDLib = MakeAlign<BassCDLib>();
    try {
        BASS.EncLib = MakeAlign<BassEncLib>();
    }  catch (const Exception &e) {
        XAMP_LOG_DEBUG("Load EncLib error: {}", e.what());
    }
#else
    BASS.CAEncLib = MakeAlign<BassCAEncLib>();
#endif
    BASS.FLACEncLib = MakeAlign<BassFLACEncLib>();
    BASS.LoadVersionInfo();
    for (const auto& info : BASS.GetVersions()) {
        XAMP_LOG_DEBUG("DLL {} version: {}", info.first, info.second);
    }
}

OrderedMap<std::string, std::string> GetBassDLLVersion() {
    return BASS.GetVersions();
}

void LoadFFTLib() {
    Singleton<FFTWLib>::GetInstance();
    Singleton<FFTWFLib>::GetInstance();
}

#ifdef XAMP_OS_WIN
void LoadR8brainLib() {
    Singleton<R8brainLib>::GetInstance();
}
void LoadMBDiscIdLib() {
    Singleton<DiscIdLib>::GetInstance();
}
#endif

void LoadAvLib() {
    Singleton<AvLib>::GetInstance();
}

void LoadSoxrLib() {
    Singleton<SoxrLib>::GetInstance();
}

XAMP_STREAM_NAMESPACE_END
