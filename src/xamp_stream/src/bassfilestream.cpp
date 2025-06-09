#include <stream/bassfilestream.h>

#include <stream/bassexception.h>
#include <stream/basslib.h>

#include <fstream>
#include <base/dsd_utils.h>
#include <base/str_utilts.h>
#include <base/singleton.h>
#include <base/stopwatch.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <bass/bassdsd.h>

#include <stream/compressorconfig.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassFileStream);

class ArchiveContext {
public:
    static constexpr size_t kReadSize = 512 * 1024;
    
private:
    ArchiveEntry entry;
    TemporaryFile file_;
    uint64_t write_pos_ = 0;
    uint64_t read_pos_ = 0;
    uint64_t total_len_ = 0;
    std::vector<char> buffer_;

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
    BassFileStreamImpl() noexcept
        : mode_(DsdModes::DSD_MODE_PCM)
		, download_size_(0) {
        logger_ = XampLoggerFactory.GetLogger(kBassFileStreamLoggerName);
        Close();
    }

    ~BassFileStreamImpl() {
        Close();
    }

    void CreateBassStream(const std::wstring & file_path, DsdModes mode, DWORD flags) {
        if (mode == DsdModes::DSD_MODE_PCM) {
#ifdef XAMP_OS_MAC
            auto utf8 = String::ToString(file_path);
            stream_.reset(BASS_LIB.BASS_StreamCreateFile(FALSE,
                utf8.c_str(),
                0,
                0,
                flags | BASS_STREAM_DECODE));
#else
            stream_.reset(BASS_LIB.BASS_StreamCreateFile(FALSE,
                file_path.c_str(),
                0,
                0,
                flags | BASS_STREAM_DECODE | BASS_UNICODE));
#endif
        }
        else {
#ifdef XAMP_OS_MAC
            auto utf8 = String::ToString(file_path);
            stream_.reset(BASS_LIB.DSDLib->BASS_DSD_StreamCreateFile(FALSE,
                utf8.c_str(),
                0,
                0,
                flags | BASS_STREAM_DECODE,
                0));
#else
            stream_.reset(BASS_LIB.DSDLib->BASS_DSD_StreamCreateFile(FALSE,
                file_path.c_str(),
                0,
                0,
                flags | BASS_STREAM_DECODE | BASS_UNICODE,
                0));            
#endif
            // BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            BASS_LIB.BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);
        }
    }

    void CreateMemoryMappedBassStream(std::wstring const& file_path, DsdModes mode, DWORD flags) {
        if (!file_.Open(file_path)) {
            throw PlatformException();
        }

        if (mode == DsdModes::DSD_MODE_PCM) {
            stream_.reset(BASS_LIB.BASS_StreamCreateFile(TRUE,
                file_.GetData(),
                0,
                file_.GetLength(),
                flags | BASS_STREAM_DECODE));
        }
        else {
            stream_.reset(BASS_LIB.DSDLib->BASS_DSD_StreamCreateFile(TRUE,
                file_.GetData(),
                0,
                file_.GetLength(),
                flags | BASS_STREAM_DECODE,
                0));
            // BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            BASS_LIB.BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);
        }
    }

