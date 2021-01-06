#include <base/str_utilts.h>
#include <base/memory_mapped_file.h>

#include <output_device/audiodevicemanager.h>

#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>

#include <base/dataconverter.h>
#include <player/audio_util.h>

namespace xamp::player {

static bool TestDsdFileFormat(MemoryMappedFile &file) {
	if (file.GetLength() < 4) {
        return false;
	}
	
    constexpr std::string_view dsd_chunks{ "DSD " };
    constexpr std::string_view dsdiff_chunks{ "FRM8" };
	
    return memcmp(file.GetData(), dsd_chunks.data(), 4) == 0
	|| memcmp(file.GetData(), dsdiff_chunks.data(), 4) == 0;
}

static bool TestDsdFileFormat(std::wstring const& file_path) {
    MemoryMappedFile file;
    file.Open(file_path);
    return TestDsdFileFormat(file);
}

template <typename T>
static HashSet<std::wstring> GetSupportFileExtensions() {
    HashSet<std::wstring> file_ext;
	for (const auto & ext : T::GetSupportFileExtensions()) {
        file_ext.insert(ToStdWString(ext));
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
    return MakeAlign<FileStream, BassFileStream>();    
}

DsdModes SetStreamDsdMode(AlignPtr<FileStream>& stream, bool is_dsd_file, const DeviceInfo& device_info, bool use_native_dsd) {
    auto dsd_mode = DsdModes::DSD_MODE_PCM;

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
            if (is_dsd_file && device_info.is_support_dsd && use_native_dsd) {
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
    else {
        dsd_mode = DsdModes::DSD_MODE_PCM;
        XAMP_LOG_DEBUG("Use PCM mode.");
    }

    return dsd_mode;
}

std::vector<std::string> GetSupportFileExtensions() {
    auto av = AvFileStream::GetSupportFileExtensions();
    XAMP_LOG_DEBUG("Get AvFileStream support file ext: {}", Join(av));
    std::sort(av.begin(), av.end());

	auto bass = BassFileStream::GetSupportFileExtensions();
    XAMP_LOG_DEBUG("Get BassFileStream support file ext: {}", Join(av));
    std::sort(bass.begin(), bass.end());
	
    std::vector<std::string> v;
    std::set_union(av.begin(), av.end(),
        bass.begin(), bass.end(),
        std::back_inserter(v));
    return v;
}

std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(std::wstring const& file_path,
    std::wstring const& file_ext,
    DeviceInfo const& device_info,
    bool use_native_dsd) {
    auto is_dsd_file = TestDsdFileFormat(file_path);
    auto test_dsd_mode_stream = MakeStream(file_ext);    
    auto dsd_mode = SetStreamDsdMode(test_dsd_mode_stream, is_dsd_file, device_info, use_native_dsd);
    test_dsd_mode_stream->OpenFile(file_path);
    return std::make_pair(dsd_mode, std::move(test_dsd_mode_stream));
}

}
