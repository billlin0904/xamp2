//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <stream/pcm2dsdfilestream.h>
#include <stream/pcmdsdlib.h>

#include <base/bitperfect.h>
#include <base/dataconverter.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

XAMP_STREAM_NAMESPACE_BEGIN

class Pcm2DsdFileStream::Pcm2DsdFileStreamImpl {
public:
    Pcm2DsdFileStreamImpl(ScopedPtr<FileStream> input,
        uint32_t multiplier, double gain_db, bool dither,
        uint32_t filter, 
        uint32_t modulator,
        uint32_t block_frames)
        : source_(std::move(input))
        , multiplier_(multiplier)
        , gain_db_(gain_db)
        , dither_enabled_(dither)
        , options_{sizeof(pcm_dsd_options), filter, modulator, block_frames} {
        if (!source_) {
            throw std::invalid_argument("PCM2DSD requires an open PCM source");
        }
        initialize();
    }

    ~Pcm2DsdFileStreamImpl() {
        if (handle_) {
            LIB_PCM_DSD_LIB.pcm_dsd_destroy(handle_);
        }
    }

    void destroyHandle() {
        if (!handle_) {
            return;
        }
        LIB_PCM_DSD_LIB.pcm_dsd_destroy(handle_);
    }

    void initialize() {
        if (handle_) {
            LIB_PCM_DSD_LIB.pcm_dsd_destroy(handle_);
            handle_ = nullptr;
        }
        const auto input = source_->integerPcmFormat();
        if (!input || !pcm::valid(*input) ||
            !bitperfect::supported(input->valid_bits, input->channels, input->sample_rate) ||
            input->container_bits != 32 || input->layout != pcm::Layout::Interleaved) {
            throw std::runtime_error("PCM2DSD requires BitPerfect stereo 16/24/32-bit integer PCM");
        }
        if (!std::isfinite(gain_db_) || gain_db_ < -24 || gain_db_ > 12) {
            throw std::runtime_error("PCM2DSD gain must be between -24 and +12 dB");
        }
        input_format_ = *input;
        pcm_dsd_config config;
        LIB_PCM_DSD_LIB.pcm_dsd_default_config(&config);
        config.input_sample_rate = input->sample_rate;
        config.channels = input->channels;
        config.dsd_multiplier = multiplier_;
        config.input_gain = 0.5 * std::pow(10.0, gain_db_ / 20.0);
        config.dither_amplitude = dither_enabled_ ? std::ldexp(1.0, -24) : 0.0;
        const int status = LIB_PCM_DSD_LIB.pcm_dsd_create_ex(&config, &options_, &handle_);
        if (status != PCM_DSD_OK) {
            throw std::runtime_error("PCM2DSD cannot create converter: unsupported rate/ratio or insufficient memory");
        }
        output_rate_ = LIB_PCM_DSD_LIB.pcm_dsd_output_sample_rate(handle_);
        clear();
    }

    void clear() {
        pcm_pos_ = pcm_frames_ = raw_pos_ = raw_size_ = 0;
        source_end_ = finished_ = false;
        marker_ = 0x05;
    }

    bool refill() {
        raw_pos_ = raw_size_ = 0;
        while (!finished_) {
            size_t used = 0;
            size_t written = 0;
            int status;
            if (pcm_pos_ == pcm_frames_ && !source_end_) {
                const auto block = source_->readPcm(pcm_storage_);
                pcm_pos_ = 0;
                pcm_frames_ = block.frames;
                if (block.format != input_format_) {
                    throw std::runtime_error("PCM2DSD source format changed");
                }
                source_end_ = block.frames == 0;
                convertPcmToDouble(block.data.data(), pcm_.data(),
                    block.frames * input_format_.channels, input_format_);
            }

            if (source_end_ && pcm_pos_ == pcm_frames_) {
                status = LIB_PCM_DSD_LIB.pcm_dsd_flush(handle_, raw_.data(), raw_.size(), &written);
                finished_ = status == PCM_DSD_FINISHED;
            } else {
                status = LIB_PCM_DSD_LIB.pcm_dsd_process(handle_, pcm_.data() + pcm_pos_ * 2, pcm_frames_ - pcm_pos_,
                                                  raw_.data(), raw_.size(), &used, &written);
                pcm_pos_ += used;
            }

            if (status != PCM_DSD_OK && status != PCM_DSD_FINISHED) {
                throw std::runtime_error("PCM2DSD conversion failed");
            }
            raw_size_ = written;
            if (raw_size_) {
                return true;
            }
            if (!finished_ && !source_end_ && !used) {
                throw std::runtime_error("PCM2DSD converter made no progress");
            }
        }

        return false;
    }

