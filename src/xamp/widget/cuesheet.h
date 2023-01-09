//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <streambuf>
#include <istream>
#include <string>
#include <utility>
#include <vector>
#include <map>

struct CueSegment {
    CueSegment(uint32_t index, const std::wstring& filename)
        : index(index)
		, begin(0)
		, end(~0U)
		, filename(filename) {
    }

    uint32_t index;
    uint32_t begin;
    uint32_t end;
    std::wstring filename;
};

class CueSheet;

class CueTrack {
public:
    typedef std::vector<CueSegment>::iterator iterator;

    static constexpr std::wstring_view kTitle = L"TITLE";
    static constexpr std::wstring_view kAudio = L"AUDIO";
    static constexpr int32_t kPregapIndex = 0x7fffffff;
    static constexpr int32_t kPostgapIndex = 0x7ffffffe;

    CueTrack(std::shared_ptr<CueSheet> cuesheet, uint32_t number)
        : number_(number)
		, cuesheet_(cuesheet) {
    }

    std::wstring name() const {
	    const auto itr = metadata_.find(L"TITLE");
        return itr != metadata_.end() ? (*itr).second : L"";
    }

    void Set(const std::wstring& key, const std::wstring& value) {
        metadata_[key] = value;
    }

    iterator begin() {
	    return segments_.begin();
    }

    iterator end() {
	    return segments_.end();
    }

    CueSegment* LastSegment() {
        return !segments_.empty() ? &segments_.back() : nullptr;
    }

    void AddSegment(const CueSegment& seg);
private:
    uint32_t number_;
    std::weak_ptr<CueSheet> cuesheet_;
    OrderedMap<std::wstring, std::wstring> metadata_;
    std::vector<CueSegment> segments_;
};

class CueSheetException : public std::exception {
public:
	explicit CueSheetException(std::string what)
	    : what_(std::move(what)) {	    
    }
private:
    std::string what_;
};

class CueSheet : public std::enable_shared_from_this<CueSheet> {
public:
    CueSheet()
	    : has_multiple_files_(false)
		, lineno_(0) {
    }

    void Parse(std::wstreambuf* src);

private:
    CueSegment* LastSegment();

    uint32_t ParseIndexTimeFormat(const std::wstring* args);

    void ParseFile(const std::wstring* args);
    void ParseTrack(const std::wstring* args);
    void ParseIndex(const std::wstring* args);
    void ParsePostgap(const std::wstring* args);
    void ParsePregap(const std::wstring* args);
    void ParseMeta(const std::wstring* args);

    void ParseRem(const std::wstring* args) {
	    ParseMeta(args + 1);
    }

    bool has_multiple_files_;
    size_t lineno_;
    std::wstring file_name_;
    std::vector<CueTrack> tracks_;
    OrderedMap<std::wstring, std::wstring> metadata_;
};