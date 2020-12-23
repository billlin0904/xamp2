#include <output_device/audiodevicemanager.h>
#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <base/dataconverter.h>
#include <player/audio_util.h>

namespace xamp::player {

DsdStream* AsDsdStream(AlignPtr<FileStream> const &stream) noexcept {
    return dynamic_cast<DsdStream*>(stream.get());
}

DsdDevice* AsDsdDevice(AlignPtr<Device> const &device) noexcept {
    return dynamic_cast<DsdDevice*>(device.get());
}

std::pair<float, float> GetNormalizedPeaks(AlignPtr<FileStream>& stream) {
    constexpr uint32_t kReadSampleSize = 8192;

    const auto num_stream_ch = stream->GetFormat().GetChannels();

    stream->Seek(0);
    
    Buffer<float> buffer(kReadSampleSize * num_stream_ch);

    std::pair<float, float> result(0, 0);

    while (true) {
        auto num_samples = stream->GetSamples(buffer.Get(), buffer.GetSize());
        if (num_samples == 0) {
            break;
        }
        if (num_stream_ch == 1) {
            for (auto i = 0; i < num_samples / num_stream_ch; ++i) {
                result.first = FloatMaxSSE2(buffer[i], result.first);
            }
        } else {
            for (auto i = 0; i < num_samples / num_stream_ch; ++i) {
                result.first = FloatMaxSSE2(buffer[i], result.first);
                result.second = FloatMaxSSE2(buffer[i + 1], result.second);
            }
        }
    }
    stream->Seek(0);
    return result;
}

AlignPtr<FileStream> MakeFileStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream) {
    static const HashSet<std::wstring_view> dsd_ext{
        {L".dsf"},
        {L".dff"},
    };
    static const HashSet<std::wstring_view> use_bass{
        {L".m4a"},
        {L".ape"},
    };

    const auto is_dsd_stream = dsd_ext.find(file_ext) != dsd_ext.end();
    const auto is_use_bass = use_bass.find(file_ext) != use_bass.end();

    if (old_stream != nullptr) {
        if (is_dsd_stream || is_use_bass) {
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

    if (is_dsd_stream || is_use_bass) {
        return MakeAlign<FileStream, BassFileStream>();
    }
    return MakeAlign<FileStream, AvFileStream>();
}

DsdModes SetStreamDsdMode(AlignPtr<FileStream>& stream, const DeviceInfo& device_info, bool use_native_dsd) {
    auto dsd_mode = DsdModes::DSD_MODE_PCM;

    if (auto* dsd_stream = AsDsdStream(stream)) {
        // ASIO device (win32).
        if (AudioDeviceManager::GetInstance().IsASIODevice(device_info.device_type_id)) {
            if (dsd_stream->IsDsdFile() && device_info.is_support_dsd) {
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
            if (dsd_stream->IsDsdFile() && device_info.is_support_dsd && use_native_dsd) {
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

DsdModes GetStreamDsdMode(std::wstring const& file_path, std::wstring const& file_ext, DeviceInfo const& device_info, bool use_native_dsd) {
    auto test_dsd_mode_stream = MakeFileStream(file_ext);
    test_dsd_mode_stream->OpenFile(file_path);
    return SetStreamDsdMode(test_dsd_mode_stream, device_info, use_native_dsd);
}

}