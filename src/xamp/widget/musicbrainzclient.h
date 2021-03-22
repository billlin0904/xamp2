//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QString>

#include <widget/http.h>

struct FingerprintInfo {
	int32_t artist_id{ 0 };
	int32_t duration{ 0 };
	QString fingerprint;
};

class MusicBrainzClient : public QObject {
	Q_OBJECT
public:
	explicit MusicBrainzClient(QObject* parent = nullptr);

	void searchBy(const FingerprintInfo& fingerprint);

	void lookupArtist(int32_t artist_id, const QString& artist_mbid);

signals:
	void finished(int32_t artist_id, const QString & discogs_artist_id);	
};
