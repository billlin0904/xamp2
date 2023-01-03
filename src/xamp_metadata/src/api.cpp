#include <metadata/api.h>

#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/imetadataextractadapter.h>
#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

namespace xamp::metadata {

AlignPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

AlignPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

}
