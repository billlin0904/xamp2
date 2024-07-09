#include <metadata/taglib.h>
#include <metadata/taglibmetawriter.h>

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
	
}

class TaglibMetadataWriter::TaglibMetadataWriterImpl {
public:
	TaglibMetadataWriterImpl() = default;

    void Write(const Path &path, const TrackInfo &track_info) const {
        Write(path, [&ti = std::as_const(track_info)](auto, auto tag) {
			tag->setTrack(ti.track);

			if (ti.album) {
				tag->setAlbum(ti.album.value());
			} else {
				tag->setAlbum("");
			}

			if (ti.artist) {
				tag->setArtist(ti.artist.value());
			} else {
				tag->setArtist("");
			}

			if (ti.title) {
				tag->setTitle(ti.title.value());
			} else {
				tag->setTitle("");
			}

			if (ti.comment) {
				tag->setComment(ti.comment.value());
			} else {
				tag->setComment("");
			}
		});
    }

    void WriteTitle(const Path & path, const std::wstring &title) {
        Write(path, [&title = std::as_const(title)](auto, auto tag) {
	        tag->setTitle(title.empty() ? TagLib::String() : title);
	    });
    }

    void WriteArtist(const Path & path, const std::wstring &artist) {
        Write(path, [&artist = std::as_const(artist)](auto, auto tag) {
	        tag->setArtist(artist.empty() ? TagLib::String() : artist);
	    });
    }

    void WriteAlbum(const Path & path, const std::wstring &album) {
        Write(path, [&album = std::as_const(album)](auto, auto tag) {
	        tag->setAlbum(album.empty() ? TagLib::String() : album);
	    });
    }

    void WriteTrack(const Path & path, uint32_t track) {
        Write(path, [track](auto, auto tag) {
	        tag->setTrack(track);
	    });
    }

	void WriteComment(const Path& path, const std::wstring& comment) {
		Write(path, [&comment = std::as_const(comment)](auto, auto tag) {
			tag->setComment(comment.empty() ? TagLib::String() : comment);
			});
	}

	void WriteGenre(const Path& path, const std::wstring& genre) {
		Write(path, [&genre = std::as_const(genre)](auto, auto tag) {
			tag->setGenre(genre.empty() ? TagLib::String() : genre);
			});
	}

	void WriteYear(const Path& path, uint32_t year) {
		Write(path, [year](auto, auto tag) {
			tag->setYear(year);
			});
	}

	void WriteReplayGain(Path const& path, const ReplayGain & replay_gain) {
		const auto ext = String::ToLower(path.extension().string());

		if (ext == ".flac") {
            Write(path, [&replay_gain = std::as_const(replay_gain)](auto* file, Tag* tag) {
				if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
					Ogg::XiphComment* comment = nullptr;
					if (!flac_file->hasXiphComment()){
						comment = flac_file->xiphComment(true);
					} else {
						comment = flac_file->xiphComment(false);
					}
					comment->addField(kReplaygainAlbumGain, std::to_string(replay_gain.album_gain));
					comment->addField(kReplaygainTrackGain, std::to_string(replay_gain.track_gain));
					comment->addField(kReplaygainAlbumPeak, std::to_string(replay_gain.album_peak));
					comment->addField(kReplaygainTrackPeak, std::to_string(replay_gain.track_peak));
					comment->addField(kReplaygainReferenceLoudness, std::to_string(replay_gain.ref_loudness));
				}
				});
		}
		else if (ext == ".mp3") {
            Write(path, [&replay_gain = std::as_const(replay_gain)](auto* file, Tag* tag) {
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
				});
		}
		else if (ext == ".m4a" || ext == ".mp4") {
            Write(path, [&replay_gain = std::as_const(replay_gain)](auto* file, Tag* tag) {
				if (auto* mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(tag)) {
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
				});
		}
	}

