#include <widget/chatgpt/chatgptwidget.h>
#include <thememanager.h>

ChatGPTWindow::ChatGPTWindow(QWidget* parent) 
	: QWidget(parent) {
    main_layout_ = new QVBoxLayout(this);

    auto* chat_display = new QFrame(this);
    auto* chat_layout = new QVBoxLayout(chat_display);
    main_layout_->addWidget(chat_display);

    //auto* input_layout = new QHBoxLayout();
    //input_line_ = new QLineEdit(this);
	//input_line_->setObjectName("inputLine");

    //send_button_ = new QPushButton(qTEXT("Send"), this);
    //input_layout->addWidget(input_line_);
    //input_layout->addWidget(send_button_);
    //main_layout_->addLayout(input_layout);

	//qTheme.setLineEditStyle(input_line_, qTEXT("inputLine"));

    //(void) QObject::connect(send_button_, &QPushButton::clicked, this, &ChatGPTWindow::doSendToChatGPT);

    chatGPTResponse(qTEXT("Test"));
    showSendMessage(qTEXT("Test"));
    chatGPTResponse(qTEXT("Test"));
    showSendMessage(qTEXT("Test"));
    chatGPTResponse(qTEXT("Test"));
    showSendMessage(qTEXT("Test"));
}

void ChatGPTWindow::doSendToChatGPT() {
    auto user_message = input_line_->text();
    if (user_message.isEmpty()) 
        return;    
	showSendMessage(user_message);
    input_line_->clear();
	emit sendToChatGPT(user_message);
}

void ChatGPTWindow::showSendMessage(const QString& user_message) {
    auto* user_frame = createMessageFrame(user_message, qTheme.fontIcon(Glyphs::ICON_PERSON).pixmap(40, 40), true);
    main_layout_->insertWidget(main_layout_->count() - 1, user_frame);
}

void ChatGPTWindow::chatGPTResponse(const QString& message) {
    auto* chat_gpt_frame = createMessageFrame(message, qTheme.fontIcon(Glyphs::ICON_AI).pixmap(40, 40));
    main_layout_->insertWidget(main_layout_->count() - 1, chat_gpt_frame);
}

QFrame* ChatGPTWindow::createMessageFrame(const QString& message, const QPixmap& avatar, bool is_user) {
    auto* frame = new QFrame(this);
    auto* layout = new QHBoxLayout(frame);

    if (!avatar.isNull()) {
        auto* avatar_label = new QLabel(frame);
        avatar_label->setPixmap(avatar.scaled(40, 40, Qt::KeepAspectRatio));
        layout->addWidget(avatar_label, 0);
    }

    auto* message_label = new QLabel(message, frame);
    if (is_user) {
        message_label->setStyleSheet(qSTR("background-color: #424548; padding: 10px; border: 1px solid #4d4d4d; border-radius: 5px; color: white;"));
    }
    else {
        message_label->setStyleSheet(qSTR("background-color: transparent; color: white;"));
    }
    
    message_label->setWordWrap(true);
    message_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(message_label, 1);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    frame->setLayout(layout);
    return frame;
}