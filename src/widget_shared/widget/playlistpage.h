//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>
#include <QLineEdit>
#include <QTabWidget>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/tabpage.h>
#include <widget/themecolor.h>

class ProcessIndicator;
class QStandardItemModel;
class QSpacerItem;
class QLabel;
class QToolButton;
class ScrollLabel;
class PlaylistTableView;

enum Match {
	MATCH_NONE,
	MATCH_ITEM,
	MATCH_SUGGEST,
};

class XAMP_WIDGET_SHARED_EXPORT PlaylistPage final : public QFrame, public TabPage {
	Q_OBJECT
public:
	static constexpr auto kMaxCompletionCount = 10;

	explicit PlaylistPage(QWidget *parent = nullptr);

	ProcessIndicator* spinner();

	PlaylistTableView* playlist();

	QLabel* cover();

    ScrollLabel* title();

	QLabel* format();

	QLabel* pageTitle();

	QToolButton* heart();

	QLineEdit* searchLineEdit();

	void setHeart(bool heart);

	void setCover(const QPixmap* cover);

	void hidePlaybackInformation(bool hide);

	void setAlbumId(int32_t album_id, int32_t heart);

	void addSuggestions(const QString& text);

	void showCompleter();

	void reload() override;
signals:
	void playMusic(int32_t playlist_id, const PlayListEntity& item, bool is_play);

	void search(const QString& text, Match match);

	void editFinished(const QString& text);

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

	void onSetCoverById(const QString& cover_id);

	void onRetranslateUi();

private:
	void initial();
	
	std::optional<int32_t> album_id_;
	bool album_heart_ = false;
	QLabel* page_title_label_{ nullptr };
	QToolButton *heart_button_{ nullptr };
	PlaylistTableView* playlist_{ nullptr };
	QLabel* cover_{ nullptr };
    ScrollLabel* title_{ nullptr };
	QLabel* format_{ nullptr };
	QLineEdit* search_line_edit_{ nullptr };
	QAction* search_line_action_{ nullptr };
	QSpacerItem* vertical_spacer_{ nullptr };
	QSpacerItem* horizontal_spacer_{ nullptr };
	QSpacerItem* horizontal_spacer_4_{ nullptr };
	QSpacerItem* horizontal_spacer_5_{ nullptr };
	QSpacerItem* horizontal_spacer_9_{ nullptr };
	QSpacerItem* middle_spacer_{ nullptr };
	QSpacerItem* right_spacer_{ nullptr };
	QSpacerItem* default_spacer_{ nullptr };
	QStandardItemModel* search_playlist_model_{ nullptr };
	QCompleter* playlist_completer_{ nullptr };
	ProcessIndicator* spinner_{ nullptr };
};
