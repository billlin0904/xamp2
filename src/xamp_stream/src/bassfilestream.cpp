#include <stream/bassfilestream.h>

#include <stream/bassexception.h>
#include <stream/basslib.h>

#include <fstream>
#include <base/dsd_utils.h>
#include <base/str_utilts.h>
#include <base/stopwatch.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <bass/bassdsd.h>
#include <base/fastiostream.h>

#include <stream/compressorconfig.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassFileStream);

struct FastIOStreamContext {
    static DWORD CALLBACK BassReadProc(void* buffer, DWORD length, void* user) {
        auto* stream = static_cast<FastIOStream*>(user);
        return static_cast<DWORD>(stream->read(buffer, length));
    }

    static QWORD CALLBACK BassLenProc(void* user) {
        auto* stream = static_cast<FastIOStream*>(user);
        return stream->size();
    }

    static BOOL CALLBACK BassSeekProc(QWORD offset, void* user) {
        auto* stream = static_cast<FastIOStream*>(user);

        if (offset == static_cast<QWORD>(-1))
            return TRUE;

        stream->seek(static_cast<int64_t>(offset), SEEK_SET);
        return TRUE;
    }

    static void CALLBACK BassCloseProc(void* user) {
        auto* stream = static_cast<FastIOStream*>(user);    
        stream->close();
    }
};

class ArchiveContext {
public:
    static constexpr size_t kReadSize = 512 * 1024;
    
private:    
    uint64_t write_pos_ = 0;
    uint64_t read_pos_ = 0;
    uint64_t total_len_ = 0;
    std::vector<char> buffer_;    
    TemporaryFile file_;
    ArchiveEntry entry;

    static bool WriteCache(ArchiveContext* ctx, uint64_t want_end) {
        while (ctx->write_pos_ < want_end) {
            auto result = ctx->entry.Read(ctx->buffer_.data(), static_cast<long>(kReadSize));
            if (!result) {
                return false;
            }
            auto r = result.value();
            if (r <= 0) {
                return false;
            }
            ctx->file_.Seek(0, SEEK_END);
            ctx->file_.Write(ctx->buffer_.data(), 1, r);
            ctx->write_pos_ += r;
        }
        return true;
    }
public:
    explicit ArchiveContext(ArchiveEntry archive_entry) {
        entry = std::move(archive_entry);
        buffer_.resize(kReadSize);
    }

    ~ArchiveContext() = default;    

    static QWORD CALLBACK ArchiveLengthCallback(void* user) {
        auto* context = static_cast<ArchiveContext*>(user);
        return context->entry.Length();
    }

    static DWORD CALLBACK ArchiveReadCallback(void* buf, DWORD len, void* user) {
        auto* ctx = static_cast<ArchiveContext*>(user);
        const uint64_t want_end = ctx->read_pos_ + len;

        WriteCache(ctx, want_end);

        const uint64_t avail = ctx->write_pos_ - ctx->read_pos_;
        const DWORD to_read = static_cast<DWORD>(std::min<uint64_t>(len, avail));
        if (to_read == 0) {
            return 0;
        }

        if (!ctx->file_.Seek(ctx->read_pos_, SEEK_SET)) {
            return 0;
        }

        size_t n = ctx->file_.Read(buf, 1, to_read);
        ctx->read_pos_ += n;

        return static_cast<DWORD>(n);
    }

    static BOOL CALLBACK ArchiveSeekCallback(QWORD offset, void* user) {
        auto* ctx = static_cast<ArchiveContext*>(user);

        if (offset > ctx->total_len_ && ctx->total_len_ != 0) {
            return FALSE;
        }

        if (!WriteCache(ctx, offset)) {
            return FALSE;
        }

        ctx->read_pos_ = static_cast<uint64_t>(offset);
        return TRUE;
    }

    static void CALLBACK ArchiveCloseCallback(void* user) {
        auto* ctx = static_cast<ArchiveContext*>(user);
        ctx->file_.Close();
    }    
};

