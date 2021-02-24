#include <fstream>

#include <base/str_utilts.h>
#include <base/memory_mapped_file.h>

#include <output_device/audiodevicemanager.h>

#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>

#include <base/dataconverter.h>
#include <player/audio_util.h>

namespace xamp::player::audio_util {

static bool TestDsdFileFormat(std::string_view const & file_chunks) noexcept {
    static constexpr std::array<std::string_view, 2> knows_chunks{
        "DSD ", // .dsd file
        "FRM8"  // .dsdiff fille
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

bool TestDsdFileFormat(std::wstring const& file_path) {
    MemoryMappedFile file;
    file.Open(file_path);
    if (file.GetLength() < 4) {
        return false;
    }
    const std::string_view file_chunks{ static_cast<char const*>(file.GetData()), 4 };
    return TestDsdFileFormat(file_chunks);
}

template <typename T>
static HashSet<std::wstring> GetSupportFileExtensions() {
    HashSet<std::wstring> file_ext;
	for (const auto & ext : T::GetSupportFileExtensions()) {
        file_ext.insert(String::ToStdWString(ext));
	}
    return file_ext;
}

DsdStream* AsDsdStream(AlignPtr<FileStream> const &stream) noexcept {
    return dynamic_cast<DsdStream*>(stream.get());
}

DsdDevice* AsDsdDevice(AlignPtr<Device> const &device) noexcept {
    return dynamic_cast<DsdDevice*>(device.get());
}

AlignPtr<FileStream> MakeStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream) {
    static const HashSet<std::wstring> use_av = GetSupportFileExtensions<AvFileStream>();
    static const HashSet<std::wstring> use_bass = GetSupportFileExtensions<BassFileStream>();

    const auto is_use_av_stream = use_av.find(file_ext) != use_av.end();
    const auto is_use_bass_stream = use_bass.find(file_ext) != use_bass.end();

    if (old_stream != nullptr) {
        if (is_use_av_stream || is_use_bass_stream) {
            if (auto* stream = dynamic_cast<BassFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
        else {
            if (auto* stream = dynamic_cast<AvFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
    }

    if (is_use_av_stream) {
        return MakeAlign<FileStream, AvFileStream>();
    }
    /*if (is_use_bass_stream) {
        return MakeAlign<FileStream, BassFileStream>();
    }*/
    return MakeAlign<FileStream, AvFileStream>();
}

DsdModes SetStreamDsdMode(AlignPtr<FileStream>& stream, bool is_dsd_file, const DeviceInfo& device_info) {
    auto dsd_mode = DsdModes::DSD_MODE_PCM;

    if (is_dsd_file) {
        if (auto* dsd_stream = AsDsdStream(stream)) {
            // ASIO device (win32).
            if (AudioDeviceManager::GetInstance().IsASIODevice(device_info.device_type_id)) {
                if (is_dsd_file && device_info.is_support_dsd) {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_NATIVE);
                    dsd_mode = DsdModes::DSD_MODE_NATIVE;
                    XAMP_LOG_DEBUG("Use Native DSD mode.");
                }
                else {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_PCM);
                    dsd_mode = DsdModes::DSD_MODE_PCM;
                    XAMP_LOG_DEBUG("Use PCM mode.");
                }
            }
            // WASAPI or CoreAudio old device.
            else {
                if (is_dsd_file && device_info.is_support_dsd) {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DOP);
                    XAMP_LOG_DEBUG("Use DOP mode.");
                    dsd_mode = DsdModes::DSD_MODE_DOP;
                }
                else {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
                    XAMP_LOG_DEBUG("Use DSD2PCM mode.");
                    dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
                }
            }
        }
    } else {
        dsd_mode = DsdModes::DSD_MODE_PCM;
        XAMP_LOG_DEBUG("Use PCM mode.");
    }

    return dsd_mode;
}

static HashSet<std::string> GetStreamSupportFileExtensions() {
	const auto av_file_ext = AvFileStream::GetSupportFileExtensions();
    XAMP_LOG_TRACE("Get AvFileStream support file ext: {}", String::Join(av_file_ext));

    const auto bass_file_ext = BassFileStream::GetSupportFileExtensions();
    XAMP_LOG_TRACE("Get BassFileStream support file ext: {}", String::Join(bass_file_ext));

    HashSet<std::string> support_file_ext;

    for (const auto & file_ext : Union<std::string>(av_file_ext, bass_file_ext)) {
        support_file_ext.insert(file_ext);
    }
    return support_file_ext;
}

HashSet<std::string> const & GetSupportFileExtensions() {
    static auto const support_file_ext = GetStreamSupportFileExtensions();
    return support_file_ext;
}

std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(std::wstring const& file_path,
    std::wstring const& file_ext,
    DeviceInfo const& device_info) {
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
    auto test_dsd_mode_stream = MakeStream(file_ext);    
    auto dsd_mode = SetStreamDsdMode(test_dsd_mode_stream, is_dsd_file, device_info);
    test_dsd_mode_stream->OpenFile(file_path);
    return std::make_pair(dsd_mode, std::move(test_dsd_mode_stream));
}

}
