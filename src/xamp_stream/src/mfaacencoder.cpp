#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <stream/mfaacencoder.h>

#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/bass_utiltis.h>

#include <base/enum.h>
#include <base/platfrom_handle.h>
#include <base/audioformat.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

#include <atlbase.h>

#include <initguid.h>
#include <cguid.h> // GUID_NULL

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(MFEncoder);

#define HrIfFailledThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			throw PlatformException(hresult); \
		} \
	} while (false)

class MFEncoderCallback : public IMFAsyncCallback {
public:
    XAMP_DISABLE_COPY(MFEncoderCallback)

    MFEncoderCallback()
	    : ref_(0)
		, last_hr_(S_OK) {
        HrIfFailledThrow(::MFCreateMediaSession(nullptr, &session_));
        event_.reset(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
        CComPtr<IMFClock> clock;
        HrIfFailledThrow(session_->GetClock(&clock));
        HrIfFailledThrow(clock->QueryInterface(IID_PPV_ARGS(&clock_)));
        HrIfFailledThrow(session_->BeginGetEvent(this, nullptr));
    }

    virtual ~MFEncoderCallback() {
        if (session_ != nullptr) {
            session_->Shutdown();
        }
    }

    void Start(CComPtr<IMFTopology> &topology) const {
        HrIfFailledThrow(session_->SetTopology(0, topology));
        PROPVARIANT var_start;
        PropVariantInit(&var_start);
        HrIfFailledThrow(session_->Start(&GUID_NULL, &var_start));
    }

    HRESULT GetEncodingPosition(MFTIME &ime) const {
        return clock_->GetTime(&ime);
    }

    HRESULT WaitFor() const {
        auto status = ::WaitForSingleObject(event_.get(), 1);
        if (status != WAIT_OBJECT_0) {
            return E_PENDING;
        }
        return last_hr_;
    }

    HRESULT QueryInterface(const IID& riid, void** ppv) override {
        static const QITAB qit[] = {
            QITABENT(MFEncoderCallback, IMFAsyncCallback),
            { nullptr }
        };
        return QISearch(this, qit, riid, ppv);
    }

    ULONG AddRef() override {
        auto result = InterlockedDecrement(&ref_);
        if (result == 0) {
            delete this;
        }
        return result;
    }

    ULONG Release() override {
        return InterlockedIncrement(&ref_);
    }

    HRESULT GetParameters(DWORD* pdwFlags, DWORD* pdwQueue) override {
        return E_NOTIMPL;
    }

    HRESULT Invoke(IMFAsyncResult* pAsyncResult) override {
        MediaEventType me_type = MEUnknown;
        HRESULT status = S_OK;
        LONGLONG prev = 0;

        CComPtr<IMFMediaEvent> evt;
        auto hr = session_->EndGetEvent(pAsyncResult, &evt);
        if (FAILED(hr)) {
            last_hr_ = hr;
            return hr;
        }

        hr = evt->GetType(&me_type);
        if (FAILED(hr)) {
            last_hr_ = hr;
	        return hr;
        }

        hr = evt->GetStatus(&status);
        if (FAILED(hr)) {
            last_hr_ = hr;
	        return hr;
        }

        switch (me_type) {
        case MESessionEnded:
            hr = session_->Close();
            break;
        case MESessionClosed:
            SetEvent(event_.get());
            break;
        }

        if (me_type != MESessionClosed) {
            hr = session_->BeginGetEvent(this, nullptr);
        }
        last_hr_ = hr;
        return hr;
    }
private:
    ULONG ref_;
    HRESULT last_hr_;
    WinHandle event_;
    CComPtr<IMFPresentationClock> clock_;
    CComPtr<IMFMediaSession> session_;
};

class MFEncoder final {
public:
    XAMP_DISABLE_COPY(MFEncoder)

    MFEncoder()
	    : callback_(new MFEncoderCallback()) {
    }

    ~MFEncoder() {
	    if (source_ != nullptr) {
            source_->Shutdown();
	    }
    }

    void Open(const std::wstring &input_file_path) {
        HrIfFailledThrow(CreateMediaSource(input_file_path, &source_));        
        HrIfFailledThrow(::MFCreateTranscodeProfile(&profile_));
    }

