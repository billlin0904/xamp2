#include <bass/bass.h>
#include <bass/bassdsd.h>

#include <base/dll.h>
#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>
#include <base/vmmemlock.h>

#include <stream/bassexception.h>
#include <stream/bassfilestream.h>

namespace xamp::stream {

constexpr DWORD BASS_ERROR{ 0xFFFFFFFF };
constexpr int32_t PCM_SAMPLE_RATE_441 = { 44100 };

template <typename T>
constexpr uint8_t HiByte(T val) noexcept {
    return static_cast<uint8_t>(val >> 8);
}

template <typename T>
constexpr uint8_t LowByte(T val) noexcept {
    return static_cast<uint8_t>(val);
}

template <typename T>
constexpr uint32_t HiWord(T val) noexcept {
    return static_cast<uint32_t>(uint16_t(val) >> 16);
}

template <typename T>
constexpr uint32_t LoWord(T val) noexcept {
    return static_cast<uint16_t>(val);
}

#define BassIfFailedThrow(result) \
	do {\
		if (!(result)) {\
			throw BassException(BassLib::Instance().BASS_ErrorGetCode());\
		}\
	} while (false)

static int32_t GetDOPSampleRate(int32_t dsd_speed) {
	switch (dsd_speed) {
		// 64x CD
	case 64:
		return 176400;
		// 128x CD
	case 128:
		return 352800;
		// 256x CD
	case 256:
		return 705600;
	default:
		throw NotSupportFormatException();
	}
}

class BassDSDLib final {
public:
	BassDSDLib() try
#ifdef _WIN32
		: module_(LoadDll("bassdsd.dll"))
#else
		: module_(LoadDll("libbassdsd.dylib"))
#endif
		, BASS_DSD_StreamCreateFile(module_, "BASS_DSD_StreamCreateFile") {
	}
	catch (const Exception & e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	XAMP_DISABLE_COPY(BassDSDLib)

private:
	ModuleHandle module_;

public:
	DllFunction<HSTREAM(BOOL, const void*, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
};

class BassLib final {
public:
    static XAMP_ALWAYS_INLINE BassLib & Instance() {
        static BassLib lib;
        return lib;
    }

	~BassLib() {
		plugins_.clear();

		try {
			BassLib::Instance().BASS_Free();
		}
		catch (const Exception & e) {
			XAMP_LOG_INFO("{}", e.what());
		}
	}

	XAMP_ALWAYS_INLINE bool IsLoaded() const noexcept {
		return !plugins_.empty();
	}

	void Load() {
		if (IsLoaded()) {
			return;
		}

		BassLib::Instance().BASS_Init(0, 44100, 0, nullptr, nullptr);
#ifdef _WIN32
		// Disable Media Foundation
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);		
		LoadPlugin("bass_aac.dll");
		LoadPlugin("bassflac.dll");
		LoadPlugin("bass_ape.dll");
#else
		LoadPlugin("bass_aac.dylib");
#endif		
	}

    XAMP_DISABLE_COPY(BassLib)

private:
	BassLib() try
#ifdef _WIN32
		: module_(LoadDll("bass.dll"))
#else
        : module_(LoadDll("libbass.dylib"))
#endif
		, BASS_Init(module_, "BASS_Init")
		, BASS_SetConfig(module_, "BASS_SetConfig")
		, BASS_PluginLoad(module_, "BASS_PluginLoad")
		, BASS_PluginGetInfo(module_, "BASS_PluginGetInfo")
        , BASS_Free(module_, "BASS_Free")
        , BASS_StreamCreateFile(module_, "BASS_StreamCreateFile")
        , BASS_ChannelGetInfo(module_, "BASS_ChannelGetInfo")
        , BASS_StreamFree(module_, "BASS_StreamFree")
        , BASS_PluginFree(module_, "BASS_PluginFree")
        , BASS_ChannelGetData(module_, "BASS_ChannelGetData")
        , BASS_ChannelGetLength(module_, "BASS_ChannelGetLength")
        , BASS_ChannelBytes2Seconds(module_, "BASS_ChannelBytes2Seconds")
        , BASS_ChannelSeconds2Bytes(module_, "BASS_ChannelSeconds2Bytes")
        , BASS_ChannelSetPosition(module_, "BASS_ChannelSetPosition")
        , BASS_ErrorGetCode(module_, "BASS_ErrorGetCode")
        , BASS_ChannelGetAttribute(module_, "BASS_ChannelGetAttribute")
		, BASS_ChannelSetAttribute(module_, "BASS_ChannelSetAttribute") {
	}
	catch (const Exception &e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

    ModuleHandle module_;

public:
	XAMP_DEFINE_DLL_API(BASS_Init) BASS_Init;
	XAMP_DEFINE_DLL_API(BASS_SetConfig) BASS_SetConfig;
	DllFunction<HPLUGIN(const char *, DWORD)> BASS_PluginLoad;
	XAMP_DEFINE_DLL_API(BASS_PluginGetInfo) BASS_PluginGetInfo;
	XAMP_DEFINE_DLL_API(BASS_Free) BASS_Free;
	DllFunction<HSTREAM(BOOL, const void *, QWORD, QWORD, DWORD)> BASS_StreamCreateFile;
	XAMP_DEFINE_DLL_API(BASS_ChannelGetInfo) BASS_ChannelGetInfo;
	XAMP_DEFINE_DLL_API(BASS_StreamFree) BASS_StreamFree;
	XAMP_DEFINE_DLL_API(BASS_PluginFree) BASS_PluginFree;
	XAMP_DEFINE_DLL_API(BASS_ChannelGetData) BASS_ChannelGetData;
    XAMP_DEFINE_DLL_API(BASS_ChannelGetLength) BASS_ChannelGetLength;
    XAMP_DEFINE_DLL_API(BASS_ChannelBytes2Seconds) BASS_ChannelBytes2Seconds;
    XAMP_DEFINE_DLL_API(BASS_ChannelSeconds2Bytes) BASS_ChannelSeconds2Bytes;
    XAMP_DEFINE_DLL_API(BASS_ChannelSetPosition) BASS_ChannelSetPosition;
    XAMP_DEFINE_DLL_API(BASS_ErrorGetCode) BASS_ErrorGetCode;
    XAMP_DEFINE_DLL_API(BASS_ChannelGetAttribute) BASS_ChannelGetAttribute;
	XAMP_DEFINE_DLL_API(BASS_ChannelSetAttribute) BASS_ChannelSetAttribute;

	AlignPtr<BassDSDLib> DSDLib;

private:
	void LoadPlugin(const std::string& file_name) {
		BassPluginHandle plugin(BassLib::Instance().BASS_PluginLoad(file_name.c_str(), 0));
		if (!plugin) {
			XAMP_LOG_DEBUG("Load {} failure.", file_name);
			return;
		}

		const auto info = BassLib::Instance().BASS_PluginGetInfo(plugin.get());
		const int major_version(HiByte(HiWord(info->version)));
		const int minor_version(LowByte(HiWord(info->version)));
		const int micro_version(HiByte(LoWord(info->version)));
		const int build_version(LowByte(LoWord(info->version)));

		std::ostringstream ostr;
		ostr << major_version << "."
			<< minor_version << "."
			<< micro_version << "."
			<< build_version;
		XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, ostr.str());

		plugins_[file_name] = std::move(plugin);
	}
	
	struct BassPluginLoadTraits final {
		static HPLUGIN invalid() noexcept {
			return 0;
		}

		static void close(HPLUGIN value) noexcept {
			BassLib::Instance().BASS_PluginFree(value);
		}
	};

	using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadTraits>;

	std::unordered_map<std::string, BassPluginHandle> plugins_;
};

struct BassStreamTraits final {
	static HSTREAM invalid() noexcept {
		return 0;
	}

