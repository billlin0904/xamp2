//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>
#include <QLineEdit>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>

class QSpacerItem;
class QLabel;
class QToolButton;
class ScrollLabel;
class PlayListTableView;

class XAMP_WIDGET_SHARED_EXPORT PlaylistPage : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxCompletionCount = 10;

	explicit PlaylistPage(QWidget *parent = nullptr);

	PlayListTableView* playlist();

	QLabel* cover();

    ScrollLabel* title();

	QLabel* format();

	void SetCover(const QPixmap* cover);

	void HidePlaybackInformation(bool hide);

	void SetAlbumId(int32_t album_id, int32_t heart);

signals:
	void PlayMusic(const PlayListEntity& item);

public slots:
    void OnThemeColorChanged(QColor theme_color, QColor color);

	void SetCoverById(const QString& cover_id);

private:
	void Initial();
	
	std::optional<int32_t> album_id_;
	int32_t album_heart_{ 0 };
	QToolButton *heart_button_{nullptr};
	PlayListTableView* playlist_;
	QLabel* cover_;
    ScrollLabel* title_;
	QLabel* format_;
	QLineEdit* search_line_edit_;
	QSpacerItem* vertical_spacer_;
	QSpacerItem* horizontal_spacer_;
	QSpacerItem* horizontalSpacer_4_;
	QSpacerItem* horizontalSpacer_5_;
	QSpacerItem* middle_spacer_;
	QSpacerItem* right_spacer_;
	QSpacerItem* default_spacer_;
};
