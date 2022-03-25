#include <base/logger.h>
#include <base/threadpool.h>

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

std::pair<DsdModes, AlignPtr<IAudioStream>> MakeFileStream(Path const& path,
    DeviceInfo const& device_info,
    bool enable_sample_converter) {
    auto const file_path = path.wstring();
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
    auto test_dsd_mode_stream = MediaStreamFactory::MakeAudioStream();
    auto* fs = dynamic_cast<FileStream*>(test_dsd_mode_stream.get());
    auto dsd_mode = SetStreamDsdMode(fs, is_dsd_file, device_info, enable_sample_converter);
    fs->OpenFile(file_path);
    return std::make_pair(dsd_mode, std::move(test_dsd_mode_stream));
}

}