class BassFileStream::BassFileStreamImpl {
public:
    BassFileStreamImpl() : mode_(DsdModes::DSD_MODE_PCM)
		, download_size_(0) {
        logger_ = XAMP_LOG_CREATE_LOGGER(BassFileStream);
        Close();
    }

    ~BassFileStreamImpl() {
        Close();
    }

    void CreateBassStream(const std::wstring & file_path, DsdModes mode, DWORD flags) {
        static constexpr BASS_FILEPROCS file_process = {
               &FastIOStreamContext::BassCloseProc,
               &FastIOStreamContext::BassLenProc,
               &FastIOStreamContext::BassReadProc,
               &FastIOStreamContext::BassSeekProc
        };

        if (mode == DsdModes::DSD_MODE_PCM) {
            io_stream_.open(file_path, FastIOStream::Mode::Read);
            impl_.reset(BassLibDLL.BASS_StreamCreateFileUser(
                STREAMFILE_NOBUFFER,
                flags | BASS_STREAM_DECODE,
                &file_process,
                &io_stream_
            ));
        }
        else {
            io_stream_.open(file_path, FastIOStream::Mode::Read);
            impl_.reset(BassLibDLL.DSDLib->BASS_DSD_StreamCreateFileUser(
                STREAMFILE_NOBUFFER,
                flags | BASS_STREAM_DECODE,
                &file_process,
                &io_stream_,
                0
            ));
            // BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            BassLibDLL.BASS_ChannelSetAttribute(impl_.get(), BASS_ATTRIB_DSD_GAIN, 0.0f);
        }
    }

    void CreateFileOrURL(std::wstring const& file_path, bool is_file_path, DsdModes mode, DWORD flags) {
        if (is_file_path) {
            PrefetchFile(file_path);

	        const auto is_cda_file = IsCDAFile(file_path);

            if (is_cda_file) {
                flags |= BASS_ASYNCFILE;
                // Only for windows.
                impl_.reset(BassLibDLL.BASS_StreamCreateFile(FALSE,
                    file_path.c_str(),
                    0,
                    0,
                    flags | BASS_UNICODE | BASS_STREAM_DECODE));
                return;
            }

            CreateBassStream(file_path, mode, flags);
        } else {
#ifdef XAMP_OS_MAC
            auto utf8 = String::ToString(file_path);
            stream_.reset(BASS_LIB.BASS_StreamCreateURL(
                utf8.c_str(),
                0,
                flags | BASS_STREAM_DECODE | BASS_STREAM_STATUS,
                &BassFileStreamImpl::DownloadProc,
                this));
#else
            auto url = const_cast<wchar_t*>(file_path.c_str());
            impl_.reset(BassLibDLL.BASS_StreamCreateURL(
                url,
                0,
                flags | BASS_STREAM_DECODE | BASS_UNICODE | BASS_STREAM_STATUS,
                &BassFileStreamImpl::DownloadProc,
                this));
#endif
        }

        if (!impl_) {
            throw BassException();
        }
    }

    void Open(ArchiveEntry archive_entry) {
        static constexpr BASS_FILEPROCS file_process = {
            &ArchiveContext::ArchiveCloseCallback,
            & ArchiveContext::ArchiveLengthCallback,
            & ArchiveContext::ArchiveReadCallback,
            & ArchiveContext::ArchiveSeekCallback
        };

        DWORD flags = 0;

        switch (mode_) {
        case DsdModes::DSD_MODE_PCM:
        case DsdModes::DSD_MODE_DSD2PCM:
            flags = BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_DOP:
            // DSD-over-PCM data is 24-bit, so the BASS_SAMPLE_FLOAT flag is required.
            flags = BASS_DSD_DOP | BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_NATIVE:
            flags = BASS_DSD_RAW;
            break;
        default:
            XAMP_NO_DEFAULT;
        }

        archive_context_ = MakeAlign<ArchiveContext>(std::move(archive_entry));

        Stopwatch measure_stream_time;

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            impl_.reset(BassLibDLL.BASS_StreamCreateFileUser(
                STREAMFILE_BUFFER,
                flags | BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                &file_process,
                archive_context_.get()
            ));
        }
        else {
            impl_.reset(BassLibDLL.DSDLib->BASS_DSD_StreamCreateFileUser(
                STREAMFILE_NOBUFFER,
                flags | BASS_STREAM_DECODE,
                &file_process,
                archive_context_.get(),
                0
            ));
        }

