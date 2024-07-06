#include <widget/chatgpt/chatgptwidget.h>
#include <thememanager.h>

ChatGPTWindow::ChatGPTWindow(QWidget* parent) 
	: QWidget(parent) {
    mainLayout = new QVBoxLayout(this);

    // Chat display area
    QFrame* chatDisplay = new QFrame(this);
    QVBoxLayout* chatLayout = new QVBoxLayout(chatDisplay);
    mainLayout->addWidget(chatDisplay);

    // User input area
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLine = new QLineEdit(this);
	inputLine->setObjectName("inputLine");

    sendButton = new QPushButton(qTEXT("Send"), this);
    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);
    mainLayout->addLayout(inputLayout);

	qTheme.setLineEditStyle(inputLine, qTEXT("inputLine"));

    (void) QObject::connect(sendButton, &QPushButton::clicked, this, &ChatGPTWindow::doSendToChatGPT);
}

void ChatGPTWindow::doSendToChatGPT() {
    QString userMessage = inputLine->text();
    if (userMessage.isEmpty()) 
        return;    

    QFrame* userFrame = createMessageFrame(userMessage, ChatTextAlignmentRight, qTheme.fontIcon(Glyphs::ICON_PERSON).pixmap(40, 40));
    mainLayout->insertWidget(mainLayout->count() - 1, userFrame);

    inputLine->clear();
	emit sendToChatGPT(userMessage);
}

void ChatGPTWindow::chatGPTResponse(const QString& message) {
    QFrame* chatGptFrame = createMessageFrame(message, ChatTextAlignmentLeft, qTheme.fontIcon(Glyphs::ICON_AI).pixmap(40, 40));
    mainLayout->insertWidget(mainLayout->count() - 1, chatGptFrame);
}

QFrame* ChatGPTWindow::createMessageFrame(const QString& message,
    const ChatTextAlignment& alignment,
    const QPixmap& avatar) {
    QFrame* frame = new QFrame(this);
    QHBoxLayout* layout = new QHBoxLayout(frame);

    if (!avatar.isNull()) {
        QLabel* avatarLabel = new QLabel(frame);
        avatarLabel->setPixmap(avatar.scaled(40, 40, Qt::KeepAspectRatio));
        layout->addWidget(avatarLabel, 0);
    }

    QLabel* messageLabel = new QLabel(message, frame);
    messageLabel->setStyleSheet(qSTR("background-color: #424548; padding: 10px; border: 1px solid #4d4d4d; border-radius: 5px; color: white;"));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(messageLabel, 1);

    if (alignment == ChatTextAlignmentRight) {
        layout->addStretch();
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    frame->setLayout(layout);
    return frame;
}