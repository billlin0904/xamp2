#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

#include <metadata/metadata_util.h>

namespace xamp::metadata {

AlignPtr<MetadataReader> MakeMetadataReader() {
	return MakeAlign<MetadataReader, TaglibMetadataReader>();
}

AlignPtr<MetadataWriter> MakeMetadataWriter() {
	return MakeAlign<MetadataWriter, TaglibMetadataWriter>();
}

}