	static void close(HSTREAM value) noexcept {
		BassLib::Instance().BASS_StreamFree(value);
	}
};

using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamTraits>;

static void EnsureDsdDecoderInit() {
	if (!BassLib::Instance().DSDLib) {
		BassLib::Instance().DSDLib = MakeAlign<BassDSDLib>();
		// TODO: Set hightest dsd2pcm samplerate?
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
	}
	
}

namespace DsdHelper {
static bool TestDsdFileFormat(const std::wstring& file_path) {
	BassStreamHandle stream;

	EnsureDsdDecoderInit();

	MemoryMappedFile file;
	file.Open(file_path);
	stream.reset(BassLib::Instance().DSDLib->BASS_DSD_StreamCreateFile(TRUE,
		file.GetData(),
		0,
		file.GetLength(),
		BASS_DSD_RAW | BASS_STREAM_DECODE,
		0));

	return stream.is_valid();
}
}

class BassFileStream::BassFileStreamImpl {
public:
	BassFileStreamImpl() noexcept
        : enable_file_mapped_(true)
		, mode_(DsdModes::DSD_MODE_PCM) {
		info_ = BASS_CHANNELINFO{};
	}

	void LoadFromFile(const std::wstring & file_path) {
        info_ = BASS_CHANNELINFO{};

        DWORD flags = 0;

        switch (mode_) {
        case DsdModes::DSD_MODE_PCM:
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

		file_.Close();

		if (mode_ == DsdModes::DSD_MODE_PCM && !TestDsdFileFormat(file_path)) {
			if (enable_file_mapped_) {
				file_.Open(file_path);
				stream_.reset(BassLib::Instance().BASS_StreamCreateFile(TRUE,
					file_.GetData(),
					0,
					file_.GetLength(),
					flags | BASS_STREAM_DECODE));
			}
			else {
				stream_.reset(BassLib::Instance().BASS_StreamCreateFile(FALSE,
					file_path.data(),
					0,
					0,
					flags | BASS_STREAM_DECODE | BASS_UNICODE));
			}
		} else {
			EnsureDsdDecoderInit();

			file_.Open(file_path);
            stream_.reset(BassLib::Instance().DSDLib->BASS_DSD_StreamCreateFile(TRUE,
				file_.GetData(),
				0,
				file_.GetLength(),
				flags | BASS_STREAM_DECODE,
				0));
			// BassLib DSD module default use 6dB gain. 
			// 不設定的話會爆音!
			BassLib::Instance().BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);

			PrefetchMemory(file_.GetData(), file_.GetLength());
		}
		
		if (!stream_) {
			throw BassException(BassLib::Instance().BASS_ErrorGetCode());
		}

        info_ = BASS_CHANNELINFO{};	
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelGetInfo(stream_.get(), &info_));

