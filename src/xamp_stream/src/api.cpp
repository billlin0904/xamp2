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
#include <stream/avlib.h>
#include <stream/api.h>

namespace xamp::stream {

using namespace xamp::base;

static bool TestDsdFileFormat(std::string_view const & file_chunks) noexcept {
    static constexpr std::array<std::string_view, 2> knows_chunks{
        "DSD ", // .dsd file
        "FRM8"  // .dsdiff file
    };	
    return std::find(knows_chunks.begin(), knows_chunks.end(), file_chunks)
        != knows_chunks.end();
}

bool TestDsdFileFormatStd(std::wstring const& file_path) {
#ifdef XAMP_OS_WIN
    std::ifstream file(file_path, std::ios_base::binary);
#else
    std::ifstream file(String::ToString(file_path), std::ios_base::binary);
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
    return TestDsdFileFormat(file_chunks);
}

AlignPtr<FileStream> StreamFactory::MakeFileStream() {
#ifdef XAMP_OS_WIN
    return MakeAlign<FileStream, AvFileStream>();
#else
    return MakeAlign<FileStream, BassFileStream>();
#endif
}

AlignPtr<IFileEncoder> StreamFactory::MakeFlacEncoder() {
    return MakeAlign<IFileEncoder, BassFlacFileEncoder>();
}

AlignPtr<IFileEncoder> StreamFactory::MakeAACEncoder() {
#ifdef XAMP_OS_WIN
    return MakeAlign<IFileEncoder, MFAACFileEncoder>();
#else
    return MakeAlign<IFileEncoder, BassAACFileEncoder>();
#endif
}


AlignPtr<IFileEncoder> StreamFactory::MakeWaveEncoder() {
    return MakeAlign<IFileEncoder, BassWavFileEncoder>();
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

const HashSet<std::string> & GetSupportFileExtensions() {
    static const auto file_ext_lut = BASS.GetSupportFileExtensions();
    return file_ext_lut;
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
#else
    BASS.CAEncLib = MakeAlign<BassCAEncLib>();
#endif
    try {
        BASS.EncLib = MakeAlign<BassEncLib>();
    }  catch (const Exception &e) {
        XAMP_LOG_DEBUG("Load EncLib error: {}", e.what());
    }
    BASS.FLACEncLib = MakeAlign<BassFLACEncLib>();
    BASS.LoadVersionInfo();
    for (const auto& info : BASS.GetVersions()) {
        XAMP_LOG_DEBUG("DLL {} ver: {}", info.first, info.second);
    }
}

std::map<std::string, std::string> GetBassDLLVersion() {
    return BASS.GetVersions();
}

void LoadFFTLib() {
    Singleton<FFTWLib>::GetInstance();
    //Singleton<FFTWFLib>::GetInstance();
}

#ifdef XAMP_OS_WIN
void LoadR8brainLib() {
    Singleton<R8brainLib>::GetInstance();
}
void LoadAvLib() {
    Singleton<AvLib>::GetInstance();
}
#endif

void LoadSoxrLib() {
    Singleton<SoxrLib>::GetInstance();
}

}
