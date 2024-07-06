//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QLineEdit>

#include <widget/util/str_util.h>

class XAMP_WIDGET_SHARED_EXPORT ChatGPTWindow : public QWidget {
    Q_OBJECT
public:
    explicit ChatGPTWindow(QWidget* parent = nullptr);    
    
signals:
    void sendToChatGPT(const QString& message);

public slots:
    void chatGPTResponse(const QString& message);
    void doSendToChatGPT();

private:
    QFrame* createMessageFrame(const QString& message, const QPixmap& avatar, bool is_user = false);
	void showSendMessage(const QString& user_message);

    QVBoxLayout* main_layout_;
    QLineEdit* input_line_;
    QPushButton* send_button_;
};