        if (GetFormat().GetChannels() != XAMP_MAX_CHANNEL) {
            throw NotSupportFormatException();
        }
	}

	void Close() {
		stream_.reset();		
        mode_ = DsdModes::DSD_MODE_PCM;
		info_ = BASS_CHANNELINFO{};
	}	

	bool IsDsdFile() const noexcept {
		assert(stream_.is_valid());
		return info_.ctype == BASS_CTYPE_STREAM_DSD;
	}

	bool TestDsdFileFormat(const std::wstring& file_path) const {
		return DsdHelper::TestDsdFileFormat(file_path);
	}

	int32_t GetSamples(void *buffer, int32_t length) const noexcept {
		return InternalGetSamples(buffer, length * GetSampleSize()) / GetSampleSize();
	}

	double GetDuration() const {
		assert(stream_.is_valid());
		const auto len = BassLib::Instance().BASS_ChannelGetLength(stream_.get(), BASS_POS_BYTE);
		return BassLib::Instance().BASS_ChannelBytes2Seconds(stream_.get(), len);
	}

	AudioFormat GetFormat() const {
		assert(stream_.is_valid());
		if (mode_ == DsdModes::DSD_MODE_NATIVE) {
            return AudioFormat(DataFormat::FORMAT_DSD,
							   info_.chans,
                               ByteFormat::SINT8,
                               GetDsdSampleRate());
        } else if (mode_ == DsdModes::DSD_MODE_DOP) {
            return AudioFormat(DataFormat::FORMAT_PCM,
                               info_.chans,
                               ByteFormat::FLOAT32,
                               GetDOPSampleRate(GetDsdSpeed()));
        }
        return AudioFormat(DataFormat::FORMAT_PCM,
						   info_.chans,
                           ByteFormat::FLOAT32,
                           info_.freq);
	}

