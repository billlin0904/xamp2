//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>

#include <QCoroTask>
#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QString>

#include <widget/httpx.h>
#include <widget/ilrrcparser.h>
#include <widget/krcparser.h>
#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>

class QNetworkAccessManager;
class QObject;

struct XAMP_WIDGET_SHARED_API LyricsParser {
    Candidate candidate;
    QByteArray content;
    QSharedPointer<ILrcParser> parser;
};

struct XAMP_WIDGET_SHARED_API SearchLyricsResult {
    InfoItem info;
    QString source_name;
    QString request_title;
    QString request_artist;
    QList<LyricsParser> parsers;
};

class XAMP_WIDGET_SHARED_API ILyricsSource {
public:
    virtual ~ILyricsSource() = default;

    virtual QString name() const = 0;

    virtual QCoro::Task<QList<Candidate>> search(const PlayListEntity& keyword) = 0;

    virtual QCoro::Task<SearchLyricsResult> lookup(const Candidate& candidate) = 0;
};

XAMP_WIDGET_SHARED_API std::unique_ptr<ILyricsSource> makeNeteaseLyricsSource(
    QNetworkAccessManager* nam,
    QObject* parent = nullptr);

XAMP_WIDGET_SHARED_API std::unique_ptr<ILyricsSource> makeQQMusicLyricsSource(
    QNetworkAccessManager* nam,
    QObject* parent = nullptr);

XAMP_WIDGET_SHARED_API std::unique_ptr<ILyricsSource> makeKugouLyricsSource(
    QNetworkAccessManager* nam,
    QObject* parent = nullptr);

Q_DECLARE_METATYPE(LyricsParser)
Q_DECLARE_METATYPE(SearchLyricsResult)
