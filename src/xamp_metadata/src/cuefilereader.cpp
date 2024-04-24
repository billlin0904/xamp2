#include <cuehandler.hpp>
#include <metadata/cuefilereader.h>

XAMP_METADATA_NAMESPACE_BEGIN

class CueFileReader::CueFileReaderImpl {
public:
	void Open(const Path& path) {
		
	}

	std::vector<CueTrack> GetTracks() const {
		return tracks_;
	}
	std::vector<CueTrack> tracks_;
};

CueFileReader::CueFileReader() 
	: impl_(MakeAlign<CueFileReaderImpl>()) {
}

XAMP_PIMPL_IMPL(CueFileReader)

void CueFileReader::Open(const Path& path) {
	impl_->Open(path);
}

std::vector<CueTrack> CueFileReader::GetTracks() const {
	return impl_->GetTracks();
}

XAMP_METADATA_NAMESPACE_END
