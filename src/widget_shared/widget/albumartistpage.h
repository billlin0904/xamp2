//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <widget/widget_shared_global.h>
#include <widget/themecolor.h>

class AlbumView;
class ArtistView;
class GenreView;
class ArtistInfoPage;
class QPushButton;
class QLabel;
class QComboBox;
class QPropertyAnimation;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;
class GenrePage;
class QStackedWidget;
class ClickableLabel;
class QListWidget;
class TagListView;
class GenreViewPage;

class XAMP_WIDGET_SHARED_EXPORT AlbumTabListView : public QListView {
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

class XAMP_WIDGET_SHARED_EXPORT AlbumArtistPage final : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxCompletionCount = 10;

	explicit AlbumArtistPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

	ArtistView* artist() const {
		return artist_view_;
	}

	AlbumView* year() const {
		return year_view_;
	}

public slots:
	void refresh();

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

private:
	QVBoxLayout* genre_frame_layout_;
	QStandardItemModel* album_model_;
	QStandardItemModel* artist_model_;
	QFrame* album_frame_;
	QFrame* artist_frame_;
	QFrame* year_frame_;
	QLineEdit* album_search_line_edit_;
	QLineEdit* artist_search_line_edit_;
	AlbumTabListView* list_view_;
	AlbumView* album_view_;
	ArtistView* artist_view_;
	AlbumView* year_view_;
	ArtistInfoPage* artist_info_view_;
	TagListView* album_tag_list_widget_;
	TagListView* artist_tag_list_widget_;
	TagListView* year_tag_list_widget_;
};

