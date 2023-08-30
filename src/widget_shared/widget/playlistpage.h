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
	QToolButton *heart_button_{ nullptr };
	PlayListTableView* playlist_{ nullptr };
	QLabel* cover_{ nullptr };
    ScrollLabel* title_{ nullptr };
	QLabel* format_{ nullptr };
	QLineEdit* search_line_edit_{ nullptr };
	QSpacerItem* vertical_spacer_{ nullptr };
	QSpacerItem* horizontal_spacer_{ nullptr };
	QSpacerItem* horizontal_spacer_4_{ nullptr };
	QSpacerItem* horizontal_spacer_5_{ nullptr };
	QSpacerItem* middle_spacer_{ nullptr };
	QSpacerItem* right_spacer_{ nullptr };
	QSpacerItem* default_spacer_{ nullptr };
};