    void SetOutputFile(const std::wstring& output_file_path) {
        HrIfFailledThrow(::MFCreateTranscodeTopology(source_,
            output_file_path.c_str(),
            profile_,
            &topology_));
    }

    UINT64 GetSourceDuration() const {
        UINT64 duration = 0;
        CComPtr<IMFPresentationDescriptor> desc;
        HrIfFailledThrow(source_->CreatePresentationDescriptor(&desc));
        HrIfFailledThrow(desc->GetUINT64(MF_PD_DURATION, &duration));
        return duration;
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        HRESULT hr = S_OK;
        callback_->Start(topology_);
        const auto duration = GetSourceDuration();
        while (true) {
            hr = callback_->WaitFor();
            if (hr == E_PENDING) {
                MFTIME pos = 0;
                hr = callback_->GetEncodingPosition(pos);
                if (SUCCEEDED(hr)) {
                    const uint32_t percent = (100 * pos) / duration;
                    progress(percent);
                }                
            } else {
                break;
            }
        }
    }

    void SetContainer(const GUID& container_type) const {
        CComPtr<IMFAttributes> container_attrs;
        HrIfFailledThrow(::MFCreateAttributes(&container_attrs, 1));
        HrIfFailledThrow(container_attrs->SetGUID(
            MF_TRANSCODE_CONTAINERTYPE,
            container_type
        ));
        HrIfFailledThrow(profile_->SetContainerAttributes(container_attrs));
    }

    void SetAudioFormat(const GUID &target_format) {
        HrIfFailledThrow(::MFCreateAttributes(&attrs_, 8));
        HrIfFailledThrow(attrs_->SetGUID(MF_MT_SUBTYPE, target_format));
        if (!encoding_profile_.has_value()) {
            const auto format = AudioFormat::k16BitPCM441Khz;
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, format.GetChannels()));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, format.GetBitsPerSample()));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, format.GetSampleRate()));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AVG_BITRATE, 160000));
        } else {
            const auto& encoding_profile = encoding_profile_.value();
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, encoding_profile.num_channels));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, encoding_profile.bit_per_sample));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, encoding_profile.sample_rate));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, encoding_profile.bytes_per_second));
            HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AVG_BITRATE, encoding_profile.bitrate));
        }
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, (UINT32)AACProfileLevel::AAC_PROFILE_L2));
        HrIfFailledThrow(profile_->SetAudioAttributes(attrs_));
    }

    void SetEncodingProfile(const EncodingProfile& profile) {
        encoding_profile_ = profile;
    }
private:
    static HRESULT CreateMediaSource(const std::wstring& file_path, IMFMediaSource** media_source) {
        MF_OBJECT_TYPE object_type = MF_OBJECT_INVALID;

        CComPtr<IMFSourceResolver> source_resolver;
        CComPtr<IUnknown> unknown_source;

        auto hr = ::MFCreateSourceResolver(&source_resolver);

        if (SUCCEEDED(hr)) {
            hr = source_resolver->CreateObjectFromURL(
                file_path.c_str(),          // URL of the source.
                MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
                nullptr,                    // Optional property store.
                &object_type,                // Receives the created object type. 
                &unknown_source             // Receives a pointer to the media source.
            );
        }

        if (SUCCEEDED(hr)) {
            hr = unknown_source->QueryInterface(IID_PPV_ARGS(media_source));
        }
        return hr;
    }

    std::optional<EncodingProfile> encoding_profile_;
    CComPtr<IMFAttributes> attrs_;
    CComPtr<IMFMediaSource> source_;
    CComPtr<IMFTopology> topology_;
    CComPtr<IMFTranscodeProfile> profile_;
    CComPtr<MFEncoderCallback> callback_;
};

