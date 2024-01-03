//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>
#include <QLineEdit>
#include <QTabWidget>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/themecolor.h>

class QStandardItemModel;
class QSpacerItem;
class QLabel;
class QToolButton;
class ScrollLabel;
class PlayListTableView;

enum Match {
	MATCH_ITEM,
	MATCH_NONE,
};

class XAMP_WIDGET_SHARED_EXPORT PlaylistPage : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxCompletionCount = 10;

	explicit PlaylistPage(QWidget *parent = nullptr);

	PlayListTableView* playlist();

	QLabel* cover();

    ScrollLabel* title();

	QLabel* format();

	QToolButton* heart();

	void setHeart(bool heart);

	void setCover(const QPixmap* cover);

	void hidePlaybackInformation(bool hide);

	void setAlbumId(int32_t album_id, int32_t heart);

	void addSuggestions(const QString& text);

	void showCompleter();

signals:
	void playMusic(const PlayListEntity& item);

	void search(const QString& text, Match match);

public slots:
    void onThemeColorChanged(QColor theme_color, QColor color);

	void onCurrentThemeChanged(ThemeColor theme_color);

	void onSetCoverById(const QString& cover_id);

private:
	void initial();
	
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
	QStandardItemModel* search_playlist_model_{ nullptr };
	QCompleter* playlist_completer_{ nullptr };
};
