#include <metadata/imetadataextractadapter.h>
#include <metadata/imetadatareader.h>

namespace xamp::metadata {
	
void WalkPath(Path const & path, IMetadataExtractAdapter* adapter, IMetadataReader *reader) {
	adapter->OnWalkFirst();
	
    if (Fs::is_directory(path)) {
        Path root_path;

        for (auto const & file_or_dir : RecursiveDirectoryIterator(Fs::absolute(path), kIteratorOptions)) {
            auto const & current_path = file_or_dir.path();
            if (root_path.empty()) {
                root_path = current_path;
            }

            auto parent_path = root_path.parent_path();
            auto cur_path = current_path.parent_path();
            if (parent_path != cur_path) {
                adapter->OnWalkNext();
            	adapter->OnWalkFirst();
                root_path = current_path;
            }

            if (!Fs::is_directory(current_path)) {
                if (adapter->IsSupported(current_path)) {
                    adapter->OnWalk(path, reader->Extract(current_path));
                }
            }
        }

        adapter->OnWalkNext();
    }
    else {
        if (adapter->IsSupported(path)) {
            adapter->OnWalk(path, reader->Extract(path));
            adapter->OnWalkNext();
        }
    }
}

}