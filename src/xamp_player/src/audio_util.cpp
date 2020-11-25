#include <output_device/audiodevicemanager.h>
#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <player/audio_util.h>

namespace xamp::player {

DsdStream* AsDsdStream(AlignPtr<FileStream> const &stream) noexcept {
    return dynamic_cast<DsdStream*>(stream.get());
}

DsdDevice* AsDsdDevice(AlignPtr<Device> const &device) noexcept {
    return dynamic_cast<DsdDevice*>(device.get());
}

std::vector<float> GetNormalizedPeaks(AlignPtr<FileStream>& stream) {
    constexpr uint32_t kReadSampleSize = 8192 * 4;

    auto num_stream_ch = stream->GetFormat().GetChannels();

    std::vector<float> result(num_stream_ch);
    stream->Seek(0);
    
    Buffer<float> buffer(kReadSampleSize * num_stream_ch);

    while (true) {
        auto num_samples = stream->GetSamples(buffer.Get(), buffer.GetSize());
        if (num_samples == 0) {
            break;
        }
        for (auto i = 0; i < num_samples / num_stream_ch; ++i) {
            if (num_stream_ch == 1) {
                result[0] = (std::max)(buffer[i], result[0]);
            }
            else {
                result[0] = (std::max)(buffer[i], result[0]);
                result[1] = (std::max)(buffer[i+1], result[1]);
            }
        }
    }

    stream->Seek(0);

    return result;
}

AlignPtr<FileStream> MakeFileStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream) {
    static const HashSet<std::wstring_view> dsd_ext{
        {L".dsf"},
        {L".dff"}
    };
    static const HashSet<std::wstring_view> use_bass{
        {L".m4a"},
        {L".ape"},
    };

    const auto is_dsd_stream = dsd_ext.find(file_ext) != dsd_ext.end();
    const auto is_use_bass = use_bass.find(file_ext) != use_bass.end();

    if (old_stream != nullptr) {
        if (is_dsd_stream || is_use_bass) {
            if (auto stream = dynamic_cast<BassFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
        else {
            if (auto stream = dynamic_cast<AvFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
    }

    if (is_dsd_stream || is_use_bass) {
        return MakeAlign<FileStream, BassFileStream>();
    }
    return MakeAlign<FileStream, AvFileStream>();
}

DsdModes SetStreamDsdMode(AlignPtr<FileStream>& stream, const DeviceInfo& device_info, bool use_native_dsd) {
    DsdModes dsd_mode = DsdModes::DSD_MODE_PCM;

    if (auto* dsd_stream = AsDsdStream(stream)) {
        // ASIO device (win32).
        if (AudioDeviceManager::GetInstance().IsASIODevice(device_info.device_type_id)) {
            if (device_info.is_support_dsd) {
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
            if (device_info.is_support_dsd && use_native_dsd) {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DOP);
                XAMP_LOG_DEBUG("Use DOP mode.");
                dsd_mode = DsdModes::DSD_MODE_DOP;
            }
            else {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_PCM);
                XAMP_LOG_DEBUG("Use PCM mode.");
                dsd_mode = DsdModes::DSD_MODE_PCM;
            }
        }
    }
    else {
        dsd_mode = DsdModes::DSD_MODE_PCM;
        XAMP_LOG_DEBUG("Use PCM mode.");
    }

    return dsd_mode;
}

}