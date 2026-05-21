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
			String::Remove(temp, "\n");
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
			, temp_path_(MakeTempPath(original_path)) {
			std::error_code ec;
			if (!Fs::copy_file(original_path_, temp_path_, Fs::copy_options::overwrite_existing, ec) || ec) {
				throw PlatformException("Copy metadata source file failure.");
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
				Cleanup();
				original_path_ = std::move(other.original_path_);
				temp_path_ = std::move(other.temp_path_);
				committed_ = other.committed_;
				other.committed_ = true;
			}
			return *this;
		}

		~FileReplaceTransaction() {
			Cleanup();
		}

		[[nodiscard]] const Path& TempPath() const {
			return temp_path_;
		}

		void Commit() {
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
		static Path MakeTempPath(const Path& original_path) {
			constexpr auto kMaxRetryCreateTempFile = 128;
			const auto dir = original_path.parent_path();
			const auto stem = original_path.stem().wstring();
			const auto ext = original_path.extension().wstring();

			for (auto i = 0; i < kMaxRetryCreateTempFile; ++i) {
				const auto file_name = stem
					+ L".xamp-"
					+ String::ToStdWString(GetSequentialUUID())
					+ ext;
				auto temp_path = dir.empty()
					? Path(file_name)
					: dir / file_name;
				std::error_code ec;
				if (!Fs::exists(temp_path, ec) && !ec) {
					return temp_path;
				}
			}
			throw PlatformException("Create metadata temp file path failure.");
		}

		void Cleanup() noexcept {
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

	bool ClearTxxTag(ID3v2::Tag* tag,
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

	void SetTxxTag(ID3v2::Tag* tag, std::string const& tag_name, std::string const& value) {
		auto* txxx_frame = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, tag_name);
		if (!txxx_frame) {
			txxx_frame = new TagLib::ID3v2::UserTextIdentificationFrame();
			txxx_frame->setDescription(tag_name);
			tag->addFrame(txxx_frame);
		}
		txxx_frame->setText(value);
	}

	void WriteID3v2ReplayGain(ID3v2::Tag* tag, const ReplayGain& replay_gain) {
		if (!tag) {
			return;
		}
		while (ClearTxxTag(tag, kReplaygainAlbumGain)) {}
		while (ClearTxxTag(tag, kReplaygainTrackGain)) {}
		while (ClearTxxTag(tag, kReplaygainAlbumPeak)) {}
		while (ClearTxxTag(tag, kReplaygainTrackPeak)) {}
		while (ClearTxxTag(tag, kReplaygainReferenceLoudness)) {}
		SetTxxTag(tag, kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
		SetTxxTag(tag, kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
		SetTxxTag(tag, kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
		SetTxxTag(tag, kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
		SetTxxTag(tag, kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
	}

	TagLib::List<TagLib::VariantMap> MakeFrontCoverProperties(const TagLib::ByteVector& image_data) {
		TagLib::VariantMap picture;
		picture.insert("data", image_data);
		picture.insert("description", TagLib::String("cover"));
		picture.insert("pictureType", TagLib::String("Front Cover"));
		picture.insert("mimeType", TagLib::String("image/jpeg"));

		TagLib::List<TagLib::VariantMap> pictures;
		pictures.append(picture);
		return pictures;
	}

	bool WriteDefaultEmbeddedCover(TagLib::Tag* tag, const TagLib::ByteVector& image_data) {
		if (!tag) {
			return false;
		}
		return tag->setComplexProperties("PICTURE", MakeFrontCoverProperties(image_data));
	}

	bool WriteDefaultEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) {
		if (!file_) {
			return false;
		}
		return file_->setComplexProperties("PICTURE", MakeFrontCoverProperties(image_data));
	}

	bool RemoveDefaultEmbeddedCover(TagLib::Tag* tag) {
		if (!tag) {
			return false;
		}
		TagLib::List<TagLib::VariantMap> pictures;
		return tag->setComplexProperties("PICTURE", pictures);
	}

	bool RemoveDefaultEmbeddedCover(File* file_) {
		if (!file_) {
			return false;
		}
		TagLib::List<TagLib::VariantMap> pictures;
		return file_->setComplexProperties("PICTURE", pictures);
	}

	struct XAMP_NO_VTABLE IFileTagWriter {
		virtual ~IFileTagWriter() = default;
		virtual void WriteReplayGain(const ReplayGain& replay_gain, File* file_) = 0;
		virtual void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) = 0;
		virtual void RemoveEmbeddedCover(File* file_) = 0;
	};

	struct FlacTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
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

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			WriteDefaultEmbeddedCover(file_, image_data);
		}

		void RemoveEmbeddedCover(File* file_) override {
			RemoveDefaultEmbeddedCover(file_);
		}
	};

	struct Mp3TagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				WriteID3v2ReplayGain(mp3_file->ID3v2Tag(true), replay_gain);
			}
		}

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				WriteDefaultEmbeddedCover(mp3_file->ID3v2Tag(true), image_data);
			}
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file_)) {
				RemoveDefaultEmbeddedCover(mp3_file->ID3v2Tag(true));
			}
		}
	};

	struct WavTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				WriteID3v2ReplayGain(wav_file->ID3v2Tag(), replay_gain);
			}
		}

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				WriteDefaultEmbeddedCover(wav_file->ID3v2Tag(), image_data);
			}
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* wav_file = dynamic_cast<TagLib::RIFF::WAV::File*>(file_)) {
				RemoveDefaultEmbeddedCover(wav_file->ID3v2Tag());
			}
		}
	};

	struct DsfTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				WriteID3v2ReplayGain(dsf_file->tag(), replay_gain);
			}
		}

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				WriteDefaultEmbeddedCover(dsf_file->tag(), image_data);
			}
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* dsf_file = dynamic_cast<TagLib::DSF::File*>(file_)) {
				RemoveDefaultEmbeddedCover(dsf_file->tag());
			}
		}
	};

	struct DiffTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				WriteID3v2ReplayGain(diff_file->ID3v2Tag(true), replay_gain);
			}
		}

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				WriteDefaultEmbeddedCover(diff_file->ID3v2Tag(true), image_data);
			}
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* diff_file = dynamic_cast<TagLib::DSDIFF::File*>(file_)) {
				RemoveDefaultEmbeddedCover(diff_file->ID3v2Tag(false));
			}
		}
	};

	struct Mp4TagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
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

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			WriteDefaultEmbeddedCover(file_, image_data);
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file_)) {
				mp4_file->tag()->removeItem("covr");
			}
		}
	};

	struct OpusTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
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

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file_);
			if (!opus_file || !opus_file->isValid()) {
				return;
			}

			auto* tag = opus_file->tag();
			if (!tag) {				
				return;
			}
			WriteDefaultEmbeddedCover(tag, image_data);
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* const opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file_)) {
				auto* tag = opus_file->tag();
				RemoveDefaultEmbeddedCover(tag);
			}
		}
	};

	struct ApeTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file_) override {
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

		void WriteEmbeddedCover(File* file_, const TagLib::ByteVector& image_data) override {
			if (auto* ape_file = dynamic_cast<APE::File*>(file_)) {
				if (auto* tag = ape_file->APETag(true)) {
					WriteDefaultEmbeddedCover(tag, image_data);
				}
			}
		}

		void RemoveEmbeddedCover(File* file_) override {
			if (auto* ape_file = dynamic_cast<APE::File*>(file_)) {
				if (auto* tag = ape_file->APETag(false)) {
					RemoveDefaultEmbeddedCover(tag);
				}
			}
		}
	};

	HashMap<std::string_view, std::function<ScopedPtr<IFileTagWriter>()>>
		kFileTagWriterLut{
		{ ".flac", [] { return MakeAlign<IFileTagWriter, FlacTagWriter>(); } },
		{ ".mp3",  [] { return MakeAlign<IFileTagWriter, Mp3TagWriter>(); } },
		{ ".wav",  [] { return MakeAlign<IFileTagWriter, WavTagWriter>(); } },
		{ ".wave", [] { return MakeAlign<IFileTagWriter, WavTagWriter>(); } },
		{ ".m4a",  [] { return MakeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".mp4",  [] { return MakeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".opus",  [] { return MakeAlign<IFileTagWriter, OpusTagWriter>(); } },
		{ ".ape",  [] { return MakeAlign<IFileTagWriter, ApeTagWriter>(); } },
		{ ".dff",  [] { return MakeAlign<IFileTagWriter, DiffTagWriter>(); } },
		{ ".dsf",  [] { return MakeAlign<IFileTagWriter, DsfTagWriter>(); } }
	};

	ScopedPtr<IFileTagWriter> MakeFileTagWriter(const std::string &ext) {
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
			Save();
		}
		catch (const std::exception& e) {
			XAMP_LOG_DEBUG("Write tag failure: {}", e.what());
		}
	}

	void Open(const Path& path) {
		Save();
		Clear();

		transaction_.emplace(path);
		io_stream_.open(transaction_->TempPath());
		FileRef fileref(&io_stream_);
		if (fileref.isNull()) {
			XAMP_LOG_DEBUG("file was NULL!");
			Clear();
			return;
		}
		if (!fileref.tag()) {
			XAMP_LOG_DEBUG("tag is NULL!");
			Clear();
			return;
		}
		fileref_opt_ = fileref;
		path_ = path;
		const auto ext = String::ToLower(path_.extension().string());
		tag_writer_ = MakeFileTagWriter(ext);
	}

    void Write(const TrackInfo &track_info) const {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTrack(track_info.track);
		tag->setAlbum(track_info.album);
		tag->setArtist(track_info.artist);
		tag->setTitle(track_info.title);
		tag->setComment(track_info.comment);
		MarkDirty();
    }

    void WriteTitle(const std::wstring &title) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTitle(title.empty() ? TagLib::String() : title);
		MarkDirty();
    }

    void WriteArtist(const std::wstring &artist) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setArtist(artist.empty() ? TagLib::String() : artist);
		MarkDirty();
    }

    void WriteAlbum(const std::wstring &album) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setAlbum(album.empty() ? TagLib::String() : album);
		MarkDirty();
    }

    void WriteTrack(uint32_t track) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTrack(track);
		MarkDirty();
    }

	void WriteComment(const std::wstring& comment) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setComment(comment.empty() ? TagLib::String() : comment);
		MarkDirty();
	}

	void WriteGenre(const std::wstring& genre) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setGenre(genre.empty() ? TagLib::String() : genre);
		MarkDirty();
	}

	void WriteYear(uint32_t year) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setYear(year);
		MarkDirty();
	}

	void WriteReplayGain(const ReplayGain & replay_gain) {
		CheckFileRef()

		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->WriteReplayGain(replay_gain, file_);
			MarkDirty();
		}
	}

	void WriteEmbeddedCover(const uint8_t *image, size_t image_size) const {
		CheckFileRef()
		if (image_size > static_cast<size_t>((std::numeric_limits<unsigned int>::max)())) {
			throw PlatformException();
		}

		const TagLib::ByteVector image_data(reinterpret_cast<const char*>(image), static_cast<unsigned int>(image_size));
		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->WriteEmbeddedCover(file_, image_data);
			MarkDirty();
		}
	}

	void WriteEmbeddedCover(const std::vector<uint8_t> & image) const {
		CheckFileRef()
		WriteEmbeddedCover(image.data(), image.size());
    }

	void RemoveEmbeddedCover() {
		CheckFileRef()
		auto* file_ = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->RemoveEmbeddedCover(file_);
			MarkDirty();
		}
	}

	[[nodiscard]] bool CanWriteEmbeddedCover() const {
		const auto ext = String::ToLower(path_.extension().string());
        return kFileTagWriterLut.find(ext) != kFileTagWriterLut.end();
	}
