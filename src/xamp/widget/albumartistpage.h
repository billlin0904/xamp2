//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QListView>
#include <QStandardItemModel>

#include <widget/themecolor.h>

class AlbumView;
class ArtistInfoPage;

class AlbumTabListView : public QListView {
	Q_OBJECT
public:
	explicit AlbumTabListView(QWidget* parent = nullptr);

	void AddTab(const QString& name, int tab_id);

	void SetCurrentTab(int tab_id);
signals:
	void ClickedTable(int table_id);

	void TableNameChanged(int table_id, const QString& name);

private:
	QStandardItemModel model_;
};

class AlbumArtistPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumArtistPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

public slots:
	void OnThemeChanged(QColor backgroundColor, QColor color);

	void Refresh();

	void SetArtistId(const QString& artist, const QString& cover_id, int32_t artist_id);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

private:
	AlbumTabListView* list_view_;
	AlbumView* album_view_;
	ArtistInfoPage* artist_view_;
};

