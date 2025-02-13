//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/base.h>
#include <widget/util/str_util.h>
#include <widget/ilrrcparser.h>

struct XAMP_WIDGET_SHARED_EXPORT KrcContent {
    QString base64Content;
    QByteArray decodedContent;
};

struct XAMP_WIDGET_SHARED_EXPORT Candidate {
    QString singer;
    QString song;
    QString id;
    QString accesskey;
};

struct XAMP_WIDGET_SHARED_EXPORT InfoItem {
    int duration = 0;
    // Title
    QString songname;
    // Singer
    QString singername;
    // Album
    QString albumName;
    QString hash;
    QString sqhash;
};

XAMP_WIDGET_SHARED_EXPORT QList<InfoItem> parseInfoData(const QString& jsonString);

XAMP_WIDGET_SHARED_EXPORT QList<Candidate> parseCandidatesFromJson(const QString& jsonString);

XAMP_WIDGET_SHARED_EXPORT std::optional<KrcContent> parseKrcContent(const QString& jsonString);

class XAMP_WIDGET_SHARED_EXPORT KrcParser : public ILrcParser {
public:
    KrcParser();

    XAMP_DISABLE_COPY(KrcParser)

	bool parse(std::wistream& istr) override;

    bool parseFile(const std::wstring& file_path) override;

    bool parse(const uint8_t* buffer, size_t size);

    LyricEntry lineAt(int32_t index) const override;

    int32_t getSize() const override;

    void clear() override;

    void addLrc(const LyricEntry& lrc) override;

    const LyricEntry& getLyrics(const std::chrono::milliseconds& time) const noexcept override;

    std::vector<LyricEntry>::iterator end() override;

    std::vector<LyricEntry>::iterator begin() override;

    std::vector<LyricEntry>::const_iterator cend() const override;

    std::vector<LyricEntry>::const_iterator cbegin() const override;

	bool hasTranslation() const override;
private:
    bool parseKrcText(const std::wstring& wtext);
	bool has_trans_lrc_ = false;
    std::vector<LyricEntry> lyrics_;
};
