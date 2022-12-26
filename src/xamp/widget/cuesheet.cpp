#include <widget/widget_shared.h>
#include <base/str_utilts.h>
#include <base/bom.h>
#include <widget/cuesheet.h>

static uint32_t M2F(uint32_t mm, uint32_t ss, uint32_t ff) noexcept {
    return (mm * 60 + ss) * 75 + ff;
}

template <typename CharT>
struct CueTokenizer {
    typedef std::char_traits<CharT> traits_type;
    typedef typename traits_type::int_type int_type;

    explicit CueTokenizer(std::basic_streambuf<CharT>* sb)
        : lineno(0)
		, sb(sb) {
    }

    bool IsEmpty() const noexcept {
        return fields.empty();
    }

    bool IsSkip(int_type c) const noexcept {
	    return c == ' ' || c == '\t' || c == '\r';
    }

    size_t GetFieldSize() const {
        return fields.size();
    }

    std::wstring GetFirstField() const {
        return IsEmpty() ? L"" : fields[0];
    }

    bool Nextline() {
        fields.clear();
        int_type c;
        std::basic_string<CharT> field;

        while (traits_type::not_eof(c = sb->sbumpc())) {
            if (c == '"') {
                if (c == '\n')
                    throw std::runtime_error("");
                else if (c != '"')
                    field.push_back(c);
                else if (sb->sgetc() != '"')
                    break;
                else {
                    sb->snextc();
                    field.push_back(c);
                }
            }
            else if (c == '\n') {
                ++lineno;
                break;
            }
            else if (IsSkip(c)) {
                if (!field.empty()) {
                    fields.push_back(field);
                    field.clear();
                }
                while (IsSkip(sb->sgetc()))
                    sb->snextc();
            }
        }
        if (!field.empty()) {
            fields.push_back(field);
        }
        return !fields.empty() || c == '\n';
    }

private:
    size_t lineno;
    std::basic_streambuf<CharT>* sb;
    std::vector<std::basic_string<CharT>> fields;
};

void CueTrack::AddSegment(const CueSegment& seg) {
    if (!segments_.empty()) {
        auto& last = segments_.back();
        if (last.index >= seg.index) {
            if (last.index == std::numeric_limits<int32_t>::max()) {
                throw CueSheetException(String::Format(
                    "Conflicting use of "
                    "INDEX00 or PREGAP found on "
                    "track {}-{}", number_, number_ + 1));
            }

            throw CueSheetException(String::Format(
                "INDEX must be in strictly "
                "ascending order: track {}", number_ + 1));
        }
        if (last.filename == seg.filename && last.end == seg.begin) {
            last.end = seg.end;
            return;
        }
    }
    segments_.push_back(seg);
}

void CueSheet::Parse(std::wstreambuf* src) {
    struct ParseHandler {
        void (CueSheet::* handle)(const std::wstring* args);
        size_t num_args;
    };

    const OrderedMap<std::wstring, ParseHandler> handlers {
        { L"FILE", { &CueSheet::ParseFile, 3 }},
        { L"TRACK", { &CueSheet::ParseTrack, 3 }},
        { L"INDEX", { &CueSheet::ParseIndex, 3 }},
        { L"POSTGAP", { &CueSheet::ParsePostgap, 2 }},
        { L"PREGAP", { &CueSheet::ParsePregap, 2 }},
        { L"REM", { &CueSheet::ParseRem, 3 }},
        { L"CATALOG", { &CueSheet::ParseMeta, 2 }},
        { L"ISRC", { &CueSheet::ParseMeta, 2 }},
        { L"PERFORMER", { &CueSheet::ParseMeta, 2 }},
        { L"SONGWRITER", { &CueSheet::ParseMeta, 2 }},
        { L"TITLE", { &CueSheet::ParseMeta, 2 }},
    };

    CueTokenizer<wchar_t> tokenizer(src);
    while (tokenizer.Nextline()) {
        if (tokenizer.IsEmpty()) {
	        continue;
        }

        const auto cmd = tokenizer.GetFirstField();
        const auto itr = handlers.find(cmd);
        if (itr == handlers.end()) {
            continue;
        }

        if (tokenizer.GetFieldSize() == (*itr).second.num_args) {
	        const auto p = (*itr).second.handle;
            (this->*p)(&cmd);
        }
    }
}

