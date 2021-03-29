#include <fstream>

#include <base/str_utilts.h>
#include <base/memory_mapped_file.h>

#include <output_device/audiodevicemanager.h>

#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <stream/stream_util.h>

#include <base/dataconverter.h>
#include <player/audio_util.h>

namespace xamp::player::audio_util {

DsdDevice * AsDsdDevice(AlignPtr<Device> const &device) noexcept {
    return dynamic_cast<DsdDevice*>(device.get());
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
