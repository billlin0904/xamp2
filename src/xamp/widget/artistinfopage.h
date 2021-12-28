//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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
    void onThemeChanged(QColor backgroundColor, QColor color);

	void setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id);

	void setAlbumCount(int32_t album_count);

	void setTracks(int32_t tracks);

	void setTotalDuration(double durations);

private:
	QPixmap getArtistImage(QPixmap const* cover) const;
	
	QLabel* cover_;
	QLabel* artist_;
	QLabel* tracks_;
	QLabel* albums_;
	QLabel* durtions_;
	QString cover_id_;
	int32_t artist_id_;
	AlbumView* album_view_;
};


