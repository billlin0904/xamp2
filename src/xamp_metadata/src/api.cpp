#include <metadata/api.h>

#include <metadata/libcuelib.h>

#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

ScopedPtr<IMetadataReader> makeMetadataReader() {
	return makeAlign<IMetadataReader, TaglibMetadataReader>();
}

ScopedPtr<IMetadataWriter> makeMetadataWriter() {
	return makeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

const HashSet<std::string>& getSupportFileExtensions() {
	return TaglibMetadataReader::getSupportFileExtensions();
}

void loadCueLib() {
	SharedSingleton<LibCueLib>::getInstance();
}

XAMP_METADATA_NAMESPACE_END
