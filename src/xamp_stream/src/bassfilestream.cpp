#include <bass/bass.h>
#include <bass/bassdsd.h>
#include <base/dll.h>

#include <base/exception.h>

#ifdef _WIN32
#include <base/unique_handle.h>
#endif

#include <base/memory.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>
#include <base/vmmemlock.h>

#include <stream/bassfilestream.h>

namespace xamp::stream {

const DWORD BASS_ERROR{ 0xFFFFFFFF };

static Errors TranslateBassError(int error) {
	switch (error) {
	case BASS_ERROR_FORMAT:
		return XAMP_ERROR_NOT_SUPPORT_FORMAT;
	default:
		return XAMP_ERROR_UNKNOWN;
	}
}

static const char* GetBassErrorMessage(int error) noexcept {
	switch (error) {
	case BASS_OK:
		return "All is OK";
	case BASS_ERROR_MEM:
		return "Memory error";
	case BASS_ERROR_FILEOPEN:
		return "Can't open the file";
	case BASS_ERROR_DRIVER:
		return "Can't find a free/valid driver";
	case BASS_ERROR_BUFLOST:
		return "The sample buffer was lost";
	case BASS_ERROR_HANDLE:
		return "Invalid handle";
	case BASS_ERROR_FORMAT:
		return "Unsupported sample format";
	case BASS_ERROR_POSITION:
		return "Invalid position";
	case BASS_ERROR_INIT:
		return "BASS_Init has not been successfully called";
	case BASS_ERROR_START:
		return "BASS_Start has not been successfully called";
	case BASS_ERROR_ALREADY:
		return "Already initialized/paused/whatever";
	case BASS_ERROR_NOCHAN:
		return "Can't get a free channel";
	case BASS_ERROR_ILLTYPE:
		return "An illegal type was specified";
	case BASS_ERROR_ILLPARAM:
		return "An illegal parameter was specified";
	case BASS_ERROR_NO3D:
		return "No 3D support";
	case BASS_ERROR_NOEAX:
		return "No EAX support";
	case BASS_ERROR_DEVICE:
		return "Illegal device number";
	case BASS_ERROR_NOPLAY:
		return "Not playing";
	case BASS_ERROR_FREQ:
		return "Illegal sample rate";
	case BASS_ERROR_NOTFILE:
		return "The stream is not a file stream";
	case BASS_ERROR_NOHW:
		return "No hardware voices available";
	case BASS_ERROR_EMPTY:
		return "The MOD music has no sequence data";
	case BASS_ERROR_NONET:
		return "No internet connection could be opened";
	case BASS_ERROR_CREATE:
		return "Couldn't create the file";
	case BASS_ERROR_NOFX:
		return "Effects are not available";
	case BASS_ERROR_NOTAVAIL:
		return "Requested data is not available";
	case BASS_ERROR_DECODE:
		return "The channel is/isn't a 'decoding channel'";
	case BASS_ERROR_DX:
		return "A sufficient DirectX version is not installed";
	case BASS_ERROR_TIMEOUT:
		return "Connection timedout";
	case BASS_ERROR_FILEFORM:
		return "Unsupported file format";
	case BASS_ERROR_SPEAKER:
		return "Unavailable speaker";
	case BASS_ERROR_VERSION:
		return "Invalid BASS version (used by add-ons)";
	case BASS_ERROR_CODEC:
		return "Codec is not available/supported";
	case BASS_ERROR_ENDED:
		return "The channel/file has ended";
	case BASS_ERROR_BUSY:
		return "The device is busy";
	case BASS_ERROR_UNKNOWN:
	default:
		return "Some other mystery problem";
	}
}

class BassException : public Exception {
public:
	explicit BassException(int error)
		: Exception(TranslateBassError(error), GetBassErrorMessage(error))
		, error_(error) {
	}

private:
	int error_;
};

#define BASS_IF_FAILED_THROW(result) \
	do {\
		if (!(result)) {\
			throw BassException(BassLib::Instance().BASS_ErrorGetCode());\
		}\
	} while (false)

class BassLib {
public:
    static BassLib & Instance() {
        static BassLib lib;
        return lib;
    }

    XAMP_DISABLE_COPY(BassLib)

private:
	BassLib() try
		: module_(LoadDll("bass.dll"))
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
        , BASS_ChannelGetAttribute(module_, "BASS_ChannelGetAttribute") {
	}
	catch (const Exception &e) {
		XAMP_LOG_INFO("{}", e.GetErrorMessage());
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
};

class BassDSDLib {
public:
	static BassDSDLib & Get() {
		static BassDSDLib lib;
		return lib;
	}

