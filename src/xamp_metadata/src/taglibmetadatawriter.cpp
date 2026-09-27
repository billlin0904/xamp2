#include <metadata/taglib.h>
#include <metadata/taglibiostream.h>
#include <metadata/taglibmetawriter.h>

#include <base/exception.h>
#include <base/logger.h>
#include <base/platfrom_handle.h>
#include <base/fastiostream.h>
#include <base/platform.h>
#include <base/str_utilts.h>

#include <limits>
#include <optional>

#ifdef XAMP_OS_WIN
#include <Windows.h>
#endif

XAMP_DECLARE_LOG_NAME(TagLib);

namespace {
	class TagLibDebugListener : public TagLib::DebugListener {
	public:
		void printMessage(const TagLib::String& msg) override {
			using namespace xamp::base;
			static auto logger = XAMP_LOG_CREATE_LOGGER(TagLib);
			std::string temp(msg.toCString());
			String::remove(temp, "\n");
			XAMP_LOG_D(logger, temp);
		}
	};

	TagLibDebugListener debug_listener;
}

namespace TagLib {
	TagLibDebugListener* debugListener = &debug_listener;

	void setDebugListener(TagLibDebugListener* listener) {
		if (listener)
			debugListener = listener;
		else
			debugListener = &debug_listener;
	}
}

XAMP_METADATA_NAMESPACE_BEGIN

namespace {	
	class FileReplaceTransaction final {
	public:
		explicit FileReplaceTransaction(const Path& original_path)
			: original_path_(original_path)
			, temp_path_(makeTempPath(original_path)) {
			std::error_code ec;
			if (!Fs::copy_file(original_path_, temp_path_, Fs::copy_options::overwrite_existing, ec) || ec) {
				XAMP_LOG_DEBUG("Copy metadata source file failure.");
				throwException<PlatformException>("Copy metadata source file failure. ({})", ec.message());
			}
		}

		XAMP_DISABLE_COPY(FileReplaceTransaction)

		FileReplaceTransaction(FileReplaceTransaction&& other) noexcept
			: original_path_(std::move(other.original_path_))
			, temp_path_(std::move(other.temp_path_))
			, committed_(other.committed_) {
			other.committed_ = true;
		}

		FileReplaceTransaction& operator=(FileReplaceTransaction&& other) noexcept {
			if (this != &other) {
				cleanup();
				original_path_ = std::move(other.original_path_);
				temp_path_ = std::move(other.temp_path_);
				committed_ = other.committed_;
				other.committed_ = true;
			}
			return *this;
		}

		~FileReplaceTransaction() {
			cleanup();
		}

		[[nodiscard]] const Path& tempPath() const {
			return temp_path_;
		}

		void commit() {
			if (committed_) {
				return;
			}

#ifdef XAMP_OS_WIN
			if (!::MoveFileExW(temp_path_.c_str(),
				original_path_.c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
				throw PlatformException("Replace metadata file failure.", static_cast<int32_t>(::GetLastError()));
			}
#else
			std::error_code ec;
			Fs::rename(temp_path_, original_path_, ec);
			if (ec) {
				throw PlatformException("Replace metadata file failure.");
			}
#endif
			committed_ = true;
		}

	private:
		void cleanup() noexcept {
			if (committed_ || temp_path_.empty()) {
				return;
			}
			std::error_code ec;
			Fs::remove(temp_path_, ec);
			committed_ = true;
		}

		Path original_path_;
		Path temp_path_;
		bool committed_{ false };
	};

	bool clearTxxTag(ID3v2::Tag* tag,
		TagLib::String const& tag_name,
		double* old_content = nullptr) {
		const auto& frame_list = tag->frameList("TXXX");
		for (auto* it : frame_list) {
			auto* fr = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(it);
			if (fr && fr->description().upper() == tag_name) {
				if (old_content) {
					*old_content = std::stod(fr->fieldList().toString().to8Bit());
				}
				tag->removeFrame(fr);
				return true;
			}
		}
		return false;
	}

	void setTxxTag(ID3v2::Tag* tag, std::string const& tag_name, std::string const& value) {
		auto* txxx_frame = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, tag_name);
		if (!txxx_frame) {
			txxx_frame = new TagLib::ID3v2::UserTextIdentificationFrame();
			txxx_frame->setDescription(tag_name);
			tag->addFrame(txxx_frame);
		}
		txxx_frame->setText(value);
	}

