//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>

#include <widget/playlistentity.h>

class QSpacerItem;
class QLabel;
class ScrollLabel;
class PlayListTableView;

class PlaylistPage : public QFrame {
	Q_OBJECT
public:
	explicit PlaylistPage(QWidget *parent = nullptr);

	PlayListTableView* playlist();

	QLabel* cover();

    ScrollLabel* title();

	QLabel* format();

	void setCover(const QPixmap* cover);

	void hidePlaybackInformation(bool hide);

signals:
	void playMusic(const PlayListEntity& item);

public slots:
    void OnThemeColorChanged(QColor theme_color, QColor color);

	void setCoverById(const QString& cover_id);

private:
	void initial();
	
	PlayListTableView* playlist_;
	QLabel* cover_;
    ScrollLabel* title_;
	QLabel* format_;
	QSpacerItem* vertical_spacer_;
	QSpacerItem* horizontal_spacer_;
	QSpacerItem* horizontalSpacer_4_;
	QSpacerItem* horizontalSpacer_5_;
	QSpacerItem* middle_spacer_;
	QSpacerItem* right_spacer_;
	QSpacerItem* default_spacer_;
};
