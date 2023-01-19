#include <base/platform.h>
#include <base/str_utilts.h>
#include <stream/podcastcache.h>

namespace xamp::stream {

std::string ToCacheID(Path const& file_path) {
    Path path(file_path);
    auto dot_pos = path.filename().string().find(".");
    auto cache_id = path.filename().string().substr(0, dot_pos);
    return cache_id;
}

PodcastFileCache::PodcastFileCache(std::string const& cache_id)
    : is_completed_(false)
    , cache_id_(cache_id) {
}

PodcastFileCache::~PodcastFileCache() = default;

void PodcastFileCache::SetTempPath(std::string const& file_ext, Path const& path) {
	const auto file_name = MakeTempFileName() + cache_id_ + file_ext;
    auto tmp_dir_path = path;
    tmp_dir_path /= file_name;
    path_ = tmp_dir_path;
    file_.open(path_, std::ios::binary);
}

[[nodiscard]] bool PodcastFileCache::IsOpen() const {
    return file_.is_open();
}

void PodcastFileCache::Write(void const* buf, size_t buf_size) {
    file_.write(static_cast<const char*>(buf), buf_size);
}

void PodcastFileCache::Close() {
    file_.close();
    is_completed_ = true;
}

Path PodcastFileCache::GetFilePath() const {
    return path_;
}

bool PodcastFileCache::IsCompleted() const noexcept {
    return is_completed_;
}

std::string PodcastFileCache::GetCacheID() const {
    return cache_id_;
}

PodcastFileCacheManager::PodcastFileCacheManager()
    : path_(Fs::temp_directory_path())
    , cache_(kPodcastFileCacheSize) {
}

std::shared_ptr<PodcastFileCache> PodcastFileCacheManager::GetOrAdd(std::string const& cache_id) {
    auto cache = cache_.GetOrAdd(cache_id, [cache_id, this]() {
        const auto file = std::make_shared<PodcastFileCache>(cache_id);
		file->SetTempPath(".m4a", path_);
        return file;
        });
    return cache->shared_from_this();
}

PodcastFileCacheManager& PodcastFileCacheManager::GetInstance() {
    return Singleton<PodcastFileCacheManager>::GetInstance();
}

void PodcastFileCacheManager::SetTempPath(Path const& path) {
	if (!path.empty()) {
        path_ = path;
	} else {
        path_ = Fs::temp_directory_path();
	}
}

void PodcastFileCacheManager::Remove(std::string const& cache_id) {
    cache_.Erase(cache_id);
}

void PodcastFileCacheManager::Remove(std::shared_ptr<PodcastFileCache> const & file_cache) {
    Remove(file_cache->GetCacheID());
}

std::shared_ptr<PodcastFileCache> GetPodcastFileCache(std::wstring const & file_path) {
	const auto cache_id = ToCacheID(file_path);
    return PodcastCache.GetOrAdd(cache_id);
}

}
