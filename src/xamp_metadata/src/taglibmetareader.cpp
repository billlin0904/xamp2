#include <metadata/taglibmetareader.h>
#include <metadata/taglibiostream.h>
#include <metadata/taglib.h>

#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/exception.h>
#include <base/fastmutex.h>

#include <vector>
#include <sstream>
#include <functional>
#include <regex>
#include <limits>

XAMP_METADATA_NAMESPACE_BEGIN
namespace {
    double ParseStringList(const std::string& s, bool string_dummy = true) {
        std::stringstream ss;
        ss << s;
        if (string_dummy) {
            std::string dummy;
            ss >> dummy;
        }
        double d = 0;
        ss >> d;
        return d;
    }

    std::optional<std::vector<std::byte>> CopyCoverData(const TagLib::ByteVector& data) {
        if (data.isEmpty()) {
            return std::nullopt;
        }
        std::vector<std::byte> buffer;
        buffer.resize(data.size());
        MemoryCopy(buffer.data(), data.data(), static_cast<int32_t>(data.size()));
        return MakeOptional<std::vector<std::byte>>(std::move(buffer));
    }

    std::optional<std::vector<std::byte>> ReadDefaultEmbeddedCover(
        const TagLib::List<TagLib::VariantMap>& pictures) {
        std::optional<std::vector<std::byte>> first_cover;
        for (const auto& picture : pictures) {
            const auto data = picture.value("data").value<TagLib::ByteVector>();
            auto cover = CopyCoverData(data);
            if (!cover) {
                continue;
            }
            const auto picture_type = picture.value("pictureType").value<TagLib::String>();
            if (picture_type == "Front Cover") {
                return cover;
            }
            if (!first_cover) {
                first_cover = std::move(cover);
            }
        }
        return first_cover;
    }

    std::optional<std::vector<std::byte>> ReadDefaultEmbeddedCover(const TagLib::Tag* tag) {
        if (!tag) {
            return std::nullopt;
        }
        return ReadDefaultEmbeddedCover(tag->complexProperties("PICTURE"));
    }

    std::optional<std::vector<std::byte>> ReadDefaultEmbeddedCover(const File* file_) {
        if (!file_) {
            return std::nullopt;
        }
        return ReadDefaultEmbeddedCover(file_->complexProperties("PICTURE"));
    }

    std::expected<ReplayGain, ParseMetadataError> ReadID3v2ReplayGain(const ID3v2::Tag* tag) {
        ReplayGain replay_gain;
        bool found = false;
        if (tag) {
            const auto& frame_list = tag->frameList("TXXX");
            for (auto* it : frame_list) {
                const auto* fr = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(it);
                if (fr) {
                    const auto desc = fr->description().upper();
                    const auto value = ParseStringList(fr->fieldList().toString().to8Bit());
                    if (desc == kReplaygainAlbumGain) {
                        replay_gain.album_gain = value;
                        found = true;
                    }
                    else if (desc == kReplaygainTrackGain) {
                        replay_gain.track_gain = value;
                        found = true;
                    }
                    else if (desc == kReplaygainAlbumPeak) {
                        replay_gain.album_peak = value;
                        found = true;
                    }
                    else if (desc == kReplaygainTrackPeak) {
                        replay_gain.track_peak = value;
                        found = true;
                    }
                    else if (desc == kReplaygainReferenceLoudness) {
                        replay_gain.ref_loudness = value;
                        found = true;
                    }
                }
            }
        }
        if (!found) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
        return replay_gain;
    }

    std::expected<ReplayGain, ParseMetadataError> ReadApeReplayGain(const APE::Tag* tag) {
        if (!tag) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        ReplayGain replay_gain;
        auto found = false;
        const auto& list_map = tag->itemListMap();
        auto read_field = [&list_map, &found](const std::string& name, double& value) {
            if (!list_map.contains(name)) {
                return;
            }
            const auto values = list_map[name].values();
            if (values.isEmpty()) {
                return;
            }
            value = ParseStringList(values.front().to8Bit(), false);
            found = true;
            };

        read_field(kReplaygainAlbumGain, replay_gain.album_gain);
        read_field(kReplaygainTrackGain, replay_gain.track_gain);
        read_field(kReplaygainAlbumPeak, replay_gain.album_peak);
        read_field(kReplaygainTrackPeak, replay_gain.track_peak);
        read_field(kReplaygainReferenceLoudness, replay_gain.ref_loudness);

        if (!found) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
        return replay_gain;
    }

