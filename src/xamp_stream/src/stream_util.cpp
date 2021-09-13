#include <fstream>
#include <base/singleton.h>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <stream/dsdstream.h>
#include <stream/bassfilestream.h>
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

AlignPtr<FileStream> MakeStream() {
    return MakeAlign<FileStream, BassFileStream>();
}

DsdStream* AsDsdStream(AlignPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<DsdStream*>(stream.get());
}
	
HashSet<std::string> const& GetSupportFileExtensions() {
    static const auto bass_file_ext = BassFileStream::GetSupportFileExtensions();
    return bass_file_ext;
}

}