	void WriteEmbeddedCover(const Path& path, const uint8_t *image, size_t image_size) const {
		const auto ext = String::ToLower(path.extension().string());
		const TagLib::ByteVector image_data(reinterpret_cast<const char*>(image), image_size);

		if (ext == ".m4a" || ext == ".mp4") {
			TagLib::MP4::CoverArt cover_art(static_cast<TagLib::MP4::CoverArt::Format>(0x0D), image_data);

			Write(path, [&cover_art = std::as_const(cover_art)](auto, auto tag) {
				if (auto* mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(tag)) {
					TagLib::MP4::CoverArtList cover_art_list;
					cover_art_list.append(cover_art);
					const TagLib::MP4::Item cover_item(cover_art_list);
					mp4_tag->setItem("covr", cover_item);
				}
				});
		}
		else if (ext == ".mp3") {
			Write(path, [&id = std::as_const(image_data)](auto file, auto) {
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
						frame->setPicture(id);
						mp3_tag->addFrame(frame);
					}
				}
				});
		}
		else if (ext == ".flac") {
			Write(path, [&id = std::as_const(image_data)](auto file, auto) {
				if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
					flac_file->removePictures();
					auto* picture = new TagLib::FLAC::Picture();
					picture->setMimeType("image/jpeg");
					picture->setData(id);
					picture->setType(TagLib::FLAC::Picture::FrontCover);
					flac_file->addPicture(picture);
				}
				});
		}
	}

	void WriteEmbeddedCover(const Path & path, const Vector<uint8_t> & image) const {		
		WriteEmbeddedCover(path, image.data(), image.size());
    }

	void RemoveEmbeddedCover(const Path& path) {
		const auto ext = String::ToLower(path.extension().string());

		if (ext == ".m4a" || ext == ".mp4") {
			Write(path, [](auto file, auto) {
				if (auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
					auto cover_art_list = mp4_file->tag()->item("covr").toCoverArtList();
					if (!cover_art_list.isEmpty()) {
						mp4_file->tag()->removeItem("covr");
					}
				}
				});
		}
		else if (ext == ".flac") {
			Write(path, [](auto file, auto) {
				if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
					flac_file->removePictures();
				}
				});
		}
		else if (ext == ".mp3") {
			Write(path, [](auto file, auto) {
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
				});
		}
	}

	XAMP_NO_DISCARD bool CanWriteEmbeddedCover(const Path& path) const {
		const auto ext = String::ToLower(path.extension().string());
		static const HashSet<std::string> support_ext{
			".m4a",
			".mp4",
			".mp3",
			".flac",
		};
        return support_ext.find(ext) != support_ext.end();
	}
private:
    template <typename Function>
    static void Write(const Path & path, Function &&fun) {
#ifdef XAMP_OS_WIN
        FileRef fileref(path.wstring().c_str());
#else
        FileRef fileref(path.string().c_str());
#endif
		if (fileref.isNull()) {
			XAMP_LOG_DEBUG("file was NULL!");
			return;
		}
		if (!fileref.tag()) {
			XAMP_LOG_DEBUG("tag is NULL!");
			return;
		}
        fun(fileref.file(), fileref.tag());
        if (!fileref.save()) {
			XAMP_LOG_DEBUG("Write tag failure!");
        }
    }
};

XAMP_PIMPL_IMPL(TaglibMetadataWriter)

TaglibMetadataWriter::TaglibMetadataWriter()
    : writer_(MakeAlign<TaglibMetadataWriterImpl>()) {
}

void TaglibMetadataWriter::WriteReplayGain(const Path & path, const ReplayGain& replay_gain) {
	return writer_->WriteReplayGain(path, replay_gain);
}

void TaglibMetadataWriter::Write(const Path& path, const TrackInfo& track_info) {
	writer_->Write(path, track_info);
}

void TaglibMetadataWriter::WriteTitle(const Path & path, const std::wstring & title) {
    writer_->WriteTitle(path, title);
}

void TaglibMetadataWriter::WriteArtist(const Path& path, const std::wstring& artist) {
	writer_->WriteArtist(path, artist);
}

void TaglibMetadataWriter::WriteTrack(const Path& path, uint32_t track) {
	writer_->WriteTrack(path, track);
}

void TaglibMetadataWriter::WriteComment(const Path& path, const std::wstring& comment) {
	writer_->WriteComment(path, comment);
}

void TaglibMetadataWriter::WriteGenre(const Path& path, const std::wstring& genre) {
	writer_->WriteGenre(path, genre);
}

void TaglibMetadataWriter::WriteYear(const Path& path, uint32_t year) {
	writer_->WriteYear(path, year);
}

void TaglibMetadataWriter::WriteAlbum(const Path & path, const std::wstring & album) {
    writer_->WriteAlbum(path, album);
}

void TaglibMetadataWriter::WriteEmbeddedCover(const Path & path, const Vector<uint8_t> & image) const {
	writer_->WriteEmbeddedCover(path, image);
}

void TaglibMetadataWriter::RemoveEmbeddedCover(const Path& path) {
	writer_->RemoveEmbeddedCover(path);
}

bool TaglibMetadataWriter::CanWriteEmbeddedCover(const Path& path) const {
	return writer_->CanWriteEmbeddedCover(path);
}

XAMP_METADATA_NAMESPACE_END
