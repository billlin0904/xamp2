#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include <QApplication>
#include <QPermissions>
#include <QMetaObject>
#include <QTimer>

#include <widget/chatgpt/permission.h>

#include <widget/chatgpt/whisperservice.h>
#include <widget/chatgpt/speechdetected.h>
#include <widget/chatgpt/speechtotext.h>

SpeechToText::SpeechToText() {
}

SpeechToText::~SpeechToText() {
    stopService();
}

void SpeechToText::stopService() {
    if (!whisper_thread_.isFinished()) {
        whisper_thread_.requestInterruption();
        whisper_thread_.quit();
        whisper_thread_.wait();
    }
}

void SpeechToText::loadModel(const QString &file_path) {
    QMicrophonePermission microphonePermission;

    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, [=](const auto &permission) {
            qDebug() << "Asking permission within the Denied case";
        });
        //requestMicrophonePermission();
        break;
    case Qt::PermissionStatus::Denied:
        qApp->requestPermission(microphonePermission, this, [=](const auto &permission) {
            qDebug() << "Asking permission within the Denied case";
        });
        //requestMicrophonePermission();
        break;
    case Qt::PermissionStatus::Granted:
        speech_detected_.reset(new SpeechDetected());
        whisper_.reset(new WhisperService());
        whisper_->loadModel(file_path);
        whisper_->moveToThread(&whisper_thread_);
        whisper_thread_.start();
        start();
        break;
    }
}

void SpeechToText::start() {
    auto device = QMediaDevices::defaultAudioInput();
    QAudioFormat fmt;

    fmt.setSampleFormat(QAudioFormat::Float);
    fmt.setSampleRate(kSampleRate);
    fmt.setChannelCount(1);
    fmt.setChannelConfig(QAudioFormat::ChannelConfigMono);

    auto description = device.description();

    if (!device.isFormatSupported(fmt)) {
        return;
    }

    source_.reset(new QAudioSource{ device, fmt });
    device_ = source_->start();
    
    (void) QObject::connect(device_, &QIODevice::readyRead, this, [this](){
        static constexpr auto kSilenceThreshold = 8;
        static constexpr auto kMinDetectedSamples = 320;
        static constexpr auto kMaxSamples = 16000 * 5;

        auto bytes = device_->readAll();

        const auto* samples = reinterpret_cast<const float*>(bytes.data());
        auto sample_count = bytes.size() / sizeof(float);        

        for (int i = 0; i < sample_count; ++i) {
            buffer_.push_back(samples[i]);

            if (buffer_.size() == kMinDetectedSamples) {
                std::vector<int16_t> temp;
                temp.resize(buffer_.size());

                for (int i = 0; i < buffer_.size(); i++) {
                    temp[i] = static_cast<int16_t>(buffer_[i] * 32768.0f);
                }

                emit sampleReady(temp);

                if (speech_detected_->isSpeech(temp)) {
                    input_.insert(input_.end(), buffer_.begin(), buffer_.end());
                    silence_counter_ = 0;
                }
                else {
                    if (silence_counter_ > kSilenceThreshold) {
                        if (input_.size() >= kMaxSamples) {
                            QMetaObject::invokeMethod(whisper_.get(),
                                "readSamples",
                                Qt::QueuedConnection,
                                Q_ARG(std::vector<float>, input_));
                            input_.clear();
                        }						
					}
                    silence_counter_++;
                }
                buffer_.clear();
            }
        }
    });

    (void) QObject::connect(whisper_.get(), &WhisperService::resultReady, this, [this](auto s){
        emit resultReady(s);
    });
}

void SpeechToText::stop() {
    if (source_){
        source_->stop();
        source_.reset();
    }

    QObject::disconnect(speech_detected_.get(), nullptr, this, nullptr);
    speech_detected_.reset();
}