	void writeID3v2ReplayGain(ID3v2::Tag* tag, const ReplayGain& replay_gain) {
		if (!tag) {
			return;
		}
		while (clearTxxTag(tag, kReplaygainAlbumGain)) {}
		while (clearTxxTag(tag, kReplaygainTrackGain)) {}
		while (clearTxxTag(tag, kReplaygainAlbumPeak)) {}
		while (clearTxxTag(tag, kReplaygainTrackPeak)) {}
		while (clearTxxTag(tag, kReplaygainReferenceLoudness)) {}
		setTxxTag(tag, kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
		setTxxTag(tag, kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
		setTxxTag(tag, kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
		setTxxTag(tag, kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
		setTxxTag(tag, kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
	}

	TagLib::List<TagLib::VariantMap> makeFrontCoverProperties(const TagLib::ByteVector& image_data) {
		TagLib::VariantMap picture;
		picture.insert("data", image_data);
		picture.insert("description", TagLib::String("cover"));
		picture.insert("pictureType", TagLib::String("Front Cover"));
		picture.insert("mimeType", TagLib::String("image/jpeg"));

		TagLib::List<TagLib::VariantMap> pictures;
		pictures.append(picture);
		return pictures;
	}

	bool writeDefaultEmbeddedCover(TagLib::Tag* tag, const TagLib::ByteVector& image_data) {
		if (!tag) {
			return false;
		}
		return tag->setComplexProperties("PICTURE", makeFrontCoverProperties(image_data));
	}

	bool writeDefaultEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) {
		if (!file_) {
			return false;
		}
		return file_->setComplexProperties("PICTURE", makeFrontCoverProperties(image_data));
	}

	bool removeDefaultEmbeddedCover(TagLib::Tag* tag) {
		if (!tag) {
			return false;
		}
		TagLib::List<TagLib::VariantMap> pictures;
		return tag->setComplexProperties("PICTURE", pictures);
	}

	bool removeDefaultEmbeddedCover(File* file_) {
		if (!file_) {
			return false;
		}
		TagLib::List<TagLib::VariantMap> pictures;
		return file_->setComplexProperties("PICTURE", pictures);
	}

	struct XAMP_NO_VTABLE IFileTagWriter {
		virtual ~IFileTagWriter() = default;
		virtual void writeReplayGain(const ReplayGain& replay_gain, File* file_) = 0;
		virtual void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) = 0;
		virtual void removeEmbeddedCover(File* file_) = 0;
	};

	struct FlacTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file_)) {
				Ogg::XiphComment* comment = nullptr;
				if (!flac_file->hasXiphComment()) {
					comment = flac_file->xiphComment(true);
				}
				else {
					comment = flac_file->xiphComment(false);
				}
				comment->addField(kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
				comment->addField(kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
				comment->addField(kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
				comment->addField(kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
				comment->addField(kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			writeDefaultEmbeddedCover(file_, image_data);
		}

		void removeEmbeddedCover(File* file_) override {
			removeDefaultEmbeddedCover(file_);
		}
	};

	struct Mp3TagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				writeID3v2ReplayGain(mp3_file->ID3v2Tag(true), replay_gain);
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				writeDefaultEmbeddedCover(mp3_file->ID3v2Tag(true), image_data);
			}
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				removeDefaultEmbeddedCover(mp3_file->ID3v2Tag(true));
			}
		}
	};

	struct WavTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				writeID3v2ReplayGain(wav_file->ID3v2Tag(), replay_gain);
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				writeDefaultEmbeddedCover(wav_file->ID3v2Tag(), image_data);
			}
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				removeDefaultEmbeddedCover(wav_file->ID3v2Tag());
			}
		}
	};

	struct DsfTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				writeID3v2ReplayGain(dsf_file->tag(), replay_gain);
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				writeDefaultEmbeddedCover(dsf_file->tag(), image_data);
			}
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				removeDefaultEmbeddedCover(dsf_file->tag());
			}
		}
	};

	struct DiffTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				writeID3v2ReplayGain(diff_file->ID3v2Tag(true), replay_gain);
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				writeDefaultEmbeddedCover(diff_file->ID3v2Tag(true), image_data);
			}
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				removeDefaultEmbeddedCover(diff_file->ID3v2Tag(false));
			}
		}
	};

	struct Mp4TagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(file_->tag())) {
				mp4_tag->setItem(kITunesReplaygainTrackGain,
					TagLib::StringList(std::to_string(replay_gain.track_gain)));
				mp4_tag->setItem(kITunesReplaygainTrackPeak,
					TagLib::StringList(std::to_string(replay_gain.track_peak)));
				mp4_tag->setItem(kITunesReplaygainAlbumGain,
					TagLib::StringList(std::to_string(replay_gain.album_gain)));
				mp4_tag->setItem(kITunesReplaygainAlbumPeak,
					TagLib::StringList(std::to_string(replay_gain.album_peak)));
				mp4_tag->setItem(kITunesReplaygainReferenceLoudness,
					TagLib::StringList(std::to_string(replay_gain.ref_loudness)));
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			writeDefaultEmbeddedCover(file_, image_data);
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file_)) {
				mp4_file->tag()->removeItem("covr");
			}
		}
	};

	struct OpusTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			auto* opus = dynamic_cast<Ogg::Opus::File*>(file_);
			if (!opus || !opus->isValid())
				return;

			auto* tag = opus->tag();
			if (!tag) {
				return;
			}

			tag->removeFields("R128_TRACK_GAIN");
			tag->removeFields("R128_ALBUM_GAIN");
			auto toQ78 = [](double db) -> std::string {
				return std::to_string(static_cast<int>(std::round(db * 256.0)));
				};

			tag->addField("R128_TRACK_GAIN", toQ78(replay_gain.track_gain));  // dB → Q7.8
			tag->addField("R128_ALBUM_GAIN", toQ78(replay_gain.album_gain));

			/* Opus 不是用 REPLAYGAIN_*，也沒有 peak 欄位的標準；
		   若你仍想保留舊欄位，可自行加上：
		   tag->addField("REPLAYGAIN_TRACK_PEAK", ...); */
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file_);
			if (!opus_file || !opus_file->isValid()) {
				return;
			}

			auto* tag = opus_file->tag();
			if (!tag) {				
				return;
			}
			writeDefaultEmbeddedCover(tag, image_data);
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* const opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file_)) {
				auto* tag = opus_file->tag();
				removeDefaultEmbeddedCover(tag);
			}
		}
	};

	struct ApeTagWriter : public IFileTagWriter {
		void writeReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* ape_file = dynamic_cast<APE::File*>(file_)) {
				if (auto* tag = ape_file->APETag(true)) {
					tag->addValue(kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
					tag->addValue(kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
					tag->addValue(kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
					tag->addValue(kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
					tag->addValue(kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
				}
			}
		}

		void writeEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* ape_file = dynamic_cast<APE::File*>(file_)) {
				if (auto* tag = ape_file->APETag(true)) {
					writeDefaultEmbeddedCover(tag, image_data);
				}
			}
		}

		void removeEmbeddedCover(File* file_) override {
			if (auto* ape_file = dynamic_cast<APE::File*>(file_)) {
				if (auto* tag = ape_file->APETag(false)) {
					removeDefaultEmbeddedCover(tag);
				}
			}
		}
	};

	HashMap<std::string_view, std::function<ScopedPtr<IFileTagWriter>()>>
		kFileTagWriterLut{
		{ ".flac", [] { return makeAlign<IFileTagWriter, FlacTagWriter>(); } },
		{ ".mp3",  [] { return makeAlign<IFileTagWriter, Mp3TagWriter>(); } },
		{ ".wav",  [] { return makeAlign<IFileTagWriter, WavTagWriter>(); } },
		{ ".wave", [] { return makeAlign<IFileTagWriter, WavTagWriter>(); } },
		{ ".m4a",  [] { return makeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".mp4",  [] { return makeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".opus",  [] { return makeAlign<IFileTagWriter, OpusTagWriter>(); } },
		{ ".ape",  [] { return makeAlign<IFileTagWriter, ApeTagWriter>(); } },
		{ ".dff",  [] { return makeAlign<IFileTagWriter, DiffTagWriter>(); } },
		{ ".dsf",  [] { return makeAlign<IFileTagWriter, DsfTagWriter>(); } }
	};

	ScopedPtr<IFileTagWriter> makeFileTagWriter(const std::string &ext) {
		auto itr = kFileTagWriterLut.find(ext);
		if (itr != kFileTagWriterLut.end()) {
			return std::invoke(itr->second);
		}
		return nullptr;
	}
}

#define CheckFileRef() \
	if (!fileref_opt_) {\
		XAMP_LOG_ERROR("file ref is null!");\
		return;\
		}


class TaglibMetadataWriter::TaglibMetadataWriterImpl {
public:
	TaglibMetadataWriterImpl() = default;

	~TaglibMetadataWriterImpl() {
		try {
			save();
		}
		catch (const std::exception& e) {
			XAMP_LOG_DEBUG("write tag failure: {}", e.what());
		}
	}

	void open(const Path& path) {
		save();
		clear();

		transaction_.emplace(path);
		io_stream_.open(transaction_->tempPath());
		FileRef fileref(&io_stream_);
		if (fileref.isNull()) {
			XAMP_LOG_DEBUG("file was NULL!");
			clear();
			return;
		}
		if (!fileref.tag()) {
			XAMP_LOG_DEBUG("tag is NULL!");
			clear();
			return;
		}
		fileref_opt_ = fileref;
		path_ = path;
		const auto ext = String::toLower(path_.extension().string());
		tag_writer_ = makeFileTagWriter(ext);
	}

    void write(const TrackInfo &track_info) const {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTrack(track_info.track);
		tag->setAlbum(track_info.album);
		tag->setArtist(track_info.artist);
		tag->setTitle(track_info.title);
		tag->setComment(track_info.comment);
		markDirty();
    }

    void writeTitle(const std::wstring &title) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTitle(title.empty() ? TagLib::String() : title);
		markDirty();
    }

    void writeArtist(const std::wstring &artist) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setArtist(artist.empty() ? TagLib::String() : artist);
		markDirty();
    }

    void writeAlbum(const std::wstring &album) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setAlbum(album.empty() ? TagLib::String() : album);
		markDirty();
    }

    void writeTrack(uint32_t track) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTrack(track);
		markDirty();
    }

	void writeComment(const std::wstring& comment) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setComment(comment.empty() ? TagLib::String() : comment);
		markDirty();
	}

	void writeGenre(const std::wstring& genre) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setGenre(genre.empty() ? TagLib::String() : genre);
		markDirty();
	}

	void writeYear(uint32_t year) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setYear(year);
		markDirty();
	}

	void writeReplayGain(const ReplayGain & replay_gain) {
		CheckFileRef()

		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->writeReplayGain(replay_gain, file_);
			markDirty();
		}
	}

	void writeEmbeddedCover(const uint8_t *image, size_t image_size) const {
		CheckFileRef()
		if (image_size > static_cast<size_t>((std::numeric_limits<unsigned int>::max)())) {
			throw PlatformException();
		}

		const TagLib::ByteVector image_data(reinterpret_cast<const char*>(image), static_cast<unsigned int>(image_size));
		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->writeEmbeddedCover(file_, image_data);
			markDirty();
		}
	}

	void writeEmbeddedCover(const std::vector<uint8_t> & image) const {
		CheckFileRef()
		writeEmbeddedCover(image.data(), image.size());
    }

	void removeEmbeddedCover() {
		CheckFileRef()
		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->removeEmbeddedCover(file_);
			markDirty();
		}
	}

	[[nodiscard]] bool canWriteEmbeddedCover() const {
		const auto ext = String::toLower(path_.extension().string());
        return kFileTagWriterLut.find(ext) != kFileTagWriterLut.end();
	}
