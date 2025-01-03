#include <metadata/api.h>

#include <metadata/libcuelib.h>
#include <metadata/chromaprintlib.h>

#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

ScopedPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

ScopedPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

const HashSet<std::string>& GetSupportFileExtensions() {
	return TaglibMetadataReader::GetSupportFileExtensions();
}

void LoadCueLib() {
	Singleton<LibCueLib>::GetInstance();
}

void LoadChromaprintLib() {
	Singleton<ChromaprintLib>::GetInstance();
}

XAMP_METADATA_NAMESPACE_END
