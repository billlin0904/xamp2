//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QTableView>
#include <QSqlQueryModel>
#include <QItemDelegate>

#include <widget/albumview.h>

class PushButtonDelegate : public QItemDelegate {
public:
    explicit PushButtonDelegate(QObject *parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    void initStyleOptionButton(QStyleOptionButton* out, const QStyleOptionViewItem& in, const QModelIndex& index);
};

class CheckBoxDelegate : public QItemDelegate {
public:
	explicit CheckBoxDelegate(QObject* parent = nullptr);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
};

class PlaybackHistoryModel : public QSqlQueryModel {
public:
	enum {
        HISTORY_ID_ROW = 12,
		CHECKBOX_ROW = 11,
	};

	explicit PlaybackHistoryModel(QObject* parent = nullptr);

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void selectAll(bool check);
private:
    void updateSelected(const QModelIndex& index, bool check);
};

class PlaybackHistoryTableView : public QTableView {
	Q_OBJECT
public:
	enum {
		PLAYLIST_TRACK,
		PLAYLIST_TITLE,
		PLAYLIST_DURATION,
	};

	explicit PlaybackHistoryTableView(QWidget* parent = nullptr);

	void refreshOnece();

    void selectAll(bool check);
private:
	void resizeColumn();

	PlaybackHistoryModel model_;
};

class PlaybackHistoryPage : public QFrame {
	Q_OBJECT
public:
	explicit PlaybackHistoryPage(QWidget* parent = nullptr);

	PlaybackHistoryTableView* playlist() {
		return playlist_;
	}

	void refreshOnece();

signals:
    void playMusic(const MusicEntity& entity);

	void save();

public slots:
    void playNextMusic();

private:
	PlaybackHistoryTableView* playlist_;
};
