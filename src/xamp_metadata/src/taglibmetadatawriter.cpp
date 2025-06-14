#include <metadata/taglib.h>
#include <metadata/taglibmetawriter.h>

#include <base/exception.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platfrom_handle.h>
#include <base/str_utilts.h>

XAMP_DECLARE_LOG_NAME(TagLib);

namespace {
	class TagLibDebugListener : public TagLib::DebugListener {
	public:
		void printMessage(const TagLib::String& msg) override {
			using namespace xamp::base;
			static auto logger = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(TagLib));
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
	class TabLibIOStream : public TagLib::IOStream {
	public:
		TabLibIOStream() = default;

		explicit TabLibIOStream(const Path& path)
			: io_stream_(path, FastIOStream::Mode::ReadWriteOnlyExisting) {
		}

		void open(const Path& path) {
			io_stream_.open(path, FastIOStream::Mode::ReadWriteOnlyExisting);
		}

		TagLib::FileName name() const override {
			return io_stream_.path().wstring().c_str();
		}

		TagLib::ByteVector readBlock(size_t len) override {
			if (!isOpen() || len == 0)
				return {};

			TagLib::ByteVector v;
			v.resize(static_cast<uint32_t>(len));

			size_t n = io_stream_.read(v.data(), len);
			v.resize(static_cast<uint32_t>(n));
			return v;
		}

		void writeBlock(const TagLib::ByteVector& v) override {
			if (v.isEmpty() || io_stream_.read_only())
				return;

			size_t total = 0;
			const char* data = v.data();
			const size_t len = v.size();

			while (total < len) {
				size_t w = io_stream_.write(data + total, len - total);
				if (w == 0) {
					throw PlatformException();
				}
				total += w;
			}
		}
		
		void insert(const TagLib::ByteVector& data, offset_t start = 0, size_t replace = 0) override {
			if (io_stream_.read_only())
				return;

			const auto total = length();
			const uint64_t insert_len = data.size();
			const uint64_t remove_len = replace;

			static constexpr size_t BUF = 64 * 1024;
			std::vector<char> buf(BUF);

			uint64_t read_pos = total;
			uint64_t write_pos = total + insert_len - remove_len;

			while (read_pos > start + remove_len) {
				size_t chunk = std::min<uint64_t>(BUF, read_pos - (start + remove_len));
				read_pos -= chunk;
				write_pos -= chunk;

				io_stream_.seek(read_pos, Beginning);
				size_t n = io_stream_.read(buf.data(), chunk);
				io_stream_.seek(write_pos, Beginning);
				io_stream_.write(buf.data(), n);
			}

			// 2) 寫入新資料
			seek(start, Beginning);
			writeBlock(data);

			// 3) 如有 replace > insert_len，砍掉多餘
			if (insert_len < remove_len) {
				truncate(total + insert_len - remove_len);
			}
		}

		void removeBlock(offset_t start = 0, size_t len = 0) override {
			if (io_stream_.read_only() || len == 0) 
				return;

			const auto total = length();
			if (start + len >= total) {
				truncate(start);
				return;
			}

			static constexpr size_t BUF = 64 * 1024;
			std::vector<char> buf(BUF);

			uint64_t read_pos = start + len;
			uint64_t write_pos = start;

			while (read_pos < total) {
				size_t chunk = std::min<uint64_t>(BUF, total - read_pos);
				io_stream_.seek(read_pos, Beginning);
				size_t n = io_stream_.read(buf.data(), chunk);
				io_stream_.seek(write_pos, Beginning);
				io_stream_.write(buf.data(), n);
				read_pos += n;
				write_pos += n;
			}
			truncate(write_pos);
		}

		bool readOnly() const override { 
			return io_stream_.read_only();
		}

		bool isOpen() const override {
			return io_stream_.is_open();
		}

		long long length() override {
			return static_cast<long long>(io_stream_.size());
		}

		void seek(long long offset, Position p = Beginning) override {
			int whence = (p == Beginning)
				? SEEK_SET
				: (p == Current) ? SEEK_CUR
				: SEEK_END;
			io_stream_.seek(offset, whence);
		}

		long long tell() const override {
			return static_cast<long long>(io_stream_.tell());
		}

