//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>

#include <QObject>
#include <QList>
#include <QString>
#include <QFutureWatcher>

#include <widget/http.h>

struct Fingerprint {
	int32_t artist_id{ 0 };
	int32_t duration{ 0 };
	QString fingerprint;
};

class MusicBrainzClient : public QObject {
	Q_OBJECT
public:
	MusicBrainzClient(QObject* parent = nullptr);

	void readFingerprint(int32_t artist_id, const QString &file_path);

	void cancel();

	void lookupArtist(int32_t artist_id, const QString& artist_mbid);

signals:
	void finished(int32_t artist_id, const QString & discogs_artist_id);

public slots:
	void fingerprintFound(int index);

private:	
	QFutureWatcher<Fingerprint>* fingerprint_watcher_;
	QList<QString> file_paths_;	
};