private:
	void Save() {
		if (!fileref_opt_) {
			return;
		}

		if (!dirty_) {
			Clear();
			return;
		}
			
		if (!fileref_opt_->save()) {
			XAMP_LOG_DEBUG("Write tag failure!");
			Clear();
			return;
		}

		CloseWorkingFile();
		if (transaction_) {
			transaction_->Commit();
		}
		Clear();
    }

	void CloseWorkingFile() {
		fileref_opt_.reset();
		io_stream_.close();
	}

	void Clear() {
		fileref_opt_.reset();
		io_stream_.close();
		transaction_.reset();
		tag_writer_.reset();
		dirty_ = false;
	}

	void MarkDirty() const {
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
    : writer_(MakeAlign<TaglibMetadataWriterImpl>()) {
}

void TaglibMetadataWriter::Open(const Path& path) {
	return writer_->Open(path);
}

void TaglibMetadataWriter::WriteReplayGain(const ReplayGain& replay_gain) {
	return writer_->WriteReplayGain(replay_gain);
}

void TaglibMetadataWriter::Write(const TrackInfo& track_info) {
	writer_->Write(track_info);
}

void TaglibMetadataWriter::WriteTitle(const std::wstring & title) {
    writer_->WriteTitle(title);
}

void TaglibMetadataWriter::WriteArtist(const std::wstring& artist) {
	writer_->WriteArtist(artist);
}

void TaglibMetadataWriter::WriteTrack(uint32_t track) {
	writer_->WriteTrack(track);
}

void TaglibMetadataWriter::WriteComment(const std::wstring& comment) {
	writer_->WriteComment(comment);
}

void TaglibMetadataWriter::WriteGenre(const std::wstring& genre) {
	writer_->WriteGenre(genre);
}

void TaglibMetadataWriter::WriteYear(uint32_t year) {
	writer_->WriteYear(year);
}

void TaglibMetadataWriter::WriteAlbum(const std::wstring & album) {
    writer_->WriteAlbum(album);
}

void TaglibMetadataWriter::WriteEmbeddedCover(const std::vector<uint8_t> & image) const {
	writer_->WriteEmbeddedCover(image);
}

void TaglibMetadataWriter::RemoveEmbeddedCover() {
	writer_->RemoveEmbeddedCover();
}

bool TaglibMetadataWriter::CanWriteEmbeddedCover() const {
	return writer_->CanWriteEmbeddedCover();
}

XAMP_METADATA_NAMESPACE_END
