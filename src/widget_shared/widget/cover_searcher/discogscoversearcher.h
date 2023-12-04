//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMultiMap>
#include <QObject>
#include <QUrl>
#include <QImage>

#include <widget/cover_searcher/coversearcher.h>

class DiscogsCoverSearcher : public ICoverSearcher {
	Q_OBJECT
public:
	explicit DiscogsCoverSearcher(QObject* parent = nullptr);

	void Search(const QString& artist, const QString& album, int id) override;

signals:
	void SearchFinished(int32_t id, const QPixmap& cover);

private:
	void RequestRelease(int32_t id, const QString & resource_url);

	QMap<int, QString> requests_search_;
};
