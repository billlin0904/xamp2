#include <FLAC/stream_decoder.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include <base/dll.h>
#include <base/fastiostream.h>
#include <base/logger.h>
#include <base/shared_singleton.h>
#include <base/unique_handle.h>
#include <stream/api.h>
#include <stream/mqafilestream.h>

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
            , XAMP_LOAD_DLL_API(FLAC__stream_decoder_seek_absolute)
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
        XAMP_DECLARE_DLL_NAME(FLAC__stream_decoder_seek_absolute);
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

class MqaFileStream::MqaFileStreamImpl {
public:
    MqaFileStreamImpl() {
        logger_ = XAMP_LOG_CREATE_LOGGER(MqaFileStreamImpl);
    }

    void open(const Path& path) {
        decoder_.reset(LibFlacDLL.FLAC__stream_decoder_new());
        if (!decoder_) throw std::runtime_error("FLAC__stream_decoder_new failed.");

        auto file_path = String::toUtf8String(path.wstring());

        const auto st = LibFlacDLL.FLAC__stream_decoder_init_file(
            decoder_.get(),
            file_path.c_str(),
            &WriteCallback,
            &MetadataCallback,
            &ErrorCallback,
            this
        );

        if (st != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            throw std::runtime_error("FLAC init_file failed.");
        }

        if (!LibFlacDLL.FLAC__stream_decoder_process_until_end_of_metadata(decoder_.get())) {
            throw std::runtime_error("FLAC metadata parse failed.");
        }

        active_ = true;
    }

    void close() {
        decoder_.reset();

        active_ = false;
        pcm_queue_.clear();
        queue_read_bytes_ = 0;
    }

    double getDuration() const {
        if (sample_rate_ == 0) return 0.0;
        if (total_samples_ == 0) return 0.0;
        return static_cast<double>(total_samples_) / static_cast<double>(sample_rate_);
    }

    uint32_t getSamples(void* buffer, uint32_t length) const {
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

    AudioFormat getFormat() const {
        return AudioFormat(DataFormat::FORMAT_PCM, channels_, bits_per_sample_, sample_rate_);
    }

    void seek(double stream_time) const {
        if (!active_ || sample_rate_ == 0) return;

        const double t = std::max(0.0, stream_time);
        const uint64_t target = static_cast<uint64_t>(t * static_cast<double>(sample_rate_));

        pcm_queue_.clear();
        queue_read_bytes_ = 0;

        LibFlacDLL.FLAC__stream_decoder_seek_absolute(decoder_.get(), target);
    }

    uint32_t getSampleSize() const {
        return sizeof(int32_t);
    }

    bool isActive() const {
        return active_; 
    }

    uint32_t getBitDepth() const { 
        return bits_per_sample_; 
    }

    uint32_t getBitRate() const {
        return 0;
    }

	bool endOfStream() const {
		if (!active_) return true;
		const auto state = LibFlacDLL.FLAC__stream_decoder_get_state(decoder_.get());
		return state == FLAC__STREAM_DECODER_END_OF_STREAM || state == FLAC__STREAM_DECODER_ABORTED;
	}

private:
    bool DecodeOneBlock() const {
        const auto state = LibFlacDLL.FLAC__stream_decoder_get_state(decoder_.get());
        if (state == FLAC__STREAM_DECODER_END_OF_STREAM) return false;
        if (state == FLAC__STREAM_DECODER_ABORTED) return false;

        if (!LibFlacDLL.FLAC__stream_decoder_process_single(decoder_.get())) return false;

        const auto state2 = LibFlacDLL.FLAC__stream_decoder_get_state(decoder_.get());
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
    mutable FlacDecoderHandle decoder_;
    mutable std::vector<int32_t> pcm_queue_;
    mutable uint64_t queue_read_bytes_ = 0;
	LoggerPtr logger_;
};

MqaFileStream::MqaFileStream()
	: impl_(makeAlign<MqaFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(MqaFileStream)

void MqaFileStream::openFile(Path const& file_path) {
    impl_->open(file_path);
}

void MqaFileStream::open(ArchiveEntry archive_entry) {
}

bool MqaFileStream::endOfStream() const {
    return impl_->endOfStream();
}

void MqaFileStream::close() {
    impl_->close();
}

double MqaFileStream::getDuration() const {
    return impl_->getDuration();
}

AudioFormat MqaFileStream::getFormat() const {
    return impl_->getFormat();
}

void MqaFileStream::seek(double stream_time) const {
    impl_->seek(stream_time);
}

uint32_t MqaFileStream::getSamples(void* buffer, uint32_t length) const {
    return impl_->getSamples(buffer, length);
}

uint32_t MqaFileStream::getSampleSize() const {
    return impl_->getSampleSize();
}

uint32_t MqaFileStream::getBitDepth() const {
    return impl_->getBitDepth();
}

uint32_t MqaFileStream::getBitRate() const {
    return impl_->getBitRate();
}

bool MqaFileStream::isActive() const {
    return impl_->isActive();
}

void LoadMqaLib() {
    SharedSingleton<FlacLib>::getInstance();
}

XAMP_STREAM_NAMESPACE_END