	XAMP_DISABLE_COPY(BassDSDLib)
	
private:
	BassDSDLib() try
		: module_(LoadDll("bassdsd.dll"))
		, BASS_DSD_StreamCreateFile(module_, "BASS_DSD_StreamCreateFile")
		, BASS_ChannelSetAttribute(module_, "BASS_ChannelSetAttribute") {
	}
	catch (const Exception& e) {
		XAMP_LOG_INFO("{}", e.GetErrorMessage());
	}

	ModuleHandle module_;

public:
	DllFunction<HSTREAM(BOOL, const void *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
	XAMP_DEFINE_DLL_API(BASS_ChannelSetAttribute) BASS_ChannelSetAttribute;
};

struct BassStreamTraits {
	static HSTREAM invalid() noexcept {
		return 0;
	}

	static void close(HSTREAM value) noexcept {
		BassLib::Instance().BASS_StreamFree(value);
	}
};

struct BassPluginLoadTraits {
	static HPLUGIN invalid() noexcept {
		return 0;
	}

	static void close(HPLUGIN value) noexcept {
		BassLib::Instance().BASS_PluginFree(value);
	}
};

using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamTraits>;
using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadTraits>;

class BassPlugin {
public:
	static BassPlugin& Instance() {
		static BassPlugin instance;
		return instance;
	}

	~BassPlugin() {
		if (IsLoaded()) {
			plugins_.clear();
		}
		try {
			BassLib::Instance().BASS_Free();
		}
		catch (const Exception& e) {
			XAMP_LOG_INFO("{}", e.what());
		}		
	}

	bool IsLoaded() const {
		return !plugins_.empty();
	}

	void Load() {
		BassLib::Instance().BASS_Init(0, 44100, 0, 0, nullptr);
#ifdef _WIN32
		// Media Foundation flac sometimes deadlock!
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
		LoadPlugin("bassdsd.dll");
#endif
	}

	XAMP_DISABLE_COPY(BassPlugin)

private:
	BassPlugin() {
	}

	void LoadPlugin(const std::string& file_name) {
		BassPluginHandle plugin(BassLib::Instance().BASS_PluginLoad(file_name.c_str(), 0));
		if (!plugin) {
			XAMP_LOG_DEBUG("Load {} failure.", file_name);
			return;
		}

		const auto info = BassLib::Instance().BASS_PluginGetInfo(plugin.get());
		const int major_version(HIBYTE(HIWORD(info->version)));
		const int minor_version(LOBYTE(HIWORD(info->version)));
		const int micro_version(HIBYTE(LOWORD(info->version)));
		const int build_version(LOBYTE(LOWORD(info->version)));

		std::ostringstream ostr;
		ostr << major_version << "." << minor_version << "." << micro_version << "." << build_version;
		XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, ostr.str());

		plugins_[file_name] = std::move(plugin);
	}

	std::unordered_map<std::string, BassPluginHandle> plugins_;
};

class BassFileStream::BassFileStreamImpl {
public:
	BassFileStreamImpl() noexcept
		: enable_file_mapped_(true)
		, mode_(DSDModes::DSD_MODE_PCM) {
		info_ = BASS_CHANNELINFO{};
	}

	~BassFileStreamImpl() noexcept = default;

