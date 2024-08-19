//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QStyledItemDelegate>
#include <QSqlQueryModel>

#include <widget/util/str_util.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

class PlayListTableFilterProxyModel;
class AlbumView;
class QLabel;
class QPushButton;
class QPropertyAnimation;

class ArtistStyledItemDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	static const ConstexprQString kArtistCacheTag;

	explicit ArtistStyledItemDelegate(QObject* parent = nullptr);

	void setTextColor(QColor color);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
signals:
	void enterAlbumView(const QModelIndex& index) const;

private:
	QColor text_color_;
};

class ArtistViewPage final : public QFrame {
	Q_OBJECT
public:	
	explicit ArtistViewPage(QWidget* parent = nullptr);

	void setArtist(const QString& artist, int32_t artist_id, const QString& artist_cover_id);

	void paintEvent(QPaintEvent* event) override;

	void onThemeChangedFinished(ThemeColor theme_color);

	AlbumView* album() const {
		return album_view_;
	}
private:
	QLabel* artist_name_;
	QLabel* artist_image_;
	QPushButton* close_button_;
	AlbumView* album_view_;
	QPixmap cover_;
};

class XAMP_WIDGET_SHARED_EXPORT ArtistView final : public QListView {
	Q_OBJECT
public:
	static constexpr auto kPageAnimationDurationMs = 200;

	explicit ArtistView(QWidget* parent = nullptr);

	void onThemeChangedFinished(ThemeColor theme_color);

	void showAll();

	void filterArtistName(const QSet<QString>& name);

	void reload();

signals:
	void getArtist(const QString& artist);

public slots:
	void search(const QString& keyword);

private:	
	void resizeEvent(QResizeEvent* event) override;

	void showPageAnimation();

	void hidePageAnimation();

	bool enable_page_{ true };
	bool hide_page_{ false };
	QString last_query_;
	ArtistViewPage* page_;
	QPropertyAnimation* animation_;
	QSqlQueryModel model_;
	PlayListTableFilterProxyModel* proxy_model_;
};