		void truncate(long long l) override	{
			if (!io_stream_.read_only())
				io_stream_.truncate(static_cast<uint64_t>(l));
		}

		FastIOStream io_stream_;
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

	struct XAMP_NO_VTABLE IFileTagWriter {
		virtual ~IFileTagWriter() = default;
		virtual void WriteReplayGain(const ReplayGain& replay_gain, File* file) = 0;
		virtual void WriteEmbeddedCover(File* file, const TagLib::ByteVector& image_data) = 0;
		virtual void RemoveEmbeddedCover(File* file) = 0;
	};

	struct FlacTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file) override {
			if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
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

		void WriteEmbeddedCover(File* file, const TagLib::ByteVector& image_data) override {
			if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
				flac_file->removePictures();
				auto* picture = new TagLib::FLAC::Picture();
				picture->setMimeType("image/jpeg");
				picture->setData(image_data);
				picture->setType(TagLib::FLAC::Picture::FrontCover);
				flac_file->addPicture(picture);
			}
		}

		void RemoveEmbeddedCover(File* file) override {
			if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
				flac_file->removePictures();
			}
		}
	};

	struct Mp3TagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
				if (auto* mp3_tag = mp3_file->ID3v2Tag(true)) {
					auto version = mp3_tag->header()->majorVersion();
					while (ClearTxxTag(mp3_tag, kReplaygainAlbumGain)) {}
					while (ClearTxxTag(mp3_tag, kReplaygainTrackGain)) {}
					while (ClearTxxTag(mp3_tag, kReplaygainAlbumPeak)) {}
					while (ClearTxxTag(mp3_tag, kReplaygainTrackPeak)) {}
					while (ClearTxxTag(mp3_tag, kReplaygainReferenceLoudness)) {}
					SetTxxTag(mp3_tag, kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
					SetTxxTag(mp3_tag, kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
					SetTxxTag(mp3_tag, kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
					SetTxxTag(mp3_tag, kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
					SetTxxTag(mp3_tag, kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
				}
			}
		}

		void WriteEmbeddedCover(File* file, const TagLib::ByteVector& image_data) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
				if (auto* mp3_tag = mp3_file->ID3v2Tag(true)) {
					const auto frame_list = mp3_tag->frameList("APIC");
					for (auto* it : frame_list) {
						auto* frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(it);
						if (frame != nullptr) {
							mp3_tag->removeFrame(frame, true);
						}
					}
					auto* frame = new TagLib::ID3v2::AttachedPictureFrame("APIC");
					frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
					frame->setMimeType("image/jpeg");
					frame->setPicture(image_data);
					mp3_tag->addFrame(frame);
				}
			}
		}

		void RemoveEmbeddedCover(File* file) override {
			if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
				if (auto* mp3_tag = mp3_file->ID3v2Tag(true)) {
					const auto frame_list = mp3_tag->frameList("APIC");
					for (auto* it : frame_list) {
						auto* frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(it);
						if (frame != nullptr) {
							mp3_tag->removeFrame(frame, true);
						}
					}
				}
			}
		}
	};

	struct Mp4TagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file) override {
			if (auto* mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(file->tag())) {
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

		void WriteEmbeddedCover(File* file, const TagLib::ByteVector& image_data) override {
			TagLib::MP4::CoverArt cover_art(static_cast<TagLib::MP4::CoverArt::Format>(0x0D), image_data);

			if (auto* mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(file->tag())) {
				TagLib::MP4::CoverArtList cover_art_list;
				cover_art_list.append(cover_art);
				const TagLib::MP4::Item cover_item(cover_art_list);
				mp4_tag->setItem("covr", cover_item);
			}
		}

		void RemoveEmbeddedCover(File* file) override {
			if (auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
				auto cover_art_list = mp4_file->tag()->item("covr").toCoverArtList();
				if (!cover_art_list.isEmpty()) {
					mp4_file->tag()->removeItem("covr");
				}
			}
		}
	};

	struct OpusTagWriter : public IFileTagWriter {
		void WriteReplayGain(const ReplayGain& replay_gain, File* file) override {
			auto* opus = dynamic_cast<Ogg::Opus::File*>(file);
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

		void WriteEmbeddedCover(File* file, const TagLib::ByteVector& image_data) override {
			auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file);
			if (!opus_file || !opus_file->isValid()) {
				return;
			}

			auto* tag = opus_file->tag();
			if (!tag) {				
				return;
			}

			tag->removeFields("METADATA_BLOCK_PICTURE");

			TagLib::FLAC::Picture picture;
			picture.setMimeType("image/jpeg");
			picture.setType(TagLib::FLAC::Picture::FrontCover);
			picture.setData(image_data);

			const auto rendered = picture.render();
			tag->addField("METADATA_BLOCK_PICTURE", rendered.toBase64());
		}

		void RemoveEmbeddedCover(File* file) override {
			if (auto* const opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file)) {
				auto* tag = opus_file->tag();
				tag->removeFields("METADATA_BLOCK_PICTURE");
			}
		}
	};

	const HashMap<std::string_view, std::function<ScopedPtr<IFileTagWriter>()>>
		kFileTagWriterLut{
		{ ".flac", [] { return MakeAlign<IFileTagWriter, FlacTagWriter>(); } },
		{ ".mp3",  [] { return MakeAlign<IFileTagWriter, Mp3TagWriter>(); } },
		{ ".m4a",  [] { return MakeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".mp4",  [] { return MakeAlign<IFileTagWriter, Mp4TagWriter>(); } },
		{ ".opus",  [] { return MakeAlign<IFileTagWriter, OpusTagWriter>(); } }
	};

	ScopedPtr<IFileTagWriter> MakeFileTagWriter(const std::string &ext) {
		const auto itr = kFileTagWriterLut.find(ext);
		if (itr != kFileTagWriterLut.end()) {
			return itr->second();
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
		Save();
	}

	void Open(const Path& path) {
		io_stream_.open(path);
		FileRef fileref(&io_stream_);
		//FileRef fileref(path.native().c_str());
		if (fileref.isNull()) {
			XAMP_LOG_DEBUG("file was NULL!");
			return;
		}
		if (!fileref.tag()) {
			XAMP_LOG_DEBUG("tag is NULL!");
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
    }

    void WriteTitle(const std::wstring &title) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTitle(title.empty() ? TagLib::String() : title);
    }

    void WriteArtist(const std::wstring &artist) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setArtist(artist.empty() ? TagLib::String() : artist);
    }

    void WriteAlbum(const std::wstring &album) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setAlbum(album.empty() ? TagLib::String() : album);
    }

    void WriteTrack(uint32_t track) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setTrack(track);
    }

	void WriteComment(const std::wstring& comment) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setComment(comment.empty() ? TagLib::String() : comment);
	}

	void WriteGenre(const std::wstring& genre) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setGenre(genre.empty() ? TagLib::String() : genre);
	}

	void WriteYear(uint32_t year) {
		CheckFileRef()
		auto* tag = fileref_opt_->tag();
		tag->setYear(year);
	}

	void WriteReplayGain(const ReplayGain & replay_gain) {
		CheckFileRef()

		auto* file = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->WriteReplayGain(replay_gain, file);
		}
	}

	void WriteEmbeddedCover(const uint8_t *image, size_t image_size) const {
		CheckFileRef()
		const auto ext = String::ToLower(path_.extension().string());		

		const TagLib::ByteVector image_data(reinterpret_cast<const char*>(image), image_size);
		auto* file = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->WriteEmbeddedCover(file, image_data);
		}
	}

	void WriteEmbeddedCover(const std::vector<uint8_t> & image) const {
		CheckFileRef()
		WriteEmbeddedCover(image.data(), image.size());
    }

	void RemoveEmbeddedCover() {
		CheckFileRef()
		auto* file = fileref_opt_->file();
		if (tag_writer_ != nullptr) {
			tag_writer_->RemoveEmbeddedCover(file);
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
			
		if (!fileref_opt_->save()) {
			XAMP_LOG_DEBUG("Write tag failure!");
		}
    }

	Path path_;
	std::optional<FileRef> fileref_opt_;
	ScopedPtr<IFileTagWriter> tag_writer_;
	TabLibIOStream io_stream_;
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