	void LoadFromFile(const std::wstring & file_path) {
        if (!BassPlugin::Instance().IsLoaded()) {
			BassPlugin::Instance().Load();
        }

        info_ = BASS_CHANNELINFO{};

        DWORD flags = 0;

        switch (mode_) {
        case DSDModes::DSD_MODE_PCM:
            flags = BASS_SAMPLE_FLOAT;
            break;
        case DSDModes::DSD_MODE_DOP:
            flags = BASS_DSD_DOP;
            break;
        case DSDModes::DSD_MODE_DOP_AA:
            flags = BASS_DSD_DOP_AA;
            break;
        case DSDModes::DSD_MODE_RAW:
            flags = BASS_DSD_RAW;
            break;
		default:
			XAMP_NO_DEFAULT;
        }

		file_.Close();

		if (mode_ == DSDModes::DSD_MODE_PCM) {
			if (enable_file_mapped_) {
				file_.Open(file_path);
				flags |= BASS_ASYNCFILE;
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
			file_.Open(file_path);
			stream_.reset(BassDSDLib::Get().BASS_DSD_StreamCreateFile(TRUE,
				file_.GetData(),
				0,
				file_.GetLength(),
				flags | BASS_STREAM_DECODE,
				0));	
			PrefetchMemory((void*)file_.GetData(), file_.GetLength());
		}
		
		if (!stream_) {
			throw BassException(BassLib::Instance().BASS_ErrorGetCode());
		}

        info_ = BASS_CHANNELINFO{};	
        BASS_IF_FAILED_THROW(BassLib::Instance().BASS_ChannelGetInfo(stream_.get(), &info_));

        if (GetFormat().GetChannels() != XAMP_MAX_CHANNEL) {
            throw NotSupportFormatException();
        }
	}

	void Close() {
		stream_.reset();
        mode_ = DSDModes::DSD_MODE_PCM;
		info_ = BASS_CHANNELINFO{};
	}	

	bool IsDSDFile() const noexcept {
		return info_.ctype == BASS_CTYPE_STREAM_DSD;
	}

	int32_t GetSamples(void *buffer, int32_t length) const noexcept {
		return InternalGetSamples(buffer, length * GetSampleSize()) / GetSampleSize();
	}

	double GetDuration() const {
		const auto len = BassLib::Instance().BASS_ChannelGetLength(stream_.get(), BASS_POS_BYTE);
		return BassLib::Instance().BASS_ChannelBytes2Seconds(stream_.get(), len);
	}

	AudioFormat GetFormat() const {
		if (mode_ == DSDModes::DSD_MODE_RAW) {
			return AudioFormat(info_.chans, base::ByteFormat::SINT8, GetDSDSampleRate());
		}
		return AudioFormat(info_.chans, base::ByteFormat::FLOAT32, info_.freq);
	}

	void Seek(double stream_time) const {
		const auto pos_bytes = BassLib::Instance().BASS_ChannelSeconds2Bytes(stream_.get(), stream_time);
        BASS_IF_FAILED_THROW(BassLib::Instance().BASS_ChannelSetPosition(stream_.get(), pos_bytes, BASS_POS_BYTE));
	}

    int32_t GetDSDSampleRate() const {
        float rate = 0;
        BASS_IF_FAILED_THROW(BassLib::Instance().BASS_ChannelGetAttribute(stream_.get(), BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<int32_t>(rate);
    }

    bool SupportDOP() const noexcept {
        return false;
    }

    bool SupportDOP_AA() const noexcept {
        return false;
    }

    bool SupportRAW() const noexcept {
        return true;
    }

    void SetDSDMode(DSDModes mode) noexcept {
        mode_ = mode;
    }

    DSDModes GetDSDMode() const noexcept {
        return mode_;       
    }

	int32_t GetSampleSize() const noexcept {
		return mode_ == DSDModes::DSD_MODE_RAW ? sizeof(int8_t) : sizeof(float);
	}

	DSDSampleFormat GetDSDSampleFormat() const noexcept {
		return DSDSampleFormat::DSD_INT8MSB;
	}

	void SetPCMSampleRate(int32_t samplerate) {
		BassLib::Instance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, samplerate);
	}
private:
	XAMP_ALWAYS_INLINE int32_t InternalGetSamples(void *buffer, int32_t length) const noexcept {
		const auto byte_read = BassLib::Instance().BASS_ChannelGetData(stream_.get(), buffer, length);
		if (byte_read == BASS_ERROR) {
			return 0;
		}
		return byte_read;
	}

	bool enable_file_mapped_;
    DSDModes mode_;
	BassStreamHandle stream_;
	MemoryMappedFile file_;	    
	BASS_CHANNELINFO info_;
};


BassFileStream::BassFileStream()
	: stream_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::OpenFromFile(const std::wstring & file_path)  {
	stream_->LoadFromFile(file_path);
}

void BassFileStream::Close() {
	stream_->Close();
}

double BassFileStream::GetDuration() const {
	return stream_->GetDuration();
}

AudioFormat BassFileStream::GetFormat() const {
	return stream_->GetFormat();
}

void BassFileStream::Seek(double stream_time) const {
	stream_->Seek(stream_time);
}

int32_t BassFileStream::GetSamples(void *buffer, int32_t length) const noexcept {
	return stream_->GetSamples(buffer, length);
}

std::wstring BassFileStream::GetStreamName() const {
	return L"BASS";
}

bool BassFileStream::SupportDOP() const {
    return stream_->SupportDOP();
}

bool BassFileStream::SupportDOP_AA() const {
    return stream_->SupportDOP_AA();
}

bool BassFileStream::SupportRAW() const {
    return stream_->SupportRAW();
}

void BassFileStream::SetDSDMode(DSDModes mode) {
    stream_->SetDSDMode(mode);
}

DSDModes BassFileStream::GetDSDMode() const {
    return stream_->GetDSDMode();
}

bool BassFileStream::IsDSDFile() const noexcept {
    return stream_->IsDSDFile();
}

int32_t BassFileStream::GetDSDSampleRate() const {
    return stream_->GetDSDSampleRate();
}

int32_t BassFileStream::GetSampleSize() const {
	return stream_->GetSampleSize();
}

DSDSampleFormat BassFileStream::GetDSDSampleFormat() const {
	return stream_->GetDSDSampleFormat();
}

void BassFileStream::SetPCMSampleRate(int32_t samplerate) {
	stream_->SetPCMSampleRate(samplerate);
}

}
