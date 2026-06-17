#include <FLAC/stream_decoder.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <base/dll.h>
#include <base/fastiostream.h>
#include <base/shared_singleton.h>
#include <base/unique_handle.h>

#include <stream/mqaidentifier.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
    class FlacLib final {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        FlacLib()
            : module_(OpenSharedLibrary("FLAC"))
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_new)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_delete)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_finish)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_init_file)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_process_until_end_of_metadata)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_process_single)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_get_state)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_set_md5_checking)
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_set_metadata_respond) {
        }

        XAMP_DISABLE_COPY(FlacLib)

    private:
        SharedLibraryHandle module_;

    public:
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_new);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_delete);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_finish);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_init_file);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_process_until_end_of_metadata);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_process_single);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_get_state);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_set_md5_checking);
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_set_metadata_respond);
    };

#define LibFlacDLL SharedSingleton<FlacLib>::getInstance()

    struct FlacDecoderHandleTraits final {
        static FLAC__StreamDecoder* invalid() {
            return nullptr;
        }

        static void close(FLAC__StreamDecoder* value) {
            LibFlacDLL.FLAC__stream_decoder_finish(value);
            LibFlacDLL.FLAC__stream_decoder_delete(value);
        }
    };

    using FlacDecoderHandle = UniqueHandle<FLAC__StreamDecoder*, FlacDecoderHandleTraits>;
}

// See: https://github.com/purpl3F0x/MQA_identifier/tree/master
class MqaIdentifier::MqaIdentifierImpl {
public:
    class MqaFile {
    public:
        friend class MqaIdentifierImpl;

        explicit MqaFile(const Path& path)
            : path_(path) {
        }

        void decode();

        static FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder*,
            const FLAC__Frame* frame,
            const FLAC__int32* const buffer[],
            void* client_data);

        static void MetadataCallback(const FLAC__StreamDecoder*,
            const FLAC__StreamMetadata* metadata,
            void* client_data);

        static void ErrorCallback(const FLAC__StreamDecoder*,
            FLAC__StreamDecoderErrorStatus,
            void* client_data);

    private:
        const Path path_;
        std::string mqa_encoder_;
        std::vector<std::array<const FLAC__int32, 2>> samples_;
        FLAC__uint64 decoded_samples_{ 0 };
        uint32_t sample_rate_{ 0 };
        uint32_t channels_{ 0 };
        uint32_t bps_{ 0 };
        uint32_t original_sample_rate_{ 0 };
    };

    explicit MqaIdentifierImpl(const Path& path);

    bool detect();

    bool isMQA() const {
        return is_mqa_;
    }

    bool isMQAStudio() const {
        return is_mqa_studio_;
    }

    uint32_t getOriginalSampleRate() const {
        return file_.original_sample_rate_;
    }

private:
    uint32_t OriginalSampleRateDecoder(unsigned c) {
        /*
         * If LSB is 0 then base is 44100 else 48000
         * 3 MSB need to be rotated and raised to the power of 2 (so 1, 2, 4, 8, ...)
         * output is base * multiplier
         */
        const uint32_t base = (c & 1u) ? 48000 : 44100;

        uint32_t multiplier = 1u << (((c >> 3u) & 1u) | (((c >> 2u) & 1u) << 1u) | (((c >> 1u) & 1u) << 2u));
        // Double for DSD
        if (multiplier > 16) {
            multiplier *= 2;
        }

        return base * multiplier;
    }

    MqaFile file_;
    bool is_mqa_{ false };
    bool is_mqa_studio_{ false };
};

MqaIdentifier::MqaIdentifierImpl::MqaIdentifierImpl(const Path& path)
    : file_(path) {
}

bool MqaIdentifier::MqaIdentifierImpl::detect() {
    file_.decode();

    static constexpr uint64_t kMQASignature = 0xbe0498c88;

    uint64_t buffer = 0;
    uint64_t buffer1 = 0;
    uint64_t buffer2 = 0;
    const auto pos = (file_.bps_ - 16u);

    for (const auto& s : file_.samples_) {
        buffer |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> pos) & 1u;
        buffer1 |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> (pos + 1)) & 1u;
        buffer2 |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> (pos + 2)) & 1u;

        if (buffer == kMQASignature) {
            is_mqa_ = true;
            // Get Original Sample Rate
            uint8_t orsf = 0;
            for (auto m = 3u; m < 7; m++) { // TODO: this need fix (orsf is 5bits)
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos) & 1u;
                orsf |= j << (6u - m);
            }
            file_.original_sample_rate_ = OriginalSampleRateDecoder(orsf);

            // Get MQA Studio
            uint8_t provenance = 0u;
            for (auto m = 29u; m < 34; m++) {
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos) & 1u;
                provenance |= j << (33u - m);
            }
            is_mqa_studio_ = provenance > 8;

            return true;
        }
        else if (buffer1 == kMQASignature) {
            is_mqa_ = true;
            // Get Original Sample Rate
            uint8_t orsf = 0;
            for (auto m = 3u; m < 7; m++) { // TODO: this need fix (orsf is 5bits)
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> (pos + 1)) & 1u;
                orsf |= j << (6u - m);
            }
            file_.original_sample_rate_ = OriginalSampleRateDecoder(orsf);

            // Get MQA Studio
            uint8_t provenance = 0u;
            for (auto m = 29u; m < 34; m++) {
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> (pos + 1)) & 1u;
                provenance |= j << (33u - m);
            }
            is_mqa_studio_ = provenance > 8;

            return true;
        }
        else if (buffer2 == kMQASignature) {
            is_mqa_ = true;
            // Get Original Sample Rate
            uint8_t orsf = 0;
            for (auto m = 3u; m < 7; m++) { // TODO: this need fix (orsf is 5bits)
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> (pos + 2)) & 1u;
                orsf |= j << (6u - m);
            }
            file_.original_sample_rate_ = OriginalSampleRateDecoder(orsf);

            // Get MQA Studio
            uint8_t provenance = 0u;
            for (auto m = 29u; m < 34; m++) {
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> (pos + 2)) & 1u;
                provenance |= j << (33u - m);
            }
            is_mqa_studio_ = provenance > 8;

            return true;
        }
        else {
            buffer = (buffer << 1u) & 0xFFFFFFFFFu;
            buffer1 = (buffer1 << 1u) & 0xFFFFFFFFFu;
            buffer2 = (buffer2 << 1u) & 0xFFFFFFFFFu;
        }
    }
    return false;
}

