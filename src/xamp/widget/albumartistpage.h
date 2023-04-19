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

#include <widget/themecolor.h>

class AlbumView;
class ArtistInfoPage;
class QPushButton;
class QLabel;

class ArtistStyledItemDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit ArtistStyledItemDelegate(QObject* parent = nullptr);

	void SetTextColor(QColor color);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
signals:
	void EnterAlbumView(const QModelIndex& index) const;

private:
	QColor text_color_;
};

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

class ArtistViewPage final : public QFrame {
	Q_OBJECT
public:
 	explicit ArtistViewPage(QWidget* parent = nullptr);

	void SetArtistImage(const QPixmap &image);

	void resizeEvent(QResizeEvent* event) override;
private:
	QLabel* artist_image_;
	QPushButton* close_button_;
	AlbumView* album_view_;
};

class ArtistView final : public QListView {
	Q_OBJECT
public:
	explicit ArtistView(QWidget* parent = nullptr);

	void OnThemeChanged(QColor backgroundColor, QColor color);

	void Refresh();
private:
	void resizeEvent(QResizeEvent* event) override;

	ArtistViewPage* page_;
	QSqlQueryModel model_;
};

class AlbumArtistPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumArtistPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

public slots:
	void Refresh();

	void SetArtistId(const QString& artist, const QString& cover_id, int32_t artist_id);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

private:
	AlbumTabListView* list_view_;
	AlbumView* album_view_;
	ArtistView* artist_view_;
	ArtistInfoPage* artist_info_view_;
};

