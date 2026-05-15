#include <FLAC/stream_decoder.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include <base/fastiostream.h>
#include <base/logger.h>
#include <stream/mqafilestream.h>
#include <FLAC++/decoder.h>

XAMP_STREAM_NAMESPACE_BEGIN


// See: https://github.com/purpl3F0x/MQA_identifier/tree/master
class MqaIdentifier::MqaIdentifierImpl {
public:
    class MqaFile : public FLAC::Decoder::File {
    public:
        friend class MqaIdentifierImpl;

        explicit MqaFile(const Path& path)
            : path_(path) {
        }

        void decode();
        virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame* frame,
            const FLAC__int32* const buffer[]) override;
        void metadata_callback(const ::FLAC__StreamMetadata* metadata) override;
        void error_callback(::FLAC__StreamDecoderErrorStatus status) override;

    private:
        uint32_t sample_rate_ = 0;
        uint32_t channels_ = 0;
        uint32_t bps_ = 0;
        uint32_t original_sample_rate_ = 0;   
        FLAC__uint64 decoded_samples_ = 0;
        const Path path_;
        std::string mqa_encoder_;
        std::vector<std::array<const FLAC__int32, 2>> samples_;
    };

    explicit MqaIdentifierImpl(const Path& path);

    bool Detect();
    
    bool IsMQA() const {
        return is_mqa_;
    }

    bool IsMQAStudio() const {
        return is_mqa_studio_;
	}

    uint32_t GetOriginalSampleRate() const {
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
        if (multiplier > 16) multiplier *= 2;

        return base * multiplier;
    }

    bool is_mqa_{ false };
    bool is_mqa_studio_{ false };
    MqaFile file_;
};

MqaIdentifier::MqaIdentifierImpl::MqaIdentifierImpl(const Path& path)
    : file_(path) {
}

bool MqaIdentifier::MqaIdentifierImpl::Detect() {
    file_.decode();

	static constexpr uint64_t kMQASignature = 0xbe0498c88;

    uint64_t buffer = 0;
    uint64_t buffer1 = 0;
    uint64_t buffer2 = 0;
    const auto pos = (file_.bps_ - 16u);

    for (const auto& s : file_.samples_) {
        buffer |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> pos) & 1u;
        buffer1 |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> pos + 1) & 1u;
        buffer2 |= ((static_cast<uint32_t>(s[0]) ^ static_cast<uint32_t>(s[1])) >> pos + 2) & 1u;

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

            // We are done return true
            return true;
		}
		else if (buffer1 == kMQASignature) {
            is_mqa_ = true;
            // Get Original Sample Rate
            uint8_t orsf = 0;
            for (auto m = 3u; m < 7; m++) { // TODO: this need fix (orsf is 5bits)
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos + 1) & 1u;
                orsf |= j << (6u - m);
            }
            file_.original_sample_rate_ = OriginalSampleRateDecoder(orsf);

            // Get MQA Studio
            uint8_t provenance = 0u;
            for (auto m = 29u; m < 34; m++) {
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos + 1) & 1u;
                provenance |= j << (33u - m);
            }
            is_mqa_studio_ = provenance > 8;

            // We are done return true
            return true;
        }
        else if (buffer2 == kMQASignature) {
            is_mqa_ = true;
            // Get Original Sample Rate
            uint8_t orsf = 0;
            for (auto m = 3u; m < 7; m++) { // TODO: this need fix (orsf is 5bits)
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos + 2) & 1u;
                orsf |= j << (6u - m);
            }
            file_.original_sample_rate_ = OriginalSampleRateDecoder(orsf);

            // Get MQA Studio
            uint8_t provenance = 0u;
            for (auto m = 29u; m < 34; m++) {
                auto cur = *(&s + m);
                auto j = ((static_cast<uint32_t>(cur[0]) ^ static_cast<uint32_t>(cur[1])) >> pos + 2) & 1u;
                provenance |= j << (33u - m);
            }
            is_mqa_studio_ = provenance > 8;

            // We are done return true
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

