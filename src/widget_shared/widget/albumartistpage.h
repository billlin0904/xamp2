//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QListView>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPropertyAnimation>

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

class GenrePage : public QFrame {
	Q_OBJECT
public:
	explicit GenrePage(QWidget* parent = nullptr);

	void SetGenre(const QString& genre);

	GenreView* view() {
		return genre_view_;
	}
signals:
	void goBackPage();

private:
	ClickableLabel* genre_label_;
	GenreView* genre_view_;
};

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
	static constexpr size_t kMaxCompletionCount = 10;

	explicit AlbumArtistPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

	ArtistView* artist() const {
		return artist_view_;
	}

public slots:
	void Refresh();

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

private:
	void AddGenreList(GenrePage* page, QStackedWidget* stack, const QString& genre);
	
	static const QSet<QString> kGenre;

	QVBoxLayout* genre_frame_layout_;
	QStandardItemModel* album_model_;
	QStandardItemModel* artist_model_;
	QFrame* album_frame_;
	QFrame* artist_frame_;
	QFrame* genre_frame_;
	QList<GenreView*> genre_list_;
	QList<GenrePage*> genre_page_list_;
	QComboBox* category_combo_box_;
	QLineEdit* album_search_line_edit_;
	QLineEdit* artist_search_line_edit_;
	AlbumTabListView* list_view_;
	AlbumView* album_view_;
	ArtistView* artist_view_;
	ArtistInfoPage* artist_info_view_;
};

