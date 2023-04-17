#include <metadata/api.h>

#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/imetadataextractadapter.h>
#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

AlignPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

AlignPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

XAMP_METADATA_NAMESPACE_END