    size_t read(uint8_t* out, size_t capacity) {
        if (mode_ == DsdModes::DSD_MODE_NATIVE) {
            size_t count = 0;
            while (count < capacity) {
                if (raw_pos_ == raw_size_ && !refill()) {
                    break;
                }
                const auto n = (std::min)(capacity - count, raw_size_ - raw_pos_);
                std::memcpy(out + count, raw_.data() + raw_pos_, n);
                raw_pos_ += n;
                count += n;
            }

            return count;
        }

        size_t count = 0;
        while (count < capacity) {
            std::array<uint8_t, 4> group{};
            size_t n = 0;
            while (n < 4) {
                if (raw_pos_ == raw_size_ && !refill()) {
                    break;
                }
                group[n++] = raw_[raw_pos_++];
            }

            if (!n) {
                break;
            }
            // Final odd byte pair: complete a DoP frame with a DSD idle pattern.
            if (n == 2) {
                group[2] = group[3] = 0x69;
            } else if (n != 4) {
                throw std::runtime_error("PCM2DSD returned an incomplete stereo byte pair");
            }
            // DoP v1.1: oldest DSD bit is bit15; marker bits23..16.
            // Canonical signed 24-bit, left-aligned in a 32-bit LE container.
            for (size_t ch = 0; ch < 2; ++ch) {
                out[count * 4] = 0;
                out[count * 4 + 1] = group[2 + ch];
                out[count * 4 + 2] = group[ch];
                out[count * 4 + 3] = marker_;
                ++count;
            }

            marker_ = marker_ == 0x05 ? 0xfa : 0x05;
        }

        return count;
    }

    void seek(double seconds) {
        if (!handle_ || !std::isfinite(seconds)) {
            throw std::invalid_argument("Invalid PCM2DSD seek");
        }
        seconds = std::clamp(seconds, 0.0, source_->getDuration());
        if (seconds >= source_->getDuration()) {
            source_->seek(source_->getDuration());
            if (LIB_PCM_DSD_LIB.pcm_dsd_reset(handle_) != PCM_DSD_OK) {
                throw std::runtime_error("PCM2DSD reset failed");
            }
            clear();
            source_end_ = finished_ = true;
            return;
        }

        const auto target = static_cast<uint64_t>(seconds * input_format_.sample_rate);
        const auto up = output_rate_ / input_format_.sample_rate;
        const uint64_t bits_per_frame = mode_ == DsdModes::DSD_MODE_NATIVE ? 8 : 16;
        const uint64_t alignment = up < bits_per_frame ? bits_per_frame / up : 1;
        auto start = target > 4096 ? target - 4096 : 0;
        start -= start % alignment;
        source_->seek(static_cast<double>(start) / input_format_.sample_rate);
        if (LIB_PCM_DSD_LIB.pcm_dsd_reset(handle_) != PCM_DSD_OK) {
            throw std::runtime_error("PCM2DSD reset failed");
        }
        clear();
        uint64_t discard = (target - start) * up / bits_per_frame;
        std::array<uint8_t, 8192> temp;
        while (discard) {
            const auto wanted = static_cast<size_t>((std::min)(discard, uint64_t{1024})) * 2;
            const auto samples = read(temp.data(), wanted);
            if (!samples) {
                break;
            }
            discard -= samples / 2;
        }
    }

    ScopedPtr<FileStream> source_;
    DsdModes mode_{DsdModes::DSD_MODE_DOP};
    uint32_t multiplier_;
    uint32_t output_rate_{0};
    double gain_db_;
    bool dither_enabled_;
    pcm_dsd_options options_;
    pcm_dsd_converter* handle_{};
    pcm::Format input_format_;
    std::array<std::byte, 2048 * 2 * 4> pcm_storage_{};
    std::array<double, 2048 * 2> pcm_{};
    std::array<uint8_t, 8192> raw_{};
    size_t pcm_pos_{};
    size_t pcm_frames_{};
    size_t raw_pos_{};
    size_t raw_size_{};
    bool source_end_{};
    bool finished_{};
    uint8_t marker_{0x05};
};

