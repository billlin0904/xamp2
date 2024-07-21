#include <widget/chatgpt/chatgptwidget.h>
#include <thememanager.h>
#include <QDir>

ChatGPTWindow::ChatGPTWindow(QWidget* parent) 
	: QWidget(parent) {
	initial();
}

void ChatGPTWindow::initial() {
	speech_to_text_.loadModel(QDir::currentPath() + qTEXT("/model/") + qTEXT("ggml-large-v2-q5_0.bin"));
	speech_to_text_.start();
}