void CueSheet::ParseFile(const std::wstring* args) {
    if (!file_name_.empty() && file_name_ != args[1]) {
        has_multiple_files_ = true;
    }
    file_name_ = args[1];
}

void CueSheet::ParseTrack(const std::wstring* args) {
    if (args[2] != CueTrack::kAudio) {
        return;
    }
    uint32_t no = 0;
    if (swscanf_s(args[1].c_str(), L"%d", &no) != 1) {
        throw CueSheetException("Invalid TRACK number");
    }
    tracks_.emplace_back(shared_from_this(), no);
}

uint32_t CueSheet::ParseIndexTimeFormat(const std::wstring* args) {
	uint32_t mm = 0, ss = 0, ff = 0;
	if (swscanf_s(args[2].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3) {
		throw CueSheetException("Invalid INDEX time format");
	}

	if (ss > 59 || ff > 74) {
		throw CueSheetException("Invalid INDEX time format");
	}
    uint32_t nframes = M2F(mm, ss, ff);
    return nframes;
}

void CueSheet::ParseIndex(const std::wstring* args) {
    if (tracks_.empty()) {
        throw CueSheetException("INDEX command before TRACK");
    }

    if (file_name_.empty()) {
        throw CueSheetException("INDEX command before FILE");
    }

    uint32_t no = 0;
    if (swscanf_s(args[1].c_str(), L"%u", &no) != 1) {
        throw CueSheetException("Invalid INDEX number");
    }

    const auto nframes = ParseIndexTimeFormat(args);
    auto * lastseg = LastSegment();
    if (lastseg && lastseg->filename == file_name_) {
        lastseg->end = nframes;
        if (lastseg->begin >= nframes) {
            throw CueSheetException("INDEX time must be in ascending order");
        }
    }

    CueSegment segment(no, file_name_);
    segment.begin = nframes;
    if (no > 0) {
        tracks_.back().AddSegment(segment);
        return;
    }

    if (tracks_.size() == 1) {
        tracks_.insert(tracks_.begin(), CueTrack(shared_from_this(), 0));
        tracks_[0].Set(L"title", L"(HTOA)");
        segment.index = 1;
    } else {
        segment.index = std::numeric_limits<int32_t>::max();
    }
    tracks_[tracks_.size() - 2].AddSegment(segment);
}

CueSegment* CueSheet::LastSegment() {
    CueSegment* seg = nullptr;
    for (int64_t i = tracks_.size() - 1; i >= 0; --i) {
        if ((seg = tracks_[i].LastSegment()) != nullptr) {
            break;
        }
    }
    return seg;
}

void CueSheet::ParsePostgap(const std::wstring* args) {
    if (tracks_.empty()) {
        throw CueSheetException("POSTGAP command before TRACK");
    }

    uint32_t mm = 0, ss = 0, ff = 0;
    if (swscanf_s(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3) {
        throw CueSheetException("Invalid POSTGAP time format");
    }

    CueSegment segment(CueTrack::kPostgapIndex, std::wstring(L"__GAP__"));
    segment.end = M2F(mm, ss, ff);
    tracks_.back().AddSegment(segment);
}

void CueSheet::ParsePregap(const std::wstring* args) {
    if (tracks_.empty()) {
        throw CueSheetException("PREGAP command before TRACK");
    }

    uint32_t mm = 0, ss = 0, ff = 0;
    if (swscanf_s(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3) {
        throw CueSheetException("Invalid PREGAP time format");
    }

    CueSegment segment(CueTrack::kPregapIndex, std::wstring(L"__GAP__"));
    segment.end = M2F(mm, ss, ff);
    if (tracks_.size() > 1) {
        tracks_[tracks_.size() - 2].AddSegment(segment);
    }
}

void CueSheet::ParseMeta(const std::wstring* args) {
    if (!tracks_.empty()) {
        tracks_.back().Set(args[0], args[1]);
    } else {
        metadata_[args[0]] = args[1];
    }
}
