#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/fastiostream.h>

#include <stream/basslib.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avlibfilestream.h>
#include <stream/mqafilestream.h>
#include <stream/ifileencoder.h>
#include <stream/bassparametriceq.h>
#include <stream/basscddevice.h>
#include <stream/dspmanager.h>
#include <stream/soxrlib.h>
#include <stream/srclib.h>
#include <stream/libavencoder.h>
#include <stream/discIdlib.h>
#include <stream/avlib.h>
#include <stream/api.h>

#ifdef XAMP_OS_WIN
#include <stream/r8brainlib.h>
#endif

XAMP_STREAM_NAMESPACE_BEGIN
namespace {
    bool IsDsdFileChunk(const std::string_view & file_chunks) {
        static constexpr std::array<std::string_view, 2> knows_chunks{
            "DSD ", // .dsd file
            "FRM8"  // .dsdiff file
        };

        for (auto &chunks : knows_chunks) {
            if (file_chunks.find(chunks) != std::string_view::npos) {
                return true;
            }
        }
        return false;
    }

    bool RequiresBassDsdStream(DsdModes dsd_mode) {
        switch (dsd_mode) {
        case DsdModes::DSD_MODE_DOP:
        case DsdModes::DSD_MODE_DOP_AA:
        case DsdModes::DSD_MODE_NATIVE:
        case DsdModes::DSD_MODE_DSD2PCM:
            return true;
        default:
            return false;
        }
    }

    void ConfigureDsdStream(const ScopedPtr<FileStream>& file_stream, DsdModes dsd_mode) {
        if (dsd_mode == DsdModes::DSD_MODE_PCM) {
            return;
        }

        auto* dsd_stream = asDsdStream(file_stream);
        if (dsd_stream == nullptr) {
            return;
        }

        switch (dsd_mode) {
        case DsdModes::DSD_MODE_DOP:
            failWith<NotSupportFormatException>(
                dsd_stream->supportDOP(),
                "Stream not support mode: {}", dsd_mode);
            break;
        case DsdModes::DSD_MODE_DOP_AA:
            failWith<NotSupportFormatException>(
                dsd_stream->supportDOP_AA(),
                "Stream not support mode: {}", dsd_mode);
            break;
        case DsdModes::DSD_MODE_NATIVE:
            failWith<NotSupportFormatException>(
                dsd_stream->supportNativeSD(),
                "Stream not support mode: {}", dsd_mode);
            break;
        case DsdModes::DSD_MODE_DSD2PCM:
        case DsdModes::DSD_MODE_AUTO:
        case DsdModes::DSD_MODE_PCM:
            break;
        default:
            throwException<NotSupportFormatException>(
                "Not support dsd-mode: {}.", dsd_mode);
            break;
        }
        dsd_stream->setDSDMode(dsd_mode);
    }
}

bool isDsdFile(const Path & path) {
    FastIOStream file_(path);
    std::array<char, 4> buffer{ 0 };
    auto readbytes = file_.read(buffer.data(), buffer.size());
    if (readbytes < 4) {
        return false;
    }
    const std::string_view file_chunks{ buffer.data(), 4 };
    return IsDsdFileChunk(file_chunks);
}

ScopedPtr<FileStream> StreamFactory::makeFileStream(const Path& filePath, bool use_mqa_decode) {
    auto dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
    if (!isDsdFile(filePath)) {
        dsd_mode = DsdModes::DSD_MODE_PCM;
    }
	return makeFileStream(filePath, dsd_mode, use_mqa_decode);
}

ScopedPtr<FileStream> StreamFactory::makeFileStream(const Path& file_path,
    DsdModes dsd_mode, 
    bool use_mqa_decode) {
    ScopedPtr<FileStream> file_stream;

    if (RequiresBassDsdStream(dsd_mode)) {
        file_stream = makeAlign<FileStream, BassFileStream>();
    }
    else if (use_mqa_decode) {
        try {
            MqaIdentifier identifier(file_path);
            if (identifier.detect() && identifier.isMQA()) {
                file_stream = makeAlign<FileStream, MqaFileStream>();
            }
            else {
                file_stream = makeAlign<FileStream, AvLibFileStream>();
            }
        }
        catch (...) {
            file_stream = makeAlign<FileStream, AvLibFileStream>();
        }
    }
    else {
		file_stream = makeAlign<FileStream, AvLibFileStream>();
    }

    ConfigureDsdStream(file_stream, dsd_mode);

    const auto allow_bass_fallback =
        !RequiresBassDsdStream(dsd_mode)
        && dynamic_cast<AvLibFileStream*>(file_stream.get()) != nullptr;

    try {
        file_stream->openFile(file_path);
    }
    catch (...) {
        if (!allow_bass_fallback) {
            throw;
        }

        XAMP_LOG_DEBUG("AvLibFileStream open failed, fallback to BassFileStream: {}",
            String::toUtf8String(file_path.wstring()));
        auto bass_file_stream = makeAlign<FileStream, BassFileStream>();
        ConfigureDsdStream(bass_file_stream, dsd_mode);
        bass_file_stream->openFile(file_path);
        file_stream = std::move(bass_file_stream);
    }
    return file_stream;
}

