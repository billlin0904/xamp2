//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QRunnable>
#include <QObject>
#include <QImage>
#include <QDir>
#include <QCache>

#include <QWidget>
#include <QDir>

class CoverFlowImageLoader : public QObject, public QRunnable {
	Q_OBJECT
public:
	explicit CoverFlowImageLoader(const QImage& image);

	explicit CoverFlowImageLoader(const QString& path);

	~CoverFlowImageLoader();

	void run();
signals:
	void completed(const QImage& image);

private:
	QImage image_;
	QString path_;
};

class CoverFlowItemsLoader : public QObject, public QRunnable {
	Q_OBJECT
public:
	explicit CoverFlowItemsLoader(const QDir& directory);

	~CoverFlowItemsLoader();

	void run();

signals:
	void completedItem(const QString& path);

private:
	QDir dir_;
};

class CoverFlowItem : public QObject {
	Q_OBJECT

public:
	CoverFlowItem(const QString& title, const QString& path, QObject* parent = nullptr);

	~CoverFlowItem();

	const QImage* image() const;

	QString path() const;

	QString title() const;

signals:
	void imageChanged(void);

public slots:
	void setImage(const QImage& image);

private:
	QString title_;
	QString path_;
	QImage image_;
};

class CoverFlowWidget : public QWidget {
	Q_OBJECT
public:
	explicit CoverFlowWidget(QWidget* parent = nullptr);

public slots:
	void addItem(const QString& path);

	void addItem(const QString& title, const QString& path);

	void addItems(const QString& path);

	void addItems(const QDir& directory);

	void showNext();

	void showPrevious();

	void showAtIndex(int index);

private:
	void paintEvent(QPaintEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void keyPressEvent(QKeyEvent* event) override;

private:
	void drawSelectedItem(QPainter* painter);
	
	void drawItemsAfterSelected(QPainter* painter);

	void drawItemsBeforeSelected(QPainter* painter);

	void drawItemAt(QPainter* painter, int x, int y, const QImage* img, int w, int h, int angle);

	const QImage* itemImage(int index) const;

	void init();

	int selected_;
	QCache<qint64, QImage> mirror_cache_;
	QList<CoverFlowItem*> items_;
	QImage empty_item_;	
};
