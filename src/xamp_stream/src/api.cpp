#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>

#include <stream/basslib.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/ifileencoder.h>
#include <stream/basswavfileencoder.h>
#include <stream/bassflacfileencoder.h>
#include <stream/basscddevice.h>
#include <stream/basscompressor.h>
#include <stream/bassequalizer.h>
#include <stream/bassvolume.h>
#include <stream/dspmanager.h>
#include <stream/fftwlib.h>
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

AlignPtr<IAudioStream> MakeAudioStream() {
    return MakeAlign<IAudioStream, BassFileStream>();
}

AlignPtr<IFileEncoder> MakeFlacEncoder() {
    return MakeAlign<IFileEncoder, BassFlacFileEncoder>();
}

#ifdef XAMP_OS_WIN
AlignPtr<IFileEncoder> MakeWavEncoder() {
    return MakeAlign<IFileEncoder, BassWavFileEncoder>();
}

AlignPtr<ICDDevice> MakeCDDevice(int32_t driver_letter) {
    return MakeAlign<ICDDevice, BassCDDevice>(static_cast<char>(driver_letter));
}
#endif
IDsdStream* AsDsdStream(AlignPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* AsDsdStream(IAudioStream* stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream);
}

HashSet<std::string> const& GetSupportFileExtensions() {
    static const auto bass_file_ext = BassFileStream::GetSupportFileExtensions();
    return bass_file_ext;
}

AlignPtr<IAudioProcessor> MakeEqualizer() {
    return MakeAlign<IAudioProcessor, BassEqualizer>();
}

AlignPtr<IAudioProcessor> MakeCompressor() {
    return MakeAlign<IAudioProcessor, BassCompressor>();
}

AlignPtr<IAudioProcessor> MakeVolume() {
    return MakeAlign<IAudioProcessor, BassVolume>();
}

AlignPtr<IDSPManager> MakeDSPManager() {
    return MakeAlign<IDSPManager, DSPManager>();
}

void LoadBassLib() {
    if (!BASS.IsLoaded()) {
        (void)Singleton<BassLib>::GetInstance().Load();
    }
    BASS.MixLib = MakeAlign<BassMixLib>();
    XAMP_LOG_DEBUG("Load BassMixLib {} successfully.", GetBassVersion(BASS.MixLib->BASS_Mixer_GetVersion()));
    BASS.DSDLib = MakeAlign<BassDSDLib>();
    XAMP_LOG_DEBUG("Load BassDSDLib successfully.");
    BASS.FxLib = MakeAlign<BassFxLib>();
    XAMP_LOG_DEBUG("Load BassFxLib successfully.", GetBassVersion(BASS.FxLib->BASS_FX_GetVersion()));
#ifdef XAMP_OS_WIN
    BASS.CDLib = MakeAlign<BassCDLib>();
    XAMP_LOG_DEBUG("Load BassCDLib successfully.");
    BASS.EncLib = MakeAlign<BassEncLib>();
    XAMP_LOG_DEBUG("Load BassEncLib successfully.");
#endif
    BASS.FlacEncLib = MakeAlign<BassFlacEncLib>();
    XAMP_LOG_DEBUG("Load BassFlacEncLib successfully.");
}

void FreeBassLib() {
    BASS.Free();
}

#ifdef XAMP_OS_WIN
void LoadFFTLib() {
    Singleton<FFTWLib>::GetInstance();
    Singleton<FFTWFLib>::GetInstance();
}
#endif

}
