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

#include <widget/themecolor.h>
#include <widget/chatgpt/speechtotext.h>
#include <widget/util/str_util.h>

namespace Ui {
    class ChatGPTWindow;
}

class XAMP_WIDGET_SHARED_EXPORT ChatGPTWindow : public QFrame {
    Q_OBJECT
public:
    explicit ChatGPTWindow(QWidget* parent = nullptr);

    ~ChatGPTWindow() override;
    
    void initial();

signals:


public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:
    bool is_recording = false;
    SpeechToText speech_to_text_;
    Ui::ChatGPTWindow* ui_;
};
