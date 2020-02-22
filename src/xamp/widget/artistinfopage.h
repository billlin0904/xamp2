//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QLabel>

#include <widget/albumview.h>

class ArtistInfoPage : public QFrame {
	Q_OBJECT
public:
	explicit ArtistInfoPage(QWidget* parent = nullptr);

	void setArtistId(const QString &cover_id, int32_t artist_id);

public slots:
	void onTextColorChanged(QColor backgroundColor, QColor color);

private:
	QLabel* cover_;
	QLabel* artist_;
	AlbumView* album_view_;
};