::FLAC__StreamDecoderWriteStatus MqaIdentifier::MqaIdentifierImpl::MqaFile::write_callback(
    const ::FLAC__Frame* frame,
    const FLAC__int32* const buffer[]) {
    if (channels_ != 2 || (bps_ != 16 && bps_ != 24)) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    decoded_samples_ += frame->header.blocksize;

    for (size_t i = 0; i < frame->header.blocksize; i++)
        samples_.push_back(std::array<const FLAC__int32, 2 >{buffer[0][i], buffer[1][i]});

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::error_callback(::FLAC__StreamDecoderErrorStatus status) {
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::metadata_callback(const ::FLAC__StreamMetadata* metadata) {
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        sample_rate_ = metadata->data.stream_info.sample_rate;
        channels_ = metadata->data.stream_info.channels;
        bps_ = metadata->data.stream_info.bits_per_sample;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        for (FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
            const auto comment = reinterpret_cast<char*>(metadata->data.vorbis_comment.comments[i].entry);

            if (std::strncmp("MQAENCODER", comment, 10) == 0)
                mqa_encoder_ = std::string(comment + 10, comment + metadata->data.vorbis_comment.comments[i].length);
        }
    }
}

void MqaIdentifier::MqaIdentifierImpl::MqaFile::decode() {
    set_md5_checking(true);
    set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT);

    auto file_path = String::ToUtf8String(path_.wstring());

    FLAC__StreamDecoderInitStatus init_status = FLAC::Decoder::File::init(file_path.c_str());
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        throw std::runtime_error("FLAC__stream_decoder_new failed.");
    }

    process_until_end_of_metadata();
    
    samples_.reserve(sample_rate_ * 3);

    while (decoded_samples_ < sample_rate_ * 3) {
        if (!process_single()) {
			throw std::runtime_error("FLAC__stream_decoder_process_single failed.");
        }
    }
}

XAMP_PIMPL_IMPL(MqaIdentifier)

MqaIdentifier::MqaIdentifier(const Path& path)
    : impl_(MakeAlign<MqaIdentifierImpl>(path)) {
}

bool MqaIdentifier::Detect() {
	return impl_->Detect();
}

bool MqaIdentifier::IsMQA() const {
    return impl_->IsMQA();
}

bool MqaIdentifier::IsMQAStudio() const {
    return impl_->IsMQAStudio();
}

uint32_t MqaIdentifier::GetOriginalSampleRate() const {
    return impl_->GetOriginalSampleRate();
}

class MqaFileStream::MqaFileStreamImpl {
public:
    MqaFileStreamImpl() {
        logger_ = XAMP_LOG_CREATE_LOGGER(MqaFileStreamImpl);
    }

    void Open(const Path& path) {
        decoder_ = FLAC__stream_decoder_new();
        if (!decoder_) throw std::runtime_error("FLAC__stream_decoder_new failed.");

        auto file_path = String::ToUtf8String(path.wstring());

        const auto st = FLAC__stream_decoder_init_file(
            decoder_,
            file_path.c_str(),
            &WriteCallback,
            &MetadataCallback,
            &ErrorCallback,
            this
        );

        if (st != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            throw std::runtime_error("FLAC init_file failed.");
        }

        if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder_)) {
            throw std::runtime_error("FLAC metadata parse failed.");
        }

        active_ = true;
    }

    void Close() {
        if (decoder_) {
            FLAC__stream_decoder_finish(decoder_);
            FLAC__stream_decoder_delete(decoder_);
            decoder_ = nullptr;
        }

        active_ = false;
        pcm_queue_.clear();
        queue_read_bytes_ = 0;
    }

    double GetDuration() const {
        if (sample_rate_ == 0) return 0.0;
        if (total_samples_ == 0) return 0.0;
        return static_cast<double>(total_samples_) / static_cast<double>(sample_rate_);
    }

    uint32_t GetSamples(void* buffer, uint32_t length) const {
        if (!buffer || length == 0 || !active_) return 0;

		length *= sizeof(int32_t);

        auto* out_bytes = static_cast<uint8_t*>(buffer);
        uint32_t written = 0;

        // 我們的 queue 裡存的是 int32 interleaved
        const uint32_t bytes_per_sample = sizeof(int32_t);
        const uint32_t bytes_per_frame = bytes_per_sample * channels_;

        // length 必須是 frame 對齊比較好（你也可以允許非對齊，這裡做向下取整）
        length -= (length % bytes_per_frame);

        while (written < length) {
            // 1) 如果 queue 有資料，先拷貝出去
            const uint32_t available = QueueBytesAvailable();
            if (available > 0) {
                const uint32_t to_copy = std::min(available, length - written);
                CopyFromQueue(out_bytes + written, to_copy);
                written += to_copy;
                continue;
            }

            // 2) queue 沒資料：推進 decoder 解一個 frame
            if (!DecodeOneBlock()) {
                // EOF 或錯誤
                break;
            }
        }

        return written / sizeof(int32_t);
    }

    AudioFormat GetFormat() const {
        return AudioFormat(DataFormat::FORMAT_PCM, channels_, bits_per_sample_, sample_rate_);
    }

    void Seek(double stream_time) const {
        if (!active_ || sample_rate_ == 0) return;

        const double t = std::max(0.0, stream_time);
        const uint64_t target = static_cast<uint64_t>(t * static_cast<double>(sample_rate_));

        pcm_queue_.clear();
        queue_read_bytes_ = 0;

        FLAC__stream_decoder_seek_absolute(decoder_, target);
    }

    uint32_t GetSampleSize() const {
        return sizeof(int32_t);
    }

    bool IsActive() const {
        return active_; 
    }

    uint32_t GetBitDepth() const { 
        return bits_per_sample_; 
    }

    uint32_t GetBitRate() const {
        return 0;
    }

