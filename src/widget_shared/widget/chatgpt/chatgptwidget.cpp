#include <widget/chatgpt/chatgptwidget.h>
#include <widget/chatgpt/waveformwidget.h>
#include <thememanager.h>
#include <QDir>

ChatGPTWindow::ChatGPTWindow(QWidget* parent) 
	: QWidget(parent) {
	initial();
}

void ChatGPTWindow::initial() {
    speech_to_text_.loadModel(QDir::currentPath() + qTEXT("/model/") + qTEXT("ggml-tiny.bin"));

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    setLayout(default_layout);

    auto* wave_widget = new WaveformWidget(this);
    default_layout->addWidget(wave_widget);

    (void)QObject::connect(&speech_to_text_,
                    &SpeechToText::sampleReady,
                    wave_widget,
                    &WaveformWidget::readAudioData);
}


