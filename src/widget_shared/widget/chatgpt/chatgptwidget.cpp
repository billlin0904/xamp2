#include <widget/chatgpt/chatgptwidget.h>
#include <widget/chatgpt/waveformwidget.h>
#include <widget/util/str_util.h>
#include <QAudioDevice>
#include <QMediaDevices>
#include <ui_chatgptwindow.h>
#include <thememanager.h>
#include <QDir>

ChatGPTWindow::ChatGPTWindow(QWidget* parent) 
	: QFrame(parent) {
    ui_ = new Ui::ChatGPTWindow();
	ui_->setupUi(this);
    setObjectName(qTEXT("chatgptWindow"));
	initial();
}

ChatGPTWindow::~ChatGPTWindow() {
	delete ui_;
}

void ChatGPTWindow::onThemeChangedFinished(ThemeColor theme_color) {
    qTheme.setRecordIcon(ui_->recordOrSendButton, false);
    qTheme.setCancelRecordIcon(ui_->cancelButton);
}

void ChatGPTWindow::initial() {   
    auto output = QMediaDevices::audioOutputs();
	for (const auto& device : output) {
		ui_->outputDeviceComboBox->addItem(device.description());
	}

	ui_->outputDeviceComboBox->setStyleSheet(qTEXT(R"(
    QComboBox#outputDeviceComboBox {
         border: 1px solid #4d4d4d;
         border-radius: 6px;
         background-color: #424548; 
         color: white;
     })"));

    ui_->recordOrSendButton->setStyleSheet(qSTR(R"(
                                          QToolButton#recordOrSendButton {
                                          border: 1px solid #4d4d4d;
                                          background-color: transparent;
										  border-radius: 6px;
                                          }
										  QToolButton#recordOrSendButton:hover {		
										  background-color: %1;
										  border-radius: 6px;								 
                                          }
                                          )").arg(colorToString(qTheme.hoverColor())));

    ui_->cancelButton->setStyleSheet(qSTR(R"(
                                          QToolButton#cancelButton {
                                          border: 1px solid #4d4d4d;
                                          background-color: transparent;
										  border-radius: 6px;
                                          }
										  QToolButton#cancelButton:hover {		
										  background-color: %1;
										  border-radius: 6px;								 
                                          }
                                          )").arg(colorToString(qTheme.hoverColor())));

    (void)QObject::connect(ui_->outputDeviceComboBox,
        &QComboBox::activated,
        [this, output](const auto& index) {
			
        });

    (void)QObject::connect(&speech_to_text_,
        &SpeechToText::sampleReady,
        ui_->wave_widget,
        &WaveformWidget::readAudioData);

    (void)QObject::connect(&speech_to_text_,
        &SpeechToText::silenceDetected,
        ui_->wave_widget,
        &WaveformWidget::silence);

    (void)QObject::connect(ui_->cancelButton, &QToolButton::clicked, [this]() {
        speech_to_text_.stop();
        qTheme.setRecordIcon(ui_->recordOrSendButton, false);
        ui_->cancelButton->hide();
        is_recording = false;
        });

    speech_to_text_.stop();

    (void)QObject::connect(&speech_to_text_, &SpeechToText::resultReady, [this](const auto &text) {
        ui_->plainTextEdit->setPlainText(text);
        });

    (void)QObject::connect(ui_->recordOrSendButton, &QToolButton::clicked, [output, this]() {
		if (ui_->readlTimeMonitoringCheckBox->isChecked()) {
            auto index = ui_->outputDeviceComboBox->currentIndex();
            speech_to_text_.setRealtimeMonitorDevice(output[index]);
		}

        if (!is_recording) {
            speech_to_text_.loadModel(QDir::currentPath() + qTEXT("/model/") + qTEXT("ggml-tiny.bin"));
            qTheme.setRecordIcon(ui_->recordOrSendButton, true);
            ui_->cancelButton->show();
            is_recording = true;
        }
        else {
            speech_to_text_.stop();
            qTheme.setRecordIcon(ui_->recordOrSendButton, false);
            ui_->cancelButton->hide();
            is_recording = false;
        }
        });

    qTheme.setRecordIcon(ui_->recordOrSendButton, false);
	qTheme.setCancelRecordIcon(ui_->cancelButton);
	ui_->cancelButton->hide();

    setStyleSheet(qTEXT("QFrame#chatgptWindow { background-color: transparent; border: none; }"));
    ui_->frame->setStyleSheet(qTEXT("QFrame#frame { background-color: transparent; border: none; }"));
    ui_->plainTextEdit->setStyleSheet(qTEXT("QPlainTextEdit#plainTextEdit { background-color: transparent; border: none; }"));
}