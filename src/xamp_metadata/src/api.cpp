#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

#include <metadata/api.h>

namespace xamp::metadata {

AlignPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

AlignPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

}