private:
	void save() {
		if (!fileref_opt_) {
			return;
		}

		if (!dirty_) {
			clear();
			return;
		}
			
		if (!fileref_opt_->save()) {
			XAMP_LOG_DEBUG("write tag failure!");
			clear();
			return;
		}

		closeWorkingFile();
		if (transaction_) {
			transaction_->commit();
		}
		clear();
    }

	void closeWorkingFile() {
		fileref_opt_.reset();
		io_stream_.close();
	}

	void clear() {
		fileref_opt_.reset();
		io_stream_.close();
		transaction_.reset();
		tag_writer_.reset();
		dirty_ = false;
	}

	void markDirty() const {
		dirty_ = true;
	}

	Path path_;
	std::optional<FileRef> fileref_opt_;
	std::optional<FileReplaceTransaction> transaction_;
	ScopedPtr<IFileTagWriter> tag_writer_;
	TaglibIOStream io_stream_;
	mutable bool dirty_{ false };
};

XAMP_PIMPL_IMPL(TaglibMetadataWriter)

TaglibMetadataWriter::TaglibMetadataWriter()
    : writer_(makeAlign<TaglibMetadataWriterImpl>()) {
}

void TaglibMetadataWriter::open(const Path& path) {
	return writer_->open(path);
}

void TaglibMetadataWriter::writeReplayGain(const ReplayGain& replay_gain) {
	return writer_->writeReplayGain(replay_gain);
}