ScopedPtr<IFileEncoder> StreamFactory::makeFileEncoder() {
    return makeAlign<IFileEncoder, LibAbFileEncoder>();
}

ScopedPtr<IAudioProcessor> StreamFactory::makeParametricEq() {
    return makeAlign<IAudioProcessor, BassParametricEq>();
}

ScopedPtr<IDSPManager> StreamFactory::makeDSPManager() {
    return makeAlign<IDSPManager, DSPManager>();
}

#ifdef XAMP_OS_WIN
ScopedPtr<ICDDevice> StreamFactory::makeCDDevice(int32_t driver_letter) {
    return makeAlign<ICDDevice, BassCDDevice>(static_cast<char>(driver_letter));
}
#endif

IDsdStream* asDsdStream(ScopedPtr<FileStream> const& stream) {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* asFileStream(ScopedPtr<IAudioStream> const& stream) {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* asDsdStream(FileStream* stream) {
    return dynamic_cast<IDsdStream*>(stream);
}

std::expected<ArchiveFileStream, std::string> StreamFactory::makeArchiveFileStream(const Path& archive_path,
    const std::wstring& archive_entry_name) {
    ArchiveFile file_;
    
    auto enitities = file_.open(archive_path);
    if (enitities.has_value()) {
        auto archive_entiry = file_.getEntryByName(archive_entry_name);
        if (archive_entiry.has_value()) {
            auto file_stream = makeAlign<FileStream, BassFileStream>();
            file_stream->open(std::move(archive_entiry.value()));
			ArchiveFileStream result;
			result.archive_file = std::move(file_);
			result.file_stream = std::move(file_stream);
			return result;
        }
        return std::unexpected(archive_entiry.error());
    }
    return std::unexpected(enitities.error());
}

ScopedPtr<FileStream> StreamFactory::makeFileStream(ArchiveEntry archive_entry, 
    DsdModes dsd_mode) {
    auto file_stream = makeAlign<FileStream, BassFileStream>();

    ConfigureDsdStream(file_stream, dsd_mode);
    file_stream->open(std::move(archive_entry));
    return file_stream;
}

void loadBassLib() {
    if (!LIB_BASS.IsLoaded()) {
        SharedSingleton<BassLib>::getInstance().load();
    }
    LIB_BASS.MixLib = makeAlign<BassMixLib>();
    LIB_BASS.DSDLib = makeAlign<BassDSDLib>();
    LIB_BASS.FxLib = makeAlign<BassFxLib>();
#ifdef XAMP_OS_WIN
    LIB_BASS.CDLib = makeAlign<BassCDLib>();
#endif
    LIB_BASS.loadVersionInfo();
    for (const auto& info : LIB_BASS.getVersions()) {
        XAMP_LOG_DEBUG("DLL {} version: {}", info.first, info.second);
    }
}

OrderedMap<std::string, std::string> getBassDLLVersion() {
    return LIB_BASS.getVersions();
}

#ifdef XAMP_OS_WIN
void loadR8BrainLib() {
    SharedSingleton<R8brainLib>::getInstance();
}
void loadMBDiscIdLib() {
    SharedSingleton<DiscIdLib>::getInstance();
}
#endif

void loadAvLib() {
    SharedSingleton<AvLib>::getInstance();
}

void freeAvLib() {
    SharedSingleton<AvLib>::getInstance().Free();
}

void loadSoxrLib() {
    SharedSingleton<SoxrLib>::getInstance();
}

void loadSrcLib() {
    SharedSingleton<SrcLib>::getInstance();
}

XAMP_STREAM_NAMESPACE_END
