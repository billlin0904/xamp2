//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMultiMap>
#include <QObject>
#include <QUrl>
#include <QImage>

#include <widget/cover_searcher/coversearcher.h>

class MusicbrainzCoverSearcher : public ICoverSearcher {
	Q_OBJECT
public:
	explicit MusicbrainzCoverSearcher(QObject* parent = nullptr);

	void Search(const QString& artist, const QString& album, int id) override;

signals:
	void SearchFinished(int32_t id, const QPixmap &cover);

private:
	void FetchFinished(int id, qsizetype total_size, const QImage& image);

	QMultiMap<int, QUrl> image_checks_;
	QMultiMap<int, QImage> image_list_;
	QList<QUrl> download_url_list_;
};