void TaglibMetadataWriter::write(const TrackInfo& track_info) {
	writer_->write(track_info);
}

void TaglibMetadataWriter::writeTitle(const std::wstring & title) {
    writer_->writeTitle(title);
}

void TaglibMetadataWriter::writeArtist(const std::wstring& artist) {
	writer_->writeArtist(artist);
}

void TaglibMetadataWriter::writeTrack(uint32_t track) {
	writer_->writeTrack(track);
}

void TaglibMetadataWriter::writeComment(const std::wstring& comment) {
	writer_->writeComment(comment);
}

void TaglibMetadataWriter::writeGenre(const std::wstring& genre) {
	writer_->writeGenre(genre);
}

void TaglibMetadataWriter::writeYear(uint32_t year) {
	writer_->writeYear(year);
}

void TaglibMetadataWriter::writeAlbum(const std::wstring & album) {
    writer_->writeAlbum(album);
}

void TaglibMetadataWriter::writeEmbeddedCover(const std::vector<uint8_t> & image) const {
	writer_->writeEmbeddedCover(image);
}

void TaglibMetadataWriter::removeEmbeddedCover() {
	writer_->removeEmbeddedCover();
}

bool TaglibMetadataWriter::canWriteEmbeddedCover() const {
	return writer_->canWriteEmbeddedCover();
}

XAMP_METADATA_NAMESPACE_END
