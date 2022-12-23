#include <base/logger.h>
#include <base/threadpoolexecutor.h>

#include <output_device/api.h>

#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/api.h>

#include <player/audio_player.h>
#include <player/audio_util.h>

namespace xamp::player::audio_util {

std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(Path const& path,
    DeviceInfo const& device_info,
    bool enable_sample_converter) {
    auto const file_path = path.wstring();
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
    AlignPtr<FileStream> file_stream;
    auto dsd_mode = DsdModes::DSD_MODE_PCM;
    if (is_dsd_file) {
        if (device_info.is_support_dsd && !enable_sample_converter) {
            if (IsASIODevice(device_info.device_type_id)) {
                file_stream = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_NATIVE);
                dsd_mode = DsdModes::DSD_MODE_NATIVE;
            } else {
                file_stream = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_DOP);
                dsd_mode = DsdModes::DSD_MODE_DOP;
            }
            if (auto* dsd_stream = AsDsdStream(file_stream)) {
                dsd_stream->SetDSDMode(dsd_mode);
            }
        } else {
            file_stream = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_DSD2PCM);
            dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
        }
    } else {
        file_stream = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_PCM);
        dsd_mode = DsdModes::DSD_MODE_PCM;
    }
    file_stream->OpenFile(file_path);
    return std::make_pair(dsd_mode, std::move(file_stream));
}

}
