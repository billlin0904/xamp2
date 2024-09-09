//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <widget/widget_shared_global.h>
#include <widget/themecolor.h>

class SimilarSongViewPage;
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
class QScrollArea;

class XAMP_WIDGET_SHARED_EXPORT AlbumTabListView : public QListView {
	Q_OBJECT
public:
	explicit AlbumTabListView(QWidget* parent = nullptr);

	void addTab(const QString& name, int tab_id);

	void setCurrentTab(int tab_id);

	void setTabText(const QString& name, int tab_id);
signals:
	void clickedTable(int table_id);

	void tableNameChanged(int table_id, const QString& name);

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

	AlbumView* albumRecentPlays() const {
		return recent_plays_album_view_;
	}

	ArtistView* artist() const {
		return artist_view_;
	}

	AlbumView* year() const {
		return year_view_;
	}

	SimilarSongViewPage* similarSong() const {
		return similar_song_frame_;
	}

signals:
	void removeAll();

public slots:
	void reload();

	void onThemeChangedFinished(ThemeColor theme_color);

	void onRetranslateUi();

private:
	QLabel* page_title_label_{ nullptr };
	QVBoxLayout* genre_frame_layout_{ nullptr };
	QStandardItemModel* album_model_{ nullptr };
	QStandardItemModel* artist_model_{ nullptr };
	QFrame* album_frame_{ nullptr };
	QFrame* artist_frame_{ nullptr };
	QFrame* year_frame_{ nullptr };
	SimilarSongViewPage* similar_song_frame_{ nullptr };
	QLineEdit* album_search_line_edit_{ nullptr };
	QAction* album_search_action_{ nullptr };
	QLineEdit* artist_search_line_edit_{ nullptr };
	QAction* artist_search_action_{ nullptr };
	AlbumTabListView* album_tab_list_view_{ nullptr };
	AlbumView* album_view_{ nullptr };
	AlbumView* recent_plays_album_view_{ nullptr };
	ArtistView* artist_view_{ nullptr };
	AlbumView* year_view_{ nullptr };
	ArtistInfoPage* artist_info_view_{ nullptr };
	TagListView* album_tag_list_widget_{ nullptr };
	TagListView* artist_tag_list_widget_{ nullptr };
	TagListView* year_tag_list_widget_{ nullptr };
};

