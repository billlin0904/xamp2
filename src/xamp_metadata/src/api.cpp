#include <metadata/api.h>

#include <metadata/libcuelib.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

AlignPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

AlignPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

const HashSet<std::string>& GetSupportFileExtensions() {
	return TaglibMetadataReader::GetSupportFileExtensions();
}

void LoadCueLib() {
	Singleton<LibCueLib>::GetInstance();
}

XAMP_METADATA_NAMESPACE_END
