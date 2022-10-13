#include <atlbase.h>

#include <initguid.h>
#include <cguid.h> // GUID_NULL

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

#include <base/platfrom_handle.h>
#include <base/audioformat.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/bass_utiltis.h>
#include <stream/mfaacencoder.h>

namespace xamp::stream {

#define HrIfFailledThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			throw PlatformSpecException(hresult); \
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
        MFTIME pos;
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
                    const LONGLONG percent = (100 * pos) / duration;
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
        const auto format = AudioFormat::k16BitPCM441Khz;
        HrIfFailledThrow(attrs_->SetGUID(MF_MT_SUBTYPE, target_format));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, format.GetChannels()));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, format.GetBitsPerSample()));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, format.GetSampleRate()));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AVG_BITRATE, 160000));
        HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, AACProfileLevel::AAC_PROFILE_L2));
        //HrIfFailledThrow(attrs_->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, AACPayloadType::PAYLOAD_AAC_ADTS));
        HrIfFailledThrow(profile_->SetAudioAttributes(attrs_));
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

    CComPtr<IMFAttributes> attrs_;
    CComPtr<IMFMediaSource> source_;
    CComPtr<IMFTopology> topology_;
    CComPtr<IMFTranscodeProfile> profile_;
    CComPtr<MFEncoderCallback> callback_;
};

class MFAACFileEncoder::MFAACFileEncoderImpl {
public:
    void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) {
        encoder_.Open(input_file_path);
        encoder_.SetAudioFormat(MFAudioFormat_AAC);
        encoder_.SetContainer(MFTranscodeContainerType_MPEG4);
        encoder_.SetOutputFile(output_file_path);
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        encoder_.Encode(progress);
    }
private:
    MFEncoder encoder_;
};

MFAACFileEncoder::MFAACFileEncoder()
	: impl_(MakeAlign<MFAACFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(MFAACFileEncoder)

void MFAACFileEncoder::Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) {
    impl_->Start(input_file_path, output_file_path, command);
}

void MFAACFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
