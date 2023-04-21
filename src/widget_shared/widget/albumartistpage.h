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
class ArtistInfoPage;
class QPushButton;
class QLabel;
class QPropertyAnimation;

class XAMP_WIDGET_SHARED_EXPORT ArtistStyledItemDelegate : public QStyledItemDelegate {
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

class XAMP_WIDGET_SHARED_EXPORT ArtistViewPage final : public QFrame {
	Q_OBJECT
public:
 	explicit ArtistViewPage(QWidget* parent = nullptr);

	void SetArtist(const QString &artist, int32_t artist_id, const QPixmap &image);

	void paintEvent(QPaintEvent* event) override;

	void OnCurrentThemeChanged(ThemeColor theme_color);

	AlbumView* album() const {
		return album_view_;
	}
private:
	QLabel* artist_name_;	
	QLabel* artist_image_;
	QPushButton* close_button_;
	AlbumView* album_view_;
};

class XAMP_WIDGET_SHARED_EXPORT ArtistView final : public QListView {
	Q_OBJECT
public:
	static constexpr auto kPageAnimationDurationMs = 200;

	explicit ArtistView(QWidget* parent = nullptr);

	void OnThemeChanged(QColor backgroundColor, QColor color);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void Refresh();

signals:
	void GetArtist(const QString& artist);

private:
	void resizeEvent(QResizeEvent* event) override;

	void ShowPageAnimation();

	void HidePageAnimation();

	bool enable_page_{ true };
	bool hide_page_{ false };
	ArtistViewPage* page_;
	QPropertyAnimation* animation_;
	QSqlQueryModel model_;
};

class XAMP_WIDGET_SHARED_EXPORT AlbumArtistPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumArtistPage(QWidget* parent = nullptr);

	AlbumView* album() const {
		return album_view_;
	}

	ArtistView* artist() const {
		return artist_view_;
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

