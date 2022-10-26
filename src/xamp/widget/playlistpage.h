//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QString>

#include <widget/playlistentity.h>

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
};