    void CreateFileOrURL(std::wstring const& file_path, bool is_file_path, DsdModes mode, DWORD flags) {
        if (is_file_path) {
            PrefetchFile(file_path);

	        const auto is_cda_file = IsCDAFile(file_path);

            if (is_cda_file) {
                // Only for windows.
                stream_.reset(BASS_LIB.BASS_StreamCreateFile(FALSE,
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
            stream_.reset(BASS_LIB.BASS_StreamCreateURL(
                url,
                0,
                flags | BASS_STREAM_DECODE | BASS_UNICODE | BASS_STREAM_STATUS,
                &BassFileStreamImpl::DownloadProc,
                this));
#endif
        }

        if (!stream_) {
            throw BassException();
        }
    }

    void Open(ArchiveEntry archive_entry) {
        const BASS_FILEPROCS file_process = {
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

        flags |= BASS_ASYNCFILE;

        archive_context_ = MakeAlign<ArchiveContext>(std::move(archive_entry));

        Stopwatch measure_stream_time;

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            stream_.reset(BASS_LIB.BASS_StreamCreateFileUser(
                STREAMFILE_BUFFER,
                flags | BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                &file_process,
                archive_context_.get()
            ));
        }
        else {
            stream_.reset(BASS_LIB.DSDLib->BASS_DSD_StreamCreateFileUser(
                STREAMFILE_NOBUFFER,
                flags | BASS_STREAM_DECODE,
                &file_process,
                archive_context_.get(),
                0
            ));
        }

        XAMP_LOG_DEBUG("Open track is a {} Secs", measure_stream_time.ElapsedSeconds());

        LoadStream();
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

        flags |= BASS_ASYNCFILE;

        XAMP_LOG_D(logger_, "Use DsdModes: {}", mode_);

        const auto is_http = file_path.wstring().find(L"http") != std::string::npos
    		|| file_path.wstring().find(L"https") != std::string::npos;
        XAMP_LOG_D(logger_, "Start open file");

        CreateFileOrURL(file_path.wstring(), !is_http, mode_, flags);
        LoadStream();
    }

    void LoadStream() {
        info_ = BASS_CHANNELINFO{};
        BassIfFailedThrow(BASS_LIB.BASS_ChannelGetInfo(stream_.get(), &info_));

        const auto duration = GetDuration();
        if (duration < 1.0) {
            throw LibraryException(
                String::Format("Duration too small! {:.2f} secs",
                    duration));
        }

        if (GetFormat().GetChannels() == AudioFormat::kMaxChannel) {
            return;
        }

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            mix_stream_.reset(
                BASS_LIB.MixLib->BASS_Mixer_StreamCreate(
                    GetFormat().GetSampleRate(),
                    AudioFormat::kMaxChannel,
                    BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_END));
            if (!mix_stream_) {
                throw BassException();
            }

            BassIfFailedThrow(
                BASS_LIB.MixLib->BASS_Mixer_StreamAddChannel(mix_stream_.get(),
                    stream_.get(),
                    BASS_MIXER_BUFFER));
            XAMP_LOG_D(logger_,
                "Mix stream {} channel to 2 channel", info_.chans);
            info_.chans = AudioFormat::kMaxChannel;
        }
        else {
            throw NotSupportFormatException();
        }
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
            BASS_LIB.BASS_StreamGetFilePosition(GetHStream(),
                BASS_FILEPOS_END);
        auto buffer =
            BASS_LIB.BASS_StreamGetFilePosition(GetHStream(), 
                BASS_FILEPOS_BUFFER);
        return 100.0 * static_cast<double>(buffer)
    	/ static_cast<double>(file_len);
    }

    [[nodiscard]] int32_t GetBufferingProgress() const {
        return 100 - BASS_LIB.BASS_StreamGetFilePosition(GetHStream(), 
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

    void Close() noexcept {
        stream_.reset();
        mix_stream_.reset();
        file_.Close();
        mode_ = DsdModes::DSD_MODE_PCM;
        info_ = BASS_CHANNELINFO{};
        download_size_ = 0;
    }

    [[nodiscard]] bool IsDsdFile() const noexcept {
        return info_.ctype == BASS_CTYPE_STREAM_DSD;
    }

    uint32_t GetSamples(void *buffer, uint32_t length) const {
        return InternalGetSamples(buffer,
            length * GetSampleSize()) / GetSampleSize();
    }

    [[nodiscard]] double GetDuration() const {
        const auto len = 
            BASS_LIB.BASS_ChannelGetLength(GetHStream(), BASS_POS_BYTE);
        return BASS_LIB.BASS_ChannelBytes2Seconds(GetHStream(), len);
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

    void Seek(double stream_time) const {
        const auto pos_bytes =
            BASS_LIB.BASS_ChannelSeconds2Bytes(GetHStream(), stream_time);
        BassIfFailedThrow(BASS_LIB.BASS_ChannelSetPosition(GetHStream(),
            pos_bytes, BASS_POS_BYTE));
    }

    double GetPosition() const {
        return BASS_LIB.BASS_ChannelBytes2Seconds(GetHStream(),
            BASS_LIB.BASS_ChannelGetPosition(GetHStream(), BASS_POS_BYTE));
    }

    [[nodiscard]] uint32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(BASS_LIB.BASS_ChannelGetAttribute(GetHStream(), 
            BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<uint32_t>(rate);
    }

    [[nodiscard]] bool SupportDOP() const noexcept {
        return true;
    }

    [[nodiscard]] bool SupportDOP_AA() const noexcept {
        return false;
    }

    [[nodiscard]] bool SupportNativeSD() const noexcept {
        return true;
    }

    void SetDSDMode(DsdModes mode) noexcept {
        mode_ = mode;
    }

    [[nodiscard]] DsdModes GetDSDMode() const noexcept {
        return mode_;       
    }

    [[nodiscard]] uint32_t GetSampleSize() const noexcept {
        return mode_ == DsdModes::DSD_MODE_NATIVE
    	? sizeof(int8_t) : sizeof(float);
    }

    [[nodiscard]] DsdFormat GetDsdFormat() const noexcept {
        return DsdFormat::DSD_INT8MSB;
    }

    void SetDsdToPcmSampleRate(uint32_t sample_rate) {
        BASS_LIB.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, sample_rate);
    }

    [[nodiscard]] uint32_t GetDsdSpeed() const noexcept {
        return GetDsdSampleRate() / kPcmSampleRate441;
    }

    [[nodiscard]] HSTREAM GetHStream() const noexcept {
        if (mix_stream_.is_valid()) {
            return mix_stream_.get();
        }
        return stream_.get();
    }

    [[nodiscard]] bool IsActive() const noexcept {
        return BASS_LIB.BASS_ChannelIsActive(GetHStream()) == BASS_ACTIVE_PLAYING;
    }
	
    bool EndOfStream() const {
        auto last_error = BASS_LIB.BASS_ErrorGetCode();
        if (last_error == BASS_ERROR_ENDED) {
            return true;
        }
        return false;
    }    

private:
    uint32_t InternalGetSamples(void* buffer, uint32_t length) const noexcept {
        const auto bytes_read =
            BASS_LIB.BASS_ChannelGetData(GetHStream(), buffer, length);
        if (bytes_read == kBassError) {            			
            return 0;
        }
        return static_cast<uint32_t>(bytes_read);
    }

    DsdModes mode_;
    size_t download_size_;
    BassStreamHandle stream_;
    BassStreamHandle mix_stream_;
    BassStreamHandle limiter_;
    BASS_CHANNELINFO info_;
    MemoryMappedFile file_;
    ScopedPtr<ArchiveContext> archive_context_;
    LoggerPtr logger_;
};

BassFileStream::BassFileStream()
    : stream_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::OpenFile(Path const& file_path)  {
    stream_->Open(file_path);
}

void BassFileStream::Open(ArchiveEntry archive_entry) {
    stream_->Open(std::move(archive_entry));
}

void BassFileStream::Close() noexcept {
    stream_->Close();
}

bool BassFileStream::EndOfStream() const {
    return stream_->EndOfStream();
}

double BassFileStream::GetDurationAsSeconds() const {
    return stream_->GetDuration();
}

AudioFormat BassFileStream::GetFormat() const {
    return stream_->GetFormat();
}

void BassFileStream::SeekAsSeconds(double stream_time) const {
    stream_->Seek(stream_time);
}

uint32_t BassFileStream::GetSamples(void *buffer, uint32_t length) const {
    return stream_->GetSamples(buffer, length);
}

std::string_view BassFileStream::GetDescription() const noexcept {
    return Description;
}

void BassFileStream::SetDSDMode(DsdModes mode) noexcept {
    stream_->SetDSDMode(mode);
}

DsdModes BassFileStream::GetDsdMode() const noexcept {
    return stream_->GetDSDMode();
}

bool BassFileStream::IsDsdFile() const noexcept {
    return stream_->IsDsdFile();
}

uint32_t BassFileStream::GetDsdSampleRate() const {
    return stream_->GetDsdSampleRate();
}

uint32_t BassFileStream::GetSampleSize() const noexcept {
    return stream_->GetSampleSize();
}

DsdFormat BassFileStream::GetDsdFormat() const noexcept {
    return stream_->GetDsdFormat();
}

void BassFileStream::SetDsdToPcmSampleRate(uint32_t sample_rate) {
    stream_->SetDsdToPcmSampleRate(sample_rate);
}

uint32_t BassFileStream::GetDsdSpeed() const {
    return stream_->GetDsdSpeed();
}

uint32_t BassFileStream::GetBitDepth() const {
    return stream_->GetBitDepth();
}

uint32_t BassFileStream::GetHStream() const noexcept {
    return stream_->GetHStream();
}

bool BassFileStream::IsActive() const noexcept {
    return stream_->IsActive();
}

bool BassFileStream::SupportDOP() const noexcept {
    return stream_->SupportDOP();
}

bool BassFileStream::SupportDOP_AA() const noexcept {
    return stream_->SupportDOP_AA();
}

bool BassFileStream::SupportNativeSD() const noexcept {
    return stream_->SupportNativeSD();
}

Uuid BassFileStream::GetTypeId() const {
    return XAMP_UUID_OF(BassFileStream);
}

XAMP_STREAM_NAMESPACE_END
