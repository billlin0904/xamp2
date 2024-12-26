//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/databasecoverid.h>

#include <QPushButton>
#include <QStyledItemDelegate>
#include <QCheckBox>

enum {
	ALBUM_INDEX_ALBUM = 0,
	ALBUM_INDEX_COVER,
	ALBUM_INDEX_ARTIST,
	ALBUM_INDEX_ALBUM_ID,
	ALBUM_INDEX_ARTIST_ID,
	ALBUM_INDEX_ARTIST_COVER_ID,
	ALBUM_INDEX_ALBUM_YEAR,
	ALBUM_INDEX_ALBUM_HEART,
	ALBUM_INDEX_IS_HIRES,
	ALBUM_INDEX_IS_SELECTED
};

enum ShowModes {
	SHOW_ARTIST,
	SHOW_YEAR,
	SHOW_NORMAL,
};

class XAMP_WIDGET_SHARED_EXPORT AlbumViewStyledDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void setAlbumTextColor(QColor color);

	void enableAlbumView(bool enable);

	void setPlayingAlbumId(int32_t album_id) {
		playing_album_id_ = album_id;
	}

	void setShowMode(ShowModes mode) {
		show_mode_ = mode;
	}

	void setSelectedMode(bool enable) {
		enable_selected_mode_ = enable;
		is_selected_all_ = true;
	}

	bool isSelectedMode() const {
		return enable_selected_mode_;
	}

	bool isSelectedAll() const {
		return is_selected_all_;
	}

	void setSelectedAll(bool selected) {
		is_selected_all_ = selected;
	}

	ShowModes showModes() const {
		return show_mode_;
	}

	void setCoverSize(const QSize& size) {
		cover_size_ = size;
	}
signals:
	void enterAlbumView(const QModelIndex& index) const;

	void editAlbumView(const QModelIndex& index, bool state) const;

	void showAlbumMenu(const QModelIndex& index, const QPoint& pt) const;

	void findAlbumCover(const DatabaseCoverId& id) const;

	void stopRefreshCover() const;
protected:
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;

private:
	bool enable_selected_mode_{ false };
	bool is_selected_all_{ true };
	bool enable_album_view_{ true };
	ShowModes show_mode_{ SHOW_ARTIST };
	int32_t playing_album_id_{ -1 };
	QSize cover_size_;
	QColor album_text_color_;
	QPoint mouse_point_;
	QPixmap mask_image_;
	QScopedPointer<QPushButton> more_album_opt_button_;
	QScopedPointer<QPushButton> play_button_;
	QScopedPointer<QCheckBox> edit_mode_checkbox_;
};