Pcm2DsdFileStream::Pcm2DsdFileStream(ScopedPtr<FileStream> source,
    uint32_t multiplier,
    double gain_db,
    bool dither)
    : Pcm2DsdFileStream(std::move(source), multiplier, gain_db, dither, 0, 0, 256) {
}

Pcm2DsdFileStream::Pcm2DsdFileStream(ScopedPtr<FileStream> source,
    uint32_t multiplier,
    double gain_db,
    bool dither,
    uint32_t filter,
    uint32_t modulator,
    uint32_t block_frames)
    : impl_(makeAlign<Pcm2DsdFileStreamImpl>(std::move(source),
        multiplier, 
        gain_db, 
        dither, 
        filter, 
        modulator,
        block_frames)) {
}

Pcm2DsdFileStream::~Pcm2DsdFileStream() = default;

void Pcm2DsdFileStream::openFile(const Path& path) {
    impl_->source_->openFile(path);
    impl_->initialize();
}

void Pcm2DsdFileStream::open(ArchiveEntry entry) {
    impl_->source_->open(std::move(entry));
    impl_->initialize();
}

void Pcm2DsdFileStream::close() {
    if (impl_->handle_) {
        impl_->destroyHandle();
        impl_->handle_ = nullptr;
    }
    impl_->source_->close();
    impl_->clear();
    impl_->finished_ = true;
}

bool Pcm2DsdFileStream::endOfStream() const {
    return impl_->finished_ && impl_->raw_pos_ == impl_->raw_size_;
}

bool Pcm2DsdFileStream::isActive() const {
    return !endOfStream();
}
double Pcm2DsdFileStream::getDuration() const {
    return impl_->source_->getDuration();
}

uint32_t Pcm2DsdFileStream::getSamples(void* buffer, uint32_t length) const {
    if (!length) {
        return 0;
    }
    if (!buffer || length % 2) {
        throw std::invalid_argument("PCM2DSD reads require whole stereo frames");
    }
    if (!impl_->handle_) {
        throw std::runtime_error("PCM2DSD stream is closed");
    }
    return static_cast<uint32_t>(impl_->read(static_cast<uint8_t*>(buffer), length));
}

AudioFormat Pcm2DsdFileStream::getFormat() const {
    return impl_->mode_ == DsdModes::DSD_MODE_NATIVE
               ? AudioFormat(DataFormat::FORMAT_DSD, 2, ByteFormat::SINT8, impl_->output_rate_)
               : AudioFormat(DataFormat::FORMAT_PCM, 2, ByteFormat::SINT32, impl_->output_rate_ / 16);
}

uint32_t Pcm2DsdFileStream::getSampleSize() const {
    return impl_->mode_ == DsdModes::DSD_MODE_NATIVE ? 1 : 4;
}

uint32_t Pcm2DsdFileStream::getBitDepth() const {
    return impl_->mode_ == DsdModes::DSD_MODE_NATIVE ? 1 : 24;
}

DsdModes Pcm2DsdFileStream::getDsdMode() const {
    return impl_->mode_;
}

std::optional<pcm::Format> Pcm2DsdFileStream::integerPcmFormat() const {
    if (impl_->mode_ == DsdModes::DSD_MODE_NATIVE) {
        return std::nullopt;
    }
    return pcm::canonical(24, 2, impl_->output_rate_ / 16);
}

void Pcm2DsdFileStream::seek(double seconds) const {
    impl_->seek(seconds);
}

uint32_t Pcm2DsdFileStream::getBitRate() const {
    return impl_->output_rate_ * 2 / 1000;
}

uint32_t Pcm2DsdFileStream::getDsdSampleRate() const {
    return impl_->output_rate_;
}

uint32_t Pcm2DsdFileStream::getDsdSpeed() const {
    return impl_->multiplier_;
}

void Pcm2DsdFileStream::setDSDMode(DsdModes mode) {
    if (mode != DsdModes::DSD_MODE_DOP && mode != DsdModes::DSD_MODE_NATIVE) {
        throw std::invalid_argument("PCM2DSD requires DoP or Native DSD output");
    }
    if (impl_->mode_ != mode) {
        impl_->mode_ = mode;
        impl_->seek(0);
    }
}

void Pcm2DsdFileStream::setDsdToPcmSampleRate(uint32_t) {
    throw std::invalid_argument("PCM2DSD is not a DSD decoder");
}

XAMP_STREAM_NAMESPACE_END