FLAC__StreamDecoderWriteStatus MqaIdentifier::MqaIdentifierImpl::MqaFile::WriteCallback(
    const FLAC__StreamDecoder*,
    const FLAC__Frame* frame,
    const FLAC__int32* const buffer[],
    void* client_data) {
    auto* self = static_cast<MqaFile*>(client_data);
    if (self->channels_ != 2 || (self->bps_ != 16 && self->bps_ != 24)) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    self->decoded_samples_ += frame->header.blocksize;

    for (size_t i = 0; i < frame->header.blocksize; i++) {
        self->samples_.push_back(std::array<const FLAC__int32, 2 >{buffer[0][i], buffer[1][i]});
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::ErrorCallback(const FLAC__StreamDecoder*,
    FLAC__StreamDecoderErrorStatus,
    void*) {
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::MetadataCallback(const FLAC__StreamDecoder*,
    const FLAC__StreamMetadata* metadata,
    void* client_data) {
    auto* self = static_cast<MqaFile*>(client_data);
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        self->sample_rate_ = metadata->data.stream_info.sample_rate;
        self->channels_ = metadata->data.stream_info.channels;
        self->bps_ = metadata->data.stream_info.bits_per_sample;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        for (FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
            const auto comment = reinterpret_cast<char*>(metadata->data.vorbis_comment.comments[i].entry);

            if (std::strncmp("MQAENCODER", comment, 10) == 0) {
                self->mqa_encoder_ = std::string(comment + 10, comment + metadata->data.vorbis_comment.comments[i].length);
            }
        }
    }
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::decode() {
    FlacDecoderHandle decoder(LibFlacDLL.FLAC__stream_decoder_new());
    if (!decoder) {
        throw std::runtime_error("FLAC__stream_decoder_new failed.");
    }

    sample_rate_ = 0;
    channels_ = 0;
    bps_ = 0;
    original_sample_rate_ = 0;
    decoded_samples_ = 0;
    samples_.clear();
    mqa_encoder_.clear();

    LibFlacDLL.FLAC__stream_decoder_set_md5_checking(decoder.get(), true);
    LibFlacDLL.FLAC__stream_decoder_set_metadata_respond(decoder.get(), FLAC__METADATA_TYPE_VORBIS_COMMENT);

    auto file_path = String::toUtf8String(path_.wstring());

    const auto init_status = LibFlacDLL.FLAC__stream_decoder_init_file(
        decoder.get(),
        file_path.c_str(),
        &WriteCallback,
        &MetadataCallback,
        &ErrorCallback,
        this);
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        throw std::runtime_error("FLAC init_file failed.");
    }

    if (!LibFlacDLL.FLAC__stream_decoder_process_until_end_of_metadata(decoder.get())) {
        throw std::runtime_error("FLAC metadata parse failed.");
    }

    samples_.reserve(sample_rate_ * 3);

    while (decoded_samples_ < sample_rate_ * 3) {
        if (!LibFlacDLL.FLAC__stream_decoder_process_single(decoder.get())) {
            throw std::runtime_error("FLAC__stream_decoder_process_single failed.");
        }

        const auto state = LibFlacDLL.FLAC__stream_decoder_get_state(decoder.get());
        if (state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED) {
            break;
        }
    }
}

XAMP_PIMPL_IMPL(MqaIdentifier)

MqaIdentifier::MqaIdentifier(const Path& path)
    : impl_(makeAlign<MqaIdentifierImpl>(path)) {
}

bool MqaIdentifier::detect() {
    return impl_->detect();
}

bool MqaIdentifier::isMQA() const {
    return impl_->isMQA();
}

bool MqaIdentifier::isMQAStudio() const {
    return impl_->isMQAStudio();
}

uint32_t MqaIdentifier::getOriginalSampleRate() const {
    return impl_->getOriginalSampleRate();
}

XAMP_STREAM_NAMESPACE_END
