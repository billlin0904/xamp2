//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/widget_shared_global.h>

class FlowLayout;
class QMouseEvent;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

class IconWidget : public QWidget {
public:
	IconWidget(const QIcon& icon, QWidget* parent = nullptr);

    void setIcon(const QIcon& icon);

	void paintEvent(QPaintEvent* event) override;
private:
    QIcon icon_;
};

class CardWidget : public QFrame {
public:
    CardWidget(const QIcon &icon, const QString & title, const QString& content, const QString& routeKey, int index, QWidget* parent = nullptr);

    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int index_;
    QString route_key_;
    IconWidget* icon_widget_;
    QLabel* title_label_;
    QLabel* content_label_;
    QHBoxLayout* h_box_layout_;
    QVBoxLayout* v_box_layout_;
};

class XAMP_WIDGET_SHARED_EXPORT SettingCard : public QFrame {
    Q_OBJECT
public:
    SettingCard(const QIcon& icon, const QString& title, const QString& content = QString(), QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setContent(const QString& content);
    QIcon icon() const;
    virtual void setValue(const QVariant& value);

    QHBoxLayout* h_box_layout_;
    QVBoxLayout* v_box_layout_;

private:
    QIcon icon_;
    IconWidget* icon_label_;
    QLabel* title_label_;
    QLabel* content_label_;
};

class XAMP_WIDGET_SHARED_EXPORT CardView : public QWidget {
public:
	explicit CardView(const QString& title, QWidget* parent = nullptr);

    void addCard(const QIcon& icon, const QString& title, const QString& content, const QString& routeKey, int index);

private:
    QLabel* title_label_;
    QVBoxLayout* v_box_layout_;
    FlowLayout* flow_layout_;
};