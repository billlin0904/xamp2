//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QPropertyAnimation>
#include <QStyledItemDelegate>
#include <QProgressDialog>
#include <QPushButton>

#include <widget/util/str_utilts.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/themecolor.h>
#include <widget/playlistentity.h>
#include <widget/lazyloadingmodel.h>
#include <widget/widget_shared_global.h>
#include <widget/playlisttableproxymodel.h>

class XProgressDialog;
class DatabaseFacade;
class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;
class XMessage;
class ProgressView;

enum ShowModes {
	SHOW_ARTIST,
	SHOW_YEAR,
	SHOW_NORMAL,
};

class XAMP_WIDGET_SHARED_EXPORT AlbumViewStyledDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:	
	static const ConstLatin1String kAlbumCacheTag;
	static constexpr auto kMoreIconSize = 20;
	static constexpr auto kIconSize = 40;

	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void setAlbumTextColor(QColor color);

	void enableAlbumView(bool enable);

	void setPlayingAlbumId(int32_t album_id) {
		playing_album_id_ = album_id;
	}

	void setShowMode(ShowModes mode) {
		show_mode_ = mode;
	}

	ShowModes showModes() const {
		return show_mode_;
	}	
signals:
	void enterAlbumView(const QModelIndex& index) const;

	void showAlbumMenu(const QModelIndex& index, const QPoint &pt) const;

	void findAlbumCover(int32_t album_id) const;

protected:
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;

private:
	QPixmap visibleCovers(const QString& cover_id) const;

	bool enable_album_view_{ true };
	ShowModes show_mode_{ SHOW_ARTIST };
	int32_t playing_album_id_{ -1 };
	QColor album_text_color_;
	QPoint mouse_point_;
	QPixmap mask_image_;
	QScopedPointer<QPushButton> more_album_opt_button_;
	QScopedPointer<QPushButton> play_button_;
};

class AlbumViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void setPlaylistMusic(const QString &album, int32_t album_id, const QString& cover_id, int32_t album_heart);

	PlaylistPage* playlistPage() const {
		return page_;
	}

signals:
	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void leaveAlbumView() const;

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

private:
	void paintEvent(QPaintEvent* event) override;

	PlaylistPage* page_;
	QPushButton* close_button_;	
};

class XAMP_WIDGET_SHARED_EXPORT AlbumView : public QListView {
	Q_OBJECT
public:
    static constexpr auto kPageAnimationDurationMs = 200;
	
	explicit AlbumView(QWidget* parent = nullptr);

	AlbumViewPage* albumViewPage();

	void enablePage(bool enable);

	virtual void showAll();

	void setPlayingAlbumId(int32_t album_id);

	void filterCategories(const QString& category);

	void filterCategories(const QSet<QString>& category);

	void filterYears(const QSet<QString>& years);

	void sortYears();

	void setShowMode(ShowModes mode);

	AlbumViewStyledDelegate* styledDelegate() {
		return styled_delegate_;
	}

	void resizeEvent(QResizeEvent* event) override;

signals:
    void addPlaylist(const QList<int32_t> &music_ids, const QList<PlayListEntity> &entities);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void clickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void removeAll();

	void extractFile(const QString& file_path, int32_t playlist_id);

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

	void reload();

	void filterByArtistId(int32_t artist_id);

	void hideWidget();

	void search(const QString& keyword);

    void onThemeColorChanged(QColor backgroundColor, QColor color);

    void append(const QString& file_name);

	void showMenu(const QPoint& pt);

	void showAlbumViewMenu(const QPoint& pt);

private:
	bool enable_page_{ true };

protected:
	QString last_query_;

private:
	AlbumViewPage* page_;
	AlbumViewStyledDelegate* styled_delegate_;
	QPropertyAnimation* animation_;
	LazyLoadingModel model_;
	PlayListTableFilterProxyModel* proxy_model_;
};

