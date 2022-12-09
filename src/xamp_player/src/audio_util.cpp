#include <base/logger.h>
#include <base/threadpoolexecutor.h>

#include <output_device/api.h>

#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/api.h>

#include <player/audio_player.h>
#include <player/audio_util.h>

namespace xamp::player::audio_util {

static DsdModes SetStreamDsdMode(FileStream *stream, bool is_dsd_file, const DeviceInfo& device_info, bool enable_sample_converter) {
    auto dsd_mode = DsdModes::DSD_MODE_PCM;

    if (is_dsd_file) {
        if (auto* dsd_stream = AsDsdStream(stream)) {
            if (is_dsd_file && device_info.is_support_dsd && !enable_sample_converter) {
                if (IsASIODevice(device_info.device_type_id)) {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_NATIVE);
                    dsd_mode = DsdModes::DSD_MODE_NATIVE;
                }
                else {
                    dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DOP);
                    dsd_mode = DsdModes::DSD_MODE_DOP;
                }
            }
            else {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
                dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
            }
        }
    } else {
        dsd_mode = DsdModes::DSD_MODE_PCM;
    }

    return dsd_mode;
}

std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(Path const& path,
    DeviceInfo const& device_info,
    bool enable_sample_converter) {
    auto const file_path = path.wstring();
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
    auto file_stream = StreamFactory::MakeFileStream();
    auto dsd_mode = SetStreamDsdMode(file_stream.get(), is_dsd_file, device_info, enable_sample_converter);
    file_stream->OpenFile(file_path);
    return std::make_pair(dsd_mode, std::move(file_stream));
}

}