class MFAACFileEncoder::MFAACFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
	    const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        const auto encoding_profile = config.Get<EncodingProfile>(FileEncoderConfig::kEncodingProfile);

        SetEncodingProfile(encoding_profile);

        encoder_.Open(input_file_path);
        encoder_.SetAudioFormat(MFAudioFormat_AAC);
        encoder_.SetContainer(MFTranscodeContainerType_MPEG4);
        encoder_.SetOutputFile(output_file_path);
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        encoder_.Encode(progress);
    }

    void SetEncodingProfile(const EncodingProfile& profile) {
        encoder_.SetEncodingProfile(profile);
    }

    static Vector<EncodingProfile> GetAvailableEncodingProfile() {
        constexpr auto get_encoding_profile = []() {
            static auto logger = LoggerManager::GetInstance().GetLogger(kMFEncoderLoggerName);
            Vector<EncodingProfile> profiles;

            CComPtr<IMFCollection> available_types;
            auto hr = MFTranscodeGetAudioOutputAvailableTypes(MFAudioFormat_AAC,
                MFT_ENUM_FLAG_ALL,
                nullptr,
                &available_types);
            if (FAILED(hr)) {
                return profiles;
            }

            DWORD count = 0;
            hr = available_types->GetElementCount(&count);
            Vector<CComPtr<IUnknown>> unknown_audio_types;
            for (DWORD n = 0; n < count; n++) {
                CComPtr<IUnknown> unknown_audio_type;
                hr = available_types->GetElement(n, &unknown_audio_type);
                if (SUCCEEDED(hr)) {
                    unknown_audio_types.push_back(unknown_audio_type);
                }
            }

            Vector<CComPtr<IMFMediaType>> audio_types;
            for (const auto& unknown_audio_type : unknown_audio_types) {
                CComPtr<IMFMediaType> audio_type;
                hr = unknown_audio_type->QueryInterface(IID_PPV_ARGS(&audio_type));
                if (SUCCEEDED(hr)) {
                    audio_types.push_back(audio_type);
                }
            }

            for (const auto& audio_type : audio_types) {
                uint32_t sample_rate = 0;
                audio_type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sample_rate);
                uint32_t num_channels = 0;
                audio_type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &num_channels);
                if (num_channels > AudioFormat::kMaxChannel) {
                    continue;
                }
                uint32_t bytes_per_second = 0;
                audio_type->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytes_per_second);
                uint32_t bit_per_sample = 0;
                audio_type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bit_per_sample);
                auto payload_type = AACPayloadType::PAYLOAD_AAC_RAW;
                audio_type->GetUINT32(MF_MT_AAC_PAYLOAD_TYPE, reinterpret_cast<uint32_t*>(&payload_type));
                if (payload_type != AACPayloadType::PAYLOAD_AAC_RAW) {
                    continue;
                }
                uint32_t profile_level = AACProfileLevel::AAC_PROFILE_L2;
                audio_type->GetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &profile_level);
                if (profile_level != AACProfileLevel::AAC_PROFILE_L2) {
                    continue;
                }
                XAMP_LOG_D(logger, "{}bit, {}ch, {:.2f}kHz, {}kbps, {}kbps {} {}",
                    bit_per_sample,
                    num_channels,
                    sample_rate / 1000.,
                    (8 * bytes_per_second) / 1000,
                    bytes_per_second / 100,
                    payload_type,
                    profile_level);
                EncodingProfile profile;
                profile.bitrate = (8 * bytes_per_second) / 1000;
                profile.num_channels = num_channels;
                profile.bit_per_sample = bit_per_sample;
                profile.sample_rate = sample_rate;
                profile.bytes_per_second = bytes_per_second;
                profiles.push_back(profile);
            }
            return profiles;
        };

        static const auto profiles = get_encoding_profile();
        return profiles;
    }
private:
    MFEncoder encoder_;
};

MFAACFileEncoder::MFAACFileEncoder()
	: impl_(MakeAlign<MFAACFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(MFAACFileEncoder)

void MFAACFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void MFAACFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

Vector<EncodingProfile> MFAACFileEncoder::GetAvailableEncodingProfile() {
    return MFAACFileEncoderImpl::GetAvailableEncodingProfile();
}

void MFAACFileEncoder::SetEncodingProfile(const EncodingProfile& profile) {
    impl_->SetEncodingProfile(profile);
}

XAMP_STREAM_NAMESPACE_END

#endif