    struct XAMP_NO_VTABLE IFileTagReader {
        virtual ~IFileTagReader() = default;
        virtual std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) = 0;
        virtual std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) = 0;
    };

    struct Mp3TagReader : public IFileTagReader {
        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
                return ReadID3v2ReplayGain(mp3_file->ID3v2Tag(false));
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct ApeTagReader : public IFileTagReader {
        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            if (auto* ape_file = dynamic_cast<TagLib::APE::File*>(file_)) {
                return ReadApeReplayGain(ape_file->APETag(false));
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct WavTagReader : public IFileTagReader {
        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            if (const auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
                return ReadID3v2ReplayGain(wav_file->ID3v2Tag());
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct DsfTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct DiffTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct Mp4TagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            ReplayGain replay_gain;
            auto found = false;
            if (const auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file_)) {
                if (const auto* tag = mp4_file->tag()) {
                    auto const& dict = tag->itemMap();
                    const auto track_gain = kITunesReplaygainTrackGain;
                    if (dict.contains(track_gain)) {
                        replay_gain.track_gain = ParseStringList(dict[track_gain].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto track_peak = kITunesReplaygainTrackPeak;
                    if (dict.contains(track_peak)) {
                        replay_gain.track_peak = ParseStringList(dict[track_peak].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto album_gain = kITunesReplaygainAlbumGain;
                    if (dict.contains(album_gain)) {
                        replay_gain.album_gain = ParseStringList(dict[album_gain].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto album_peak = kITunesReplaygainAlbumPeak;
                    if (dict.contains(String::AsStdString(album_peak))) {
                        replay_gain.album_peak = ParseStringList(dict[album_peak].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto reference_loudness = kITunesReplaygainReferenceLoudness;
                    if (dict.contains(reference_loudness)) {
                        replay_gain.album_peak = ParseStringList(dict[reference_loudness].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }

    };

    struct FlacTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            ReplayGain replay_gain;
            bool found = false;
            if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file_)) {
                if (const auto* xiph_comment = flac_file->xiphComment()) {
                    auto field_map = xiph_comment->fieldListMap();
                    for (auto& field : field_map) {
                        if (field.first == kReplaygainAlbumGain) {
                            replay_gain.album_gain = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainTrackPeak) {
                            replay_gain.track_peak = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainAlbumPeak) {
                            replay_gain.album_peak = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainTrackGain) {
                            replay_gain.track_gain = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainReferenceLoudness) {
                            replay_gain.ref_loudness = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                    }
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }
    };

    struct OpusTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) override {
            if (auto cover = ReadDefaultEmbeddedCover(file_)) {
                return cover.value();
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) override {
            auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file_);
            if (!opus_file || !opus_file->isValid()) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto* xiph_comment = opus_file->tag();
            if (!xiph_comment) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto get_dB = [](const TagLib::StringList& v) -> double {
                if (v.isEmpty()) return 0.0;
                return std::stod(v.front().to8Bit());
                };

            auto get_q78 = [](const TagLib::StringList& v) -> double {
                if (v.isEmpty()) return 0.0;
                return static_cast<double>(std::stoi(v.front().to8Bit())) / 256.0;
                };

            auto found = false;
            auto field_map = xiph_comment->fieldListMap();
            ReplayGain replay_gain;
            for (auto& field : field_map) {
                if (field.first == "R128_TRACK_GAIN") {             // RFC 7845
                    replay_gain.track_gain = get_q78(field.second); // Q7.8 → dB
                    found = true;
                }
                else if (field.first == "R128_ALBUM_GAIN") {        // RFC 7845
                    replay_gain.track_gain = get_q78(field.second); // Q7.8 → dB
                    found = true;
                }
                else if (field.first == kReplaygainAlbumGain) {
                    replay_gain.album_gain = get_dB(field.second);
                    found = true;
                }
                else if (field.first == kReplaygainTrackPeak) {
                    replay_gain.track_peak = get_dB(field.second);
                    found = true;
                }
                else if (field.first == kReplaygainAlbumPeak) {
                    replay_gain.album_peak = get_dB(field.second);
                    found = true;
                }
                else if (field.first == kReplaygainTrackGain) {
                    replay_gain.track_gain = get_dB(field.second);
                    found = true;
                }
                else if (field.first == kReplaygainReferenceLoudness) {
                    replay_gain.ref_loudness = get_dB(field.second);
                    found = true;
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }
    };

    const HashMap<std::string_view, std::function<ScopedPtr<IFileTagReader>()>>
        kFileTagReaderLut{
        { ".flac", [] { return MakeAlign<IFileTagReader, FlacTagReader>(); } },
        { ".mp3",  [] { return MakeAlign<IFileTagReader, Mp3TagReader>(); } },
        { ".wav",  [] { return MakeAlign<IFileTagReader, WavTagReader>(); } },
        { ".wave", [] { return MakeAlign<IFileTagReader, WavTagReader>(); } },
        { ".m4a",  [] { return MakeAlign<IFileTagReader, Mp4TagReader>(); } },
        { ".mp4",  [] { return MakeAlign<IFileTagReader, Mp4TagReader>(); } },
	    { ".opus", [] { return MakeAlign<IFileTagReader, OpusTagReader>(); } },
        { ".ape",  [] { return MakeAlign<IFileTagReader, ApeTagReader>(); } },
        { ".dff",  [] { return MakeAlign<IFileTagReader, DiffTagReader>(); } },
		{ ".dsf",  [] { return MakeAlign<IFileTagReader, DsfTagReader>(); } }
    };

    ScopedPtr<IFileTagReader> MakeFileTagReader(const std::string& ext) {
        const auto itr = kFileTagReaderLut.find(ext);
        if (itr != kFileTagReaderLut.end()) {
            return itr->second();
        }
        return nullptr;
    }

    void SetFileInfo(const Path& path, TrackInfo& track_info) {
        track_info.file_path = path;        
        try {
            track_info.file_size = Fs::file_size(path);
        }
        catch (...) {
        }
    }

    void SetAudioProperties(const AudioProperties* audio_properties, TrackInfo& track_info) {
        if (audio_properties != nullptr) {
            track_info.duration = audio_properties->lengthInMilliseconds() / 1000.0;
            track_info.bit_rate = audio_properties->bitrate();
            track_info.sample_rate = audio_properties->sampleRate();
        }
    }

    void ExtractTitleFromFileName(TrackInfo& track_info) {
        std::optional<std::wstring> file_name_no_ext(track_info.file_name_no_ext());

        if (file_name_no_ext) {
            std::wregex pattern(L"Track(\\d{2})\\.(.*)");
            std::wsmatch matches;

            if (std::regex_search(file_name_no_ext.value(), matches, pattern)) {
                track_info.track = std::stoi(matches[1].str());
				if (track_info.track > kMaxTrackNumber) {
					track_info.track = 0;
				}
                track_info.title = matches[2].str();
            }
            else {
                track_info.title = file_name_no_ext.value();
            }
        }
    }

    void ExtractTag(Path const& path, const Tag* tag, const AudioProperties* audio_properties, TrackInfo& track_info) {
        try {
            if (!tag->isEmpty()) {
                if (!tag->artist().toWString().empty()) {
                    track_info.artist = tag->artist().toWString();
                }
                if (!tag->title().toWString().empty()) {
                    track_info.title = tag->title().toWString();
                }
                if (!tag->album().toWString().empty()) {
                    track_info.album = tag->album().toWString();
                }                
                track_info.track = tag->track();
                track_info.year = tag->year();
                // 如果讀取到年分是20211126
                if (track_info.year > 0) {
                    if (track_info.year > 10000) {
                        track_info.year = track_info.year / 10000;
                    }
                    else if (track_info.year <= 1900) {
                        track_info.year = 1990;
                    }
                }
                else {
                    track_info.year = 0;
                }
                if (!tag->genre().toWString().empty()) {
                    track_info.genre = tag->genre().toWString();
                }
                if (!tag->comment().toWString().empty()) {
                    track_info.comment = tag->comment().toWString();
                }
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("ExtractTag path: {}", e.what());
        }        
    }

    class TaglibHelper {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        [[nodiscard]] HashSet<std::string> const& GetSupportFileExtensions() const {
            return support_file_extensions_;
        }

        [[nodiscard]] bool IsSupported(const std::string& file_ext) const {
            return support_file_extensions_.find(file_ext) != support_file_extensions_.end();
        }

        TaglibHelper() {
            for (const auto& file_exts : FileRef::defaultFileExtensions()) {
                support_file_extensions_.insert(std::string(".") + file_exts.toCString());
            }
            support_file_extensions_.erase(".mkv");
            support_file_extensions_.erase(".m4v");
        }
    private:
        HashSet<std::string> support_file_extensions_;
    };

    class LibarchiveIOStream : public TagLib::IOStream {
    public:
        explicit LibarchiveIOStream(ArchiveEntry entry)
            : entry(std::move(entry)) {
            ensureCached();
        }

        FileName name() const {
            return entry.Name().c_str();
        }

        void ensureCached() {
            if (!buffer_.empty())
                return;

            if (entry.Length() <= 0)
                throw std::runtime_error("ArchiveEntry length unknown or zero.");

            buffer_.resize(entry.Length());

            int64_t total_read = 0;
            while (total_read < entry.Length()) {
                auto chunk = entry.Read(buffer_.data() + total_read,
                    static_cast<size_t>(entry.Length() - total_read));
                if (!chunk) {
                    throw std::runtime_error(chunk.error().c_str());
                }
                if (chunk.value() <= 0)
                    throw std::runtime_error("ArchiveEntry Read failed or truncated.");
                total_read += chunk.value();
            }
            pos_ = 0;
        }

        void writeBlock(const TagLib::ByteVector&) override {
        }

        void insert(const ByteVector& data, offset_t start = 0, size_t replace = 0) override {
        }

        void removeBlock(offset_t start = 0, size_t length = 0) override {
        }

        bool readOnly() const override {
            return true;
        }

        bool isOpen() const override {
            return !buffer_.empty();
        }

        long long length() override {
            return static_cast<long long>(buffer_.size());
        }

        ByteVector readBlock(size_t length) {
            ensureCached();

            const auto offset = static_cast<size_t>(pos_);
            const size_t avail = buffer_.size() - offset;
            const size_t n = std::min<size_t>(length, avail);
            if (n > static_cast<size_t>((std::numeric_limits<unsigned int>::max)())) {
                throw std::runtime_error("ArchiveEntry read block too large.");
            }

            ByteVector out(buffer_.data() + offset, static_cast<unsigned int>(n));
            pos_ += static_cast<int64_t>(n);
            return out;
        }

        void seek(long long offset, Position p) override {
            ensureCached();

            long long base = 0;
            switch (p) {
            case Beginning: base = 0;              break;
            case Current:   base = pos_;           break;
            case End:       base = buffer_.size(); break;
            default:        return;
            }

            long long newPos = base + offset;
            if (newPos < 0 || newPos > static_cast<long long>(buffer_.size()))
                return;
            pos_ = newPos;
        }

        long long tell() const override {
            return pos_;
        }

        void truncate(long long) override {
        }
    private:
        int64_t pos_{};
        ArchiveEntry entry;
        std::vector<char> buffer_;
    };

    ScopedPtr<TagLib::IOStream> MakeIOStream(ArchiveEntry entry) {
        return MakeAlign<TagLib::IOStream, LibarchiveIOStream>(std::move(entry));
    }

    ScopedPtr<TagLib::IOStream> MakeIOStream(const Path& path, FastIOStream::Mode mode = FastIOStream::Mode::ReadWriteOnlyExisting) {
        return MakeAlign<TagLib::IOStream, TaglibIOStream>(path, mode);
    }
}

class TaglibMetadataReader::TaglibMetadataReaderImpl {
public:
    void Open(ArchiveEntry entry) {
        fileref_opt_ = std::nullopt;
        auto entry_name = entry.Name();
        auto archive_path = entry.ArchivePath();
        PrefetchFile(archive_path);
        io_stream_ = MakeIOStream(std::move(entry));
        FileRef fileref(io_stream_.get(), true, TagLib::AudioProperties::Fast);
        if (!fileref.isNull()) {
            fileref_opt_ = fileref;
            path_ = archive_path;
            entry_name_ = entry_name;
            file_ext_ = String::ToLower(Path(entry_name).extension().string());
            tag_reader_ = MakeFileTagReader(file_ext_);
            is_archive_file_ = true;
        }
    }

    void Open(const Path& path) {
        fileref_opt_ = std::nullopt;
        io_stream_ = MakeIOStream(path, FastIOStream::Mode::Read);
        FileRef fileref(io_stream_.get(), true, TagLib::AudioProperties::Fast);
        if (!fileref.isNull()) {
            fileref_opt_ = fileref;
            path_ = path;
            file_ext_ = String::ToLower(path_.extension().string());
            tag_reader_ = MakeFileTagReader(file_ext_);
        }
	}

    std::expected<TrackInfo, ParseMetadataError> Extract() const {
        TrackInfo track_info;

		if (!fileref_opt_) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
		}

        const auto& file_ref = *fileref_opt_;
        const auto* tag = file_ref.tag();

        if (is_archive_file_) {
            track_info.file_size = io_stream_->length();
            track_info.archive_entry_name = io_stream_->name().toString().toCWString();
            track_info.file_path = path_;
            track_info.is_zip_file = true;
            if (tag != nullptr) {
                ExtractTag(path_, tag, file_ref.audioProperties(), track_info);
            }
            SetAudioProperties(file_ref.audioProperties(), track_info);
			if (track_info.title.empty()) {
				track_info.title = Path(track_info.archive_entry_name.value()).stem().wstring();
            }
        }
        else {
            if (tag != nullptr) {
                ExtractTag(path_, tag, file_ref.audioProperties(), track_info);
            }
            SetFileInfo(path_, track_info);
            SetAudioProperties(file_ref.audioProperties(), track_info);
        }

        // Tag not empty but title maybe empty!
        if (tag) {
            if (track_info.title.empty()) {
                ExtractTitleFromFileName(track_info);
            }
        }
        auto replay_gain = ReadReplayGain(file_ref.file());
        if (replay_gain) {
            track_info.replay_gain = replay_gain.value();
        }
        return track_info;
    }

    std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover() const {
		if (!IsSupported()) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_SUPPORT);
		}

        if (!fileref_opt_ || !fileref_opt_->tag()) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
        }
        return ReadEmbeddedCover(fileref_opt_->file());
    }

    XAMP_ALWAYS_INLINE bool IsSupported() const {
		return SharedSingleton<TaglibHelper>::GetInstance().IsSupported(file_ext_);
    }

    std::expected<ReplayGain, ParseMetadataError> ReadReplayGain() const {
        if (!fileref_opt_) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
        }
        return ReadReplayGain(fileref_opt_->file());
    }

private:
    std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file_) const {
        if (tag_reader_ != nullptr) {
            return tag_reader_->ReadReplayGain(file_);
        }
        return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
    }

    std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file_) const {
        if (tag_reader_ != nullptr) {
            return tag_reader_->ReadEmbeddedCover(file_);
        }
        return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
    }

    bool is_archive_file_{ false };
    std::string file_ext_;
	std::wstring entry_name_;
    Path path_;
    std::optional<FileRef> fileref_opt_;
    ScopedPtr<IFileTagReader> tag_reader_;
    ScopedPtr<TagLib::IOStream> io_stream_;
};

XAMP_PIMPL_IMPL(TaglibMetadataReader)

TaglibMetadataReader::TaglibMetadataReader()
    : reader_(MakeAlign<TaglibMetadataReaderImpl>()) {
}

void TaglibMetadataReader::Open(ArchiveEntry archive_entry) {
    return reader_->Open(std::move(archive_entry));
}

void TaglibMetadataReader::Open(const Path& path) {
    return reader_->Open(path);
}

std::expected<TrackInfo, ParseMetadataError> TaglibMetadataReader::Extract() {
    return reader_->Extract();
}

std::expected<ReplayGain, ParseMetadataError> TaglibMetadataReader::ReadReplayGain() {
    return reader_->ReadReplayGain();
}

std::expected<std::vector<std::byte>, ParseMetadataError> TaglibMetadataReader::ReadEmbeddedCover() {
    return reader_->ReadEmbeddedCover();
}

const HashSet<std::string>& TaglibMetadataReader::GetSupportFileExtensions() {
    return SharedSingleton<TaglibHelper>::GetInstance().GetSupportFileExtensions();
}

bool TaglibMetadataReader::IsSupported() const {
    return reader_->IsSupported();
}

XAMP_METADATA_NAMESPACE_END
