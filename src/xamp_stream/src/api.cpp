#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/ifileencoder.h>
#include <stream/bassfileencoder.h>
#include <stream/basscddevice.h>
#include <stream/api.h>

namespace xamp::stream {

using namespace xamp::base;

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

AlignPtr<FileStream> MakeStream() {
    return MakeAlign<FileStream, BassFileStream>();
}

AlignPtr<IFileEncoder> MakeEncoder() {
    return MakeAlign<IFileEncoder, BassFileEncoder>();
}
#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> MakeCDDevice(int32_t driver_letter) {
    return MakeAlign<ICDDevice, BassCDDevice>(static_cast<char>(driver_letter));
}
#endif
IDsdStream* AsDsdStream(AlignPtr<FileStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}
	
HashSet<std::string> const& GetSupportFileExtensions() {
    static const auto bass_file_ext = BassFileStream::GetSupportFileExtensions();
    return bass_file_ext;
}

}