private:
    bool DecodeOneBlock() const {
        const auto state = FLAC__stream_decoder_get_state(decoder_);
        if (state == FLAC__STREAM_DECODER_END_OF_STREAM) return false;
        if (state == FLAC__STREAM_DECODER_ABORTED) return false;

        if (!FLAC__stream_decoder_process_single(decoder_)) return false;

        const auto state2 = FLAC__stream_decoder_get_state(decoder_);
        if (state2 == FLAC__STREAM_DECODER_END_OF_STREAM) return false;
        if (state2 == FLAC__STREAM_DECODER_ABORTED) return false;

        return true;
    }

    uint32_t QueueBytesAvailable() const {
        const uint64_t total = static_cast<uint64_t>(pcm_queue_.size()) * sizeof(int32_t);
        if (queue_read_bytes_ >= total) return 0;
        return static_cast<uint32_t>(total - queue_read_bytes_);
    }

    void CopyFromQueue(uint8_t* dst, uint32_t bytes) const {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(pcm_queue_.data());
        std::memcpy(dst, src + queue_read_bytes_, bytes);
        queue_read_bytes_ += bytes;

        const uint64_t total = static_cast<uint64_t>(pcm_queue_.size()) * sizeof(int32_t);
        if (queue_read_bytes_ >= total) {
            pcm_queue_.clear();
            queue_read_bytes_ = 0;
        }
    }

    static FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder*,
        const FLAC__Frame* frame,
        const FLAC__int32* const buffer[],
        void* client_data) {
        auto* self = static_cast<MqaFileStreamImpl*>(client_data);
        const uint32_t block_size = frame->header.blocksize;
        const uint32_t ch = self->channels_;
        const uint32_t bps = self->bits_per_sample_;

        // 轉成「24-in-32（高 24 位有效）」一致格式
        const int shift = (bps >= 32) ? 0 : (32 - static_cast<int>(bps));

        const size_t old = self->pcm_queue_.size();
        self->pcm_queue_.resize(old + static_cast<size_t>(block_size) * ch);
        int32_t* out = self->pcm_queue_.data() + old;

        for (uint32_t i = 0; i < block_size; ++i) {
            for (uint32_t c = 0; c < ch; ++c) {
                int32_t s = buffer[c][i];
                if (shift > 0) s <<= shift;
                *out++ = s;
            }
        }

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void MetadataCallback(const FLAC__StreamDecoder*,
        const FLAC__StreamMetadata* metadata,
        void* client_data) {
        auto* self = static_cast<MqaFileStreamImpl*>(client_data);
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            self->sample_rate_ = metadata->data.stream_info.sample_rate;
            self->channels_ = metadata->data.stream_info.channels;
            self->bits_per_sample_ = metadata->data.stream_info.bits_per_sample;
            self->total_samples_ = metadata->data.stream_info.total_samples;
        }
    }

    static void ErrorCallback(const FLAC__StreamDecoder*,
        FLAC__StreamDecoderErrorStatus,
        void* client_data) {
        auto* self = static_cast<MqaFileStreamImpl*>(client_data);
        self->active_ = false;
    }

    mutable bool active_ = false;
    uint32_t sample_rate_ = 0;
    uint32_t channels_ = 0;
    uint32_t bits_per_sample_ = 0;
    uint64_t total_samples_ = 0;
    FLAC__StreamDecoder* decoder_ = nullptr;
    mutable std::vector<int32_t> pcm_queue_;
    mutable uint64_t queue_read_bytes_ = 0;
	LoggerPtr logger_;
};

MqaFileStream::MqaFileStream()
	: impl_(MakeAlign<MqaFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(MqaFileStream)

void MqaFileStream::OpenFile(Path const& file_path) {
    impl_->Open(file_path);
}

void MqaFileStream::Open(ArchiveEntry archive_entry) {
}

void MqaFileStream::SetRate(float rate) {
}

void MqaFileStream::Close() {
    impl_->Close();
}

double MqaFileStream::GetDuration() const {
    return impl_->GetDuration();
}

AudioFormat MqaFileStream::GetFormat() const {
    return impl_->GetFormat();
}

void MqaFileStream::Seek(double stream_time) const {
    impl_->Seek(stream_time);
}

uint32_t MqaFileStream::GetSamples(void* buffer, uint32_t length) const {
    return impl_->GetSamples(buffer, length);
}

uint32_t MqaFileStream::GetSampleSize() const {
    return impl_->GetSampleSize();
}

uint32_t MqaFileStream::GetBitDepth() const {
    return impl_->GetBitDepth();
}

uint32_t MqaFileStream::GetBitRate() const {
    return impl_->GetBitRate();
}

bool MqaFileStream::IsActive() const {
    return impl_->IsActive();
}

XAMP_STREAM_NAMESPACE_END