	void Seek(double stream_time) const {
		assert(stream_.is_valid());
		const auto pos_bytes = BassLib::Instance().BASS_ChannelSeconds2Bytes(stream_.get(), stream_time);
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelSetPosition(stream_.get(), pos_bytes, BASS_POS_BYTE));
	}

    int32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelGetAttribute(stream_.get(), BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<int32_t>(rate);
    }

    bool SupportDOP() const noexcept {
        return true;
    }

    bool SupportDOP_AA() const noexcept {
        return false;
    }

    bool SupportRAW() const noexcept {
        return true;
    }

    void SetDSDMode(DsdModes mode) noexcept {
        mode_ = mode;
    }

    DsdModes GetDSDMode() const noexcept {
        return mode_;       
    }

	int32_t GetSampleSize() const noexcept {
		return mode_ == DsdModes::DSD_MODE_NATIVE ? sizeof(int8_t) : sizeof(float);
	}

	DsdSampleFormat GetDsdSampleFormat() const noexcept {
		return DsdSampleFormat::DSD_INT8MSB;
	}

	void SetDsdToPcmSampleRate(int32_t samplerate) {		
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, samplerate);
	}

    int32_t GetDsdSpeed() const noexcept {		
        return GetDsdSampleRate() / PCM_SAMPLE_RATE_441;
    }
private:
	XAMP_ALWAYS_INLINE int32_t InternalGetSamples(void *buffer, int32_t length) const noexcept {
        const auto byte_read = BassLib::Instance().BASS_ChannelGetData(stream_.get(), buffer, length);
		if (byte_read == BASS_ERROR) {
			return 0;
		}
        return int32_t(byte_read);
	}

	bool enable_file_mapped_;
    DsdModes mode_;
	BassStreamHandle stream_;
	BASS_CHANNELINFO info_;   
    MemoryMappedFile file_;
};

BassFileStream::BassFileStream()
	: stream_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::LoadBassLib() {
	if (!BassLib::Instance().IsLoaded()) {
		BassLib::Instance().Load();
	}
}

void BassFileStream::OpenFromFile(const std::wstring & file_path)  {
	stream_->LoadFromFile(file_path);
}

void BassFileStream::Close() {
	stream_->Close();
}

double BassFileStream::GetDuration() const {
	return stream_->GetDuration();
}

AudioFormat BassFileStream::GetFormat() const noexcept {
	return stream_->GetFormat();
}

void BassFileStream::Seek(double stream_time) const {
	stream_->Seek(stream_time);
}

int32_t BassFileStream::GetSamples(void *buffer, int32_t length) const noexcept {
	return stream_->GetSamples(buffer, length);
}

std::string_view BassFileStream::GetDescription() const noexcept {
    return "BASS";
}

DsdModes BassFileStream::GetSupportDsdMode() const noexcept {
	return DsdModes{ DsdModes::DSD_MODE_PCM | DsdModes::DSD_MODE_NATIVE | DsdModes::DSD_MODE_DOP };
}

void BassFileStream::SetDSDMode(DsdModes mode) noexcept {
    stream_->SetDSDMode(mode);
}

DsdModes BassFileStream::GetDsdMode() const noexcept {
    return stream_->GetDSDMode();
}

bool BassFileStream::TestDsdFileFormat(const std::wstring& file_path) const {
    return stream_->TestDsdFileFormat(file_path);
}

bool BassFileStream::IsDsdFile() const noexcept {
	return stream_->IsDsdFile();
}

int32_t BassFileStream::GetDsdSampleRate() const {
    return stream_->GetDsdSampleRate();
}

int32_t BassFileStream::GetSampleSize() const noexcept {
	return stream_->GetSampleSize();
}

DsdSampleFormat BassFileStream::GetDsdSampleFormat() const noexcept {
	return stream_->GetDsdSampleFormat();
}

void BassFileStream::SetDsdToPcmSampleRate(int32_t samplerate) {
	stream_->SetDsdToPcmSampleRate(samplerate);
}

int32_t BassFileStream::GetDsdSpeed() const noexcept {
    return stream_->GetDsdSpeed();
}

}
