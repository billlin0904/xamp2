#include <atlbase.h>

#include <initguid.h>
#include <cguid.h> // GUID_NULL

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

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

class MFEncoder {
public:
    MFEncoder() = default;

    ~MFEncoder() {
	    if (source_ != nullptr) {
            source_->Shutdown();
	    }
        if (session_ != nullptr) {
            session_->Shutdown();
        }
    }

    void Open(const std::wstring &file_path) {
        HrIfFailledThrow(CreateMediaSource(file_path, &source_));
        HrIfFailledThrow(::MFCreateMediaSession(nullptr, &session_));
        HrIfFailledThrow(::MFCreateTranscodeProfile(&profile_));
    }

    void SetOutputFile(const std::wstring& file_path) {
        HrIfFailledThrow(::MFCreateTranscodeTopology(source_, file_path.c_str(), profile_, &topology_));
        HrIfFailledThrow(session_->SetTopology(0, topology_));
        CComPtr<IMFClock> clock;
        HrIfFailledThrow(session_->GetClock(&clock));
        HrIfFailledThrow(clock->QueryInterface(IID_PPV_ARGS(&clock_)));
    }

    HRESULT Start() {
        PROPVARIANT var_start;
        PropVariantInit(&var_start);
        const auto hr = session_->Start(&GUID_NULL, &var_start);
        return hr;
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        MediaEventType me_type = MEUnknown;
        HRESULT status = S_OK;
        MFTIME pos;
        LONGLONG prev = 0;
        UINT64 duration = 0;

        CComPtr<IMFPresentationDescriptor> desc;
        HrIfFailledThrow(source_->CreatePresentationDescriptor(&desc));
        HrIfFailledThrow(desc->GetUINT64(MF_PD_DURATION, &duration));

        while (me_type != MESessionClosed) {
            CComPtr<IMFMediaEvent> evt;
            auto hr = session_->GetEvent(0, &evt);
            if (FAILED(hr)) { break; }

            hr = evt->GetType(&me_type);
            if (FAILED(hr)) { break; }

            hr = evt->GetStatus(&status);
            if (FAILED(hr)) { break; }

            switch (me_type) {
            case MESessionTopologySet:
                hr = Start();
                break;
            case MESessionStarted:
                hr = clock_->GetTime(&pos);
                if (SUCCEEDED(hr)) {
	                const uint32_t percent = (100 * pos) / duration;
                    progress(percent);
				}
                break;
            case MESessionEnded:
                hr = session_->Close();
                break;
            case MESessionClosed:
                break;
            }

            if (FAILED(hr)) { break; }
        }
    }

    void SetContainer(const GUID& container_type) const {
        CComPtr<IMFAttributes> container_attrs;
        HrIfFailledThrow(::MFCreateAttributes(&container_attrs, 1));
        HrIfFailledThrow(container_attrs->SetGUID(
            MF_TRANSCODE_CONTAINERTYPE,
            container_type
        ));
        HrIfFailledThrow(container_attrs->SetUINT32(
            MF_TRANSCODE_ADJUST_PROFILE,
            MF_TRANSCODE_ADJUST_PROFILE_DEFAULT
        ));
        HrIfFailledThrow(profile_->SetContainerAttributes(container_attrs));
    }

    void SetAudioFormat(const GUID &target_format) {
        CComPtr<IMFCollection> available_types;
        CComPtr<IUnknown> unknow_audio_type;
        CComPtr<IMFMediaType> audio_type;
        CComPtr<IMFAttributes> audio_attrs;
        DWORD mt_count = 0;

        HrIfFailledThrow(::MFTranscodeGetAudioOutputAvailableTypes(
            target_format,
            MFT_ENUM_FLAG_ALL,
            nullptr,
            &available_types
        ));

        auto hr = available_types->GetElementCount(&mt_count);
        if (mt_count == 0) {
            hr = E_UNEXPECTED;
        }
        HrIfFailledThrow(hr);
        HrIfFailledThrow(available_types->GetElement(0, &unknow_audio_type));
        HrIfFailledThrow(unknow_audio_type->QueryInterface(IID_PPV_ARGS(&audio_type)));
        HrIfFailledThrow(::MFCreateAttributes(&audio_attrs, 0));
        HrIfFailledThrow(audio_type->CopyAllItems(audio_attrs));
        HrIfFailledThrow(audio_attrs->SetGUID(MF_MT_SUBTYPE, target_format));
        HrIfFailledThrow(profile_->SetAudioAttributes(audio_attrs));
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

    CComPtr<IMFMediaSession> session_;
    CComPtr<IMFMediaSource> source_;
    CComPtr<IMFTopology> topology_;
    CComPtr<IMFTranscodeProfile> profile_;
    CComPtr<IMFPresentationClock> clock_;
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