        XAMP_LOG_DEBUG("Open track is a {} secs", measure_stream_time.ElapsedSeconds());
        LoadStream(0.0f);
    }

    void Open(Path const& file_path) {
        DWORD flags = 0;

        switch (mode_) {
        case DsdModes::DSD_MODE_PCM:
        case DsdModes::DSD_MODE_DSD2PCM:
            flags = BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_DOP:
            // DSD-over-PCM data is 24-bit, so the BASS_SAMPLE_FLOAT flag is required.
            flags = BASS_DSD_DOP | BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_NATIVE:
            flags = BASS_DSD_RAW;
            break;
        default:
            XAMP_NO_DEFAULT;
        }

        XAMP_LOG_D(logger_, "Use DsdModes: {}", mode_);

        const auto is_http = file_path.wstring().find(L"http") != std::string::npos
    		|| file_path.wstring().find(L"https") != std::string::npos;
        XAMP_LOG_D(logger_, "Start open file");

        CreateFileOrURL(file_path.wstring(), !is_http, mode_, flags);        
        LoadStream(0.0f);
    }

    void CheckZeroDuration() {
        const auto source_duration = GetSourceDurationSeconds();
        const auto duration = GetDuration();
        XAMP_LOG_DEBUG("Source duration: {:.2f} secs, Processed duration: {:.2f} secs",
            source_duration,
			duration);
        if (duration < 1.0) {
            throw LibraryException(
                String::Format("Duration too small {:.2f} secs",
                    duration));
        }
    }

    void LoadStream(float rate) {
        info_ = BASS_CHANNELINFO{};        
        BassIfFailedThrow(BassLibDLL.BASS_ChannelGetInfo(impl_.get(), &info_)); 

        if (mode_ == DsdModes::DSD_MODE_DOP || mode_ == DsdModes::DSD_MODE_NATIVE) {
            CheckZeroDuration();
            return;
        }

        if (GetFormat().GetChannels() == AudioFormat::kMaxChannel) {
            CreateTempoStream();
            SetRate(rate);
            CheckZeroDuration();
            return;
        }

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            mix_stream_.reset(
                BassLibDLL.MixLib->BASS_Mixer_StreamCreate(
                    GetFormat().GetSampleRate(),
                    AudioFormat::kMaxChannel,
                    BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_END));
            if (!mix_stream_) {
                throw BassException();
            }

            BassIfFailedThrow(
                BassLibDLL.MixLib->BASS_Mixer_StreamAddChannel(mix_stream_.get(),
                    impl_.get(),
                    BASS_MIXER_BUFFER));
            XAMP_LOG_D(logger_,
                "Mix stream {} channel to 2 channel", info_.chans);
            info_.chans = AudioFormat::kMaxChannel;
        }
        else {
            throw NotSupportFormatException();
        }        
        CreateTempoStream();
        SetRate(rate);
        CheckZeroDuration();
    }

    [[nodiscard]] uint32_t GetBitDepth() const {
        if (mode_ == DsdModes::DSD_MODE_DOP) {
            return 8;
        }
        if (mode_ == DsdModes::DSD_MODE_DSD2PCM) {
            return 24;
        }
        if (info_.origres == 0) {
            return 16;
        }
        return info_.origres;
    }

    [[nodiscard]] double GetReadProgress() const {
        auto file_len = 
            BassLibDLL.BASS_StreamGetFilePosition(GetHStream(),
                BASS_FILEPOS_END);
        auto buffer =
            BassLibDLL.BASS_StreamGetFilePosition(GetHStream(), 
                BASS_FILEPOS_BUFFER);
        return 100.0 * static_cast<double>(buffer)
    	/ static_cast<double>(file_len);
    }

    [[nodiscard]] int32_t GetBufferingProgress() const {
        return 100 - BassLibDLL.BASS_StreamGetFilePosition(GetHStream(), 
            BASS_FILEPOS_BUFFERING);
    }

	static void DownloadProc(const void* buffer, DWORD length, void* user) {
        auto* impl = static_cast<BassFileStreamImpl*>(user);
    	if (!buffer) {
            XAMP_LOG_D(impl->logger_, "Downloading 100% completed!");
            return;
    	}
        if (length == 0) {
            auto *ptr = static_cast<char const*>(buffer);
            std::string http_status(ptr);
            XAMP_LOG_D(impl->logger_, "{}", http_status);
    	} else {
            if (impl->download_size_ == 0) {
                impl->download_size_ += length;
                XAMP_LOG_D(impl->logger_,
                    "Downloading {:.2f}% {}",
                    impl->GetReadProgress(),
                    String::FormatBytes(impl->download_size_));
            }            
        }
    }

    void Close() {   
        tempo_stream_.reset();
        impl_.reset();
        mix_stream_.reset();        
        io_stream_.close();
        mode_ = DsdModes::DSD_MODE_PCM;
        info_ = BASS_CHANNELINFO{};
        download_size_ = 0;
        playback_rate_ = 1.0f;
    }

    [[nodiscard]] bool IsDsdFile() const {
        return info_.ctype == BASS_CTYPE_STREAM_DSD;
    }

    uint32_t GetSamples(void *buffer, uint32_t length) const {
        return InternalGetSamples(buffer,
            length * GetSampleSize()) / GetSampleSize();
    }
    
    static double GetHStreamDuration(HSTREAM stream) {
        const auto len =
            BassLibDLL.BASS_ChannelGetLength(stream, BASS_POS_BYTE);
        return BassLibDLL.BASS_ChannelBytes2Seconds(stream, len);
    }

    [[nodiscard]] double GetSourceDurationSeconds() const {
        return GetHStreamDuration(impl_.get());
    }

    [[nodiscard]] double GetDuration() const {
        const double src = GetSourceDurationSeconds();
        if (!tempo_stream_.is_valid()) return src;
        return src / playback_rate_;
    }

    [[nodiscard]] AudioFormat GetFormat() const {
        if (mode_ == DsdModes::DSD_MODE_NATIVE) {
            return AudioFormat(DataFormat::FORMAT_DSD,
                static_cast<uint16_t>(info_.chans),
                ByteFormat::SINT8,
                GetDsdSampleRate());
        } else if (mode_ == DsdModes::DSD_MODE_DOP) {
            return AudioFormat(DataFormat::FORMAT_PCM,
                static_cast<uint16_t>(info_.chans),
                ByteFormat::FLOAT32,
                GetDOPSampleRate(GetDsdSpeed()));
        }
        return AudioFormat(DataFormat::FORMAT_PCM,
				static_cast<uint16_t>(info_.chans),
				ByteFormat::FLOAT32,
				info_.freq);
    }

    DWORD GetSetPositionFlags() const {
        if (tempo_stream_.is_valid()) {
            return BASS_POS_DECODETO;
        }
        return 0;
	}

    void Seek(double stream_time) const {
        /*double playback_seconds = stream_time / playback_rate_;

        const auto pos_bytes =
            BassLibDLL.BASS_ChannelSeconds2Bytes(GetHStream(), playback_seconds);
        BassIfFailedThrow(BassLibDLL.BASS_ChannelSetPosition(GetHStream(),
            pos_bytes, BASS_POS_BYTE | GetSetPositionFlags()));*/

        auto h = GetHStream();
        const double playback_seconds = stream_time / playback_rate_;
        QWORD target_bytes = BassLibDLL.BASS_ChannelSeconds2Bytes(h, playback_seconds);
        const QWORD len = BassLibDLL.BASS_ChannelGetLength(h, BASS_POS_BYTE);        

        const QWORD cur_bytes = BassLibDLL.BASS_ChannelGetPosition(h, BASS_POS_BYTE);

        if (BassLibDLL.BASS_ChannelSetPosition(h, target_bytes, BASS_POS_BYTE)) {
            return;
        }

        if (len && target_bytes >= len)
            target_bytes = (len > 0) ? (len - 1) : 0;

        const int err = BassLibDLL.BASS_ErrorGetCode();
        if (tempo_stream_.is_valid() && target_bytes >= cur_bytes) {
            BassIfFailedThrow(
                BassLibDLL.BASS_ChannelSetPosition(h, target_bytes, BASS_POS_BYTE | BASS_POS_DECODETO)
            );
            return;
        }

        BassIfFailedThrow(false);
    }

    double GetPosition() const {
        double playback_seconds = BassLibDLL.BASS_ChannelBytes2Seconds(GetHStream(),
            BassLibDLL.BASS_ChannelGetPosition(GetHStream(), BASS_POS_BYTE));
        return playback_seconds * playback_rate_;
    }

    [[nodiscard]] uint32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(BassLibDLL.BASS_ChannelGetAttribute(GetSourceStream(),
            BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<uint32_t>(rate);
    }

    [[nodiscard]] uint32_t GetBitRate() const {
        float rate = 0;
        BassIfFailedThrow(BassLibDLL.BASS_ChannelGetAttribute(GetSourceStream(),
            BASS_ATTRIB_BITRATE, &rate));
        return static_cast<uint32_t>(rate);
    }

    [[nodiscard]] bool SupportDOP() const {
        return true;
    }

    [[nodiscard]] bool SupportDOP_AA() const {
        return false;
    }

    [[nodiscard]] bool SupportNativeSD() const {
        return true;
    }

    void SetDSDMode(DsdModes mode) {
        mode_ = mode;
    }

    [[nodiscard]] DsdModes GetDSDMode() const {
        return mode_;       
    }

    [[nodiscard]] uint32_t GetSampleSize() const {
        return mode_ == DsdModes::DSD_MODE_NATIVE
    	? sizeof(int8_t) : sizeof(float);
    }

    [[nodiscard]] DsdFormat GetDsdFormat() const {
        return DsdFormat::DSD_INT8MSB;
    }

    void SetDsdToPcmSampleRate(uint32_t sample_rate) {
        BassLibDLL.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, sample_rate);
    }

    [[nodiscard]] uint32_t GetDsdSpeed() const {
        return GetDsdSampleRate() / kPcmSampleRate441;
    }

    [[nodiscard]] HSTREAM GetHStream() const {      
        if (tempo_stream_.is_valid()) {
            return tempo_stream_.get();
        }
        if (mix_stream_.is_valid()) {
            return mix_stream_.get();
        }
        return impl_.get();
    }

    [[nodiscard]] bool IsActive() const {
        return BassLibDLL.BASS_ChannelIsActive(GetSourceStream()) == BASS_ACTIVE_PLAYING;
    }
	
    bool EndOfStream() const {
        auto last_error = BassLibDLL.BASS_ErrorGetCode();
        if (last_error == BASS_ERROR_ENDED) {
            return true;
        }
        return false;
    }    

    void SetRate(float percent) {
        percent = std::clamp(percent, 0.0f, 95.0f);
        playback_rate_ = 1.0f - percent / 100.0f;
        if (!tempo_enabled_) {
            return;
        }
        if (!CanUseTempo()) {
            throw NotSupportFormatException();
        }
        if (!tempo_stream_.is_valid()) {
            return;
        }
        const float tempo_percent = -percent;
        BassIfFailedThrow(BassLibDLL.BASS_ChannelSetAttribute(
            tempo_stream_.get(),
            BASS_ATTRIB_TEMPO, 
            tempo_percent));
    }

