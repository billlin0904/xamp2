//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QList>
#include <QWidget>
#include <QTimer>

#include <widget/str_utilts.h>

class XMessageItem;
class QPaintEvent;
class QLabel;

enum MessageTypes {
    MSG_SUCCESS,
    MSG_ERROR,
    MSG_WARNING,
    MSG_INFORMATION
};

class XAMP_WIDGET_SHARED_EXPORT XMessage : public QWidget {
    Q_OBJECT
public:
    explicit XMessage(QWidget* parent = nullptr);

    ~XMessage() override;

    void Push(MessageTypes type, const QString &content);

    void SetDuration(int duration_ms);

private slots:
    void AdjustItemPosition(XMessageItem* item);

    void RemoveItem(XMessageItem* item);

private:
    int width_;
    int duration_ms_;
    QList<XMessageItem*> items_;
};

class XMessageItem : public QWidget {
    Q_OBJECT
public:
    explicit XMessageItem(QWidget* parent = nullptr,
        MessageTypes type = MessageTypes::MSG_INFORMATION,
        const QString& content = kEmptyString);

    ~XMessageItem() override;

    void Show();

    void Close();

    void SetDuration(int duration_ms);

signals:
    void ItemReadyRemoved(XMessageItem* item);

    void ItemRemoved(XMessageItem* item);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void AppearAnimation();

    void DisappearAnimation();

private:
    int width_;
    int height_;
    int duration_ms_;
    QLabel* icon_;
    QLabel* content_;
    QTimer timer_;
};