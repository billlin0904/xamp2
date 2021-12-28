//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

class ArtistView;
class AlbumView;

class AlbumArtistPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumArtistPage(QWidget* parent = nullptr);

	ArtistView* artist() const {
		return artist_view_;
	}

	AlbumView* album() const {
		return album_view_;
	}

public slots:
	void onThemeChanged(QColor backgroundColor, QColor color);

	void refreshOnece();

private:
	ArtistView* artist_view_;
	AlbumView* album_view_;
};