private:
    uint32_t InternalGetSamples(void* buffer, uint32_t length) const {
        const auto bytes_read =
            BassLibDLL.BASS_ChannelGetData(GetHStream(), buffer, length);
        if (bytes_read == kBassError) {            			
            return 0;
        }
        return static_cast<uint32_t>(bytes_read);
    }

    bool CanUseTempo() const {
        return mode_ != DsdModes::DSD_MODE_NATIVE;
    }

    HSTREAM GetSourceStream() const {
        return impl_.get();
    }

    void CreateTempoStream() {
        tempo_stream_.reset();

        if (!tempo_enabled_ || !CanUseTempo()) {
            return;
        }

        const HSTREAM base = (mix_stream_.is_valid() ? mix_stream_.get() : impl_.get());
        if (!base) {
            return;
        }

        tempo_stream_.reset(BassLibDLL.FxLib->BASS_FX_TempoCreate(base, BASS_STREAM_DECODE));
        BassIfFailedThrow(tempo_stream_.get());

        const float tempo_percent = (playback_rate_ - 1.0f) * 100.0f;
        BassIfFailedThrow(BassLibDLL.BASS_ChannelSetAttribute(
            tempo_stream_.get(), BASS_ATTRIB_TEMPO, tempo_percent));
    }

    DsdModes mode_;
    bool tempo_enabled_ = true;
    float playback_rate_ = 1.0f;    
    size_t download_size_;
    BassStreamHandle impl_;
    BassStreamHandle mix_stream_;
    BassStreamHandle tempo_stream_;
    BASS_CHANNELINFO info_;
    ScopedPtr<ArchiveContext> archive_context_;
    FastIOStream io_stream_;
    LoggerPtr logger_;
};

