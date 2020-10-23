//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

#include <widget/widget_shared.h>

class AlbumView;
class QLabel;

class ArtistInfoPage final : public QFrame {
	Q_OBJECT
public:
	explicit ArtistInfoPage(QWidget* parent = nullptr);

public slots:
    void OnThemeColorChanged(QColor backgroundColor, QColor color);

	void setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id);

private:
	QLabel* cover_;
	QLabel* artist_;
	AlbumView* album_view_;
};


