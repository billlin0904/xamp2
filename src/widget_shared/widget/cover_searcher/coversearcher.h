//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMultiMap>
#include <QObject>
#include <QUrl>
#include <QImage>

class ICoverSearcher : public QObject {
	Q_OBJECT
public:
	virtual void Search(const QString& artist, const QString& album, int id) = 0;

protected:
	explicit ICoverSearcher(QObject* parent = nullptr)
		: QObject(parent) {
	}
};