BassFileStream::BassFileStream()
    : impl_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::OpenFile(Path const& file_path)  {
    impl_->Open(file_path);
}

void BassFileStream::Open(ArchiveEntry archive_entry) {
    impl_->Open(std::move(archive_entry));
}

void BassFileStream::SetRate(float rate) {
}

void BassFileStream::Close() {
    impl_->Close();
}

bool BassFileStream::EndOfStream() const {
    return impl_->EndOfStream();
}

double BassFileStream::GetDuration() const {
    return impl_->GetDuration();
}

AudioFormat BassFileStream::GetFormat() const {
    return impl_->GetFormat();
}

void BassFileStream::Seek(double stream_time) const {
    impl_->Seek(stream_time);
}

uint32_t BassFileStream::GetSamples(void *buffer, uint32_t length) const {
    return impl_->GetSamples(buffer, length);
}

void BassFileStream::SetDSDMode(DsdModes mode) {
    impl_->SetDSDMode(mode);
}

DsdModes BassFileStream::GetDsdMode() const {
    return impl_->GetDSDMode();
}

bool BassFileStream::IsDsdFile() const {
    return impl_->IsDsdFile();
}

uint32_t BassFileStream::GetDsdSampleRate() const {
    return impl_->GetDsdSampleRate();
}

uint32_t BassFileStream::GetSampleSize() const {
    return impl_->GetSampleSize();
}

DsdFormat BassFileStream::GetDsdFormat() const {
    return impl_->GetDsdFormat();
}

void BassFileStream::SetDsdToPcmSampleRate(uint32_t sample_rate) {
    impl_->SetDsdToPcmSampleRate(sample_rate);
}

uint32_t BassFileStream::GetDsdSpeed() const {
    return impl_->GetDsdSpeed();
}

uint32_t BassFileStream::GetBitDepth() const {
    return impl_->GetBitDepth();
}

uint32_t BassFileStream::GetBitRate() const {
    return impl_->GetBitRate();
}

uint32_t BassFileStream::GetHStream() const {
    return impl_->GetHStream();
}

bool BassFileStream::IsActive() const {
    return impl_->IsActive();
}

bool BassFileStream::SupportDOP() const {
    return impl_->SupportDOP();
}

bool BassFileStream::SupportDOP_AA() const {
    return impl_->SupportDOP_AA();
}

bool BassFileStream::SupportNativeSD() const {
    return impl_->SupportNativeSD();
}

XAMP_STREAM_NAMESPACE_END
