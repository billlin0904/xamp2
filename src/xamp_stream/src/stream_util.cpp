#include <fstream>
#include <base/singleton.h>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <stream/stream_util.h>

namespace xamp::stream {

using namespace xamp::base;

template <typename T>
static HashSet<std::wstring> GetSupportFileExtensions() {
    HashSet<std::wstring> file_ext;
	for (const auto & ext : T::GetSupportFileExtensions()) {
        file_ext.insert(String::ToStdWString(ext));
	}
    return file_ext;
}

struct StreamSupportFileExtensions {
    friend class Singleton<StreamSupportFileExtensions>;
	
    StreamSupportFileExtensions()
	    : libav_file_support(GetSupportFileExtensions<AvFileStream>())
		, bass_file_support(GetSupportFileExtensions<BassFileStream>())
		, file_support(GetStreamSupportFileExtensions()) {
    }

    static HashSet<std::string> GetStreamSupportFileExtensions() {
        const auto av_file_ext = AvFileStream::GetSupportFileExtensions();
        XAMP_LOG_TRACE("Get AvFileStream support file ext: {}", String::Join(av_file_ext));

        const auto bass_file_ext = BassFileStream::GetSupportFileExtensions();
        XAMP_LOG_TRACE("Get BassFileStream support file ext: {}", String::Join(bass_file_ext));

        HashSet<std::string> support_file_ext;

        for (const auto& file_ext : Union<std::string>(av_file_ext, bass_file_ext)) {
            support_file_ext.insert(file_ext);
        }
        return support_file_ext;
    }

    HashSet<std::wstring> libav_file_support;
    HashSet<std::wstring> bass_file_support;
    HashSet<std::string> file_support;
};

static bool TestDsdFileFormat(std::string_view const & file_chunks) noexcept {
    static constexpr std::array<std::string_view, 2> knows_chunks{
        "DSD ", // .dsd file
        "FRM8"  // .dsdiff file
    };	
    return std::find(knows_chunks.begin(), knows_chunks.end(), file_chunks)
        != knows_chunks.end();
}

bool TestDsdFileFormatStd(std::wstring const& file_path) {
#ifdef XAMP_OS_WIN
    std::ifstream file(file_path, std::ios_base::binary);
#else
    std::ifstream file(String::ToString(file_path), std::ios_base::binary);
#endif
    if (!file.is_open()) {
        return false;
    }
    std::array<char, 4> buffer{ 0 };
    file.read(buffer.data(), buffer.size());
    if (file.gcount() < 4) {
        return false;
    }
    const std::string_view file_chunks{ buffer.data(), 4 };
    return TestDsdFileFormat(file_chunks);
}

bool TestDsdFileFormat(std::wstring const& file_path) {
    MemoryMappedFile file;
    file.Open(file_path);
    if (file.GetLength() < 4) {
        return false;
    }
    const std::string_view file_chunks{ static_cast<char const*>(file.GetData()), 4 };
    return TestDsdFileFormat(file_chunks);
}

AlignPtr<FileStream> MakeStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream) {
    static const auto& use_av = Singleton<StreamSupportFileExtensions>::GetInstance().libav_file_support;
    static const auto& use_bass = Singleton<StreamSupportFileExtensions>::GetInstance().bass_file_support;

    const auto is_use_av_stream = use_av.find(file_ext) != use_av.end();
    const auto is_use_bass_stream = use_bass.find(file_ext) != use_bass.end();

    if (old_stream != nullptr) {
        if (is_use_av_stream || is_use_bass_stream) {
            if (auto* stream = dynamic_cast<BassFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
        else {
            if (auto* stream = dynamic_cast<AvFileStream*>(old_stream.get())) {
                old_stream->Close();
                return old_stream;
            }
        }
    }

    if (is_use_av_stream) {
        return MakeAlign<FileStream, AvFileStream>();
    }
    return MakeAlign<FileStream, BassFileStream>();
}

DsdStream* AsDsdStream(AlignPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<DsdStream*>(stream.get());
}
	
HashSet<std::string> const & GetSupportFileExtensions() {
    return Singleton<StreamSupportFileExtensions>::GetInstance().file_support;
}

}
