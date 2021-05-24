//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

#include <widget/http.h>
#include <widget/str_utilts.h>

class MusixmatchClient final : public QObject {
	Q_OBJECT
public:
	explicit MusixmatchClient(QNetworkAccessManager* manager = nullptr, QObject* parent = nullptr);

	QString getUrl(QString const &url) const;

	void matcherLyrics(QString const & q_track,
		QString const& q_artist,
		QString format = Q_UTF8("json"));

	void chartArtists(QString const &page,
		uint32_t page_size,
		QString country = Q_UTF8("us"),
		QString format = Q_UTF8("json"));

	void chartTrack(QString const& page,
		uint32_t page_size,
		bool f_has_lyrics,
		QString country = Q_UTF8("us"),
		QString format = Q_UTF8("json"));

	void trackSearch(QString const & q_track,
		QString const & q_artist,
		uint32_t page_size,
		QString const& page, 
		uint32_t s_track_rating,
		QString format = Q_UTF8("json"));

	void track(QString const & track_id,
		QString const& commontrack_id = Qt::EmptyString,
		QString const& track_isrc = Qt::EmptyString,
		QString const& track_mbid = Qt::EmptyString,
		QString format = Q_UTF8("json"));

	void trackLyrics(QString const& track_id,
		QString const& commontrack_id = Qt::EmptyString,
		QString format = Q_UTF8("json"));

	void trackSnippet(QString const& track_id,
		QString format = Q_UTF8("json"));

	void trackSubtitle(QString const& track_id,
		QString const& track_mbid,
		QString const& subtitle_format = Qt::EmptyString,
		QString const& f_subtitle_length = Qt::EmptyString,
		QString const& f_subtitle_length_max_deviation = Qt::EmptyString,
		QString format = Q_UTF8("json"));
	
private:
	QString api_key_;
	QNetworkAccessManager* manager_;
};
