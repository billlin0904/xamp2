#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include <QMetaObject>

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
    speech_detected_.reset(new SpeechDetected());
    whisper_.reset(new WhisperService());
    whisper_->loadModel(file_path);
    whisper_->moveToThread(&whisper_thread_);
    whisper_thread_.start();
}

void SpeechToText::start() {
    auto device = QMediaDevices::defaultAudioInput();
    QAudioFormat fmt;

    fmt.setSampleFormat(QAudioFormat::Float);
    fmt.setSampleRate(kSampleRate);
    fmt.setChannelConfig(QAudioFormat::ChannelConfigMono);
    fmt.setChannelCount(1);
    if (!device.isFormatSupported(fmt)) {
        return;
    }

    source_.reset(new QAudioSource{ device, fmt });
    device_ = source_->start();

    (void) QObject::connect(device_, &QIODevice::readyRead, this, [this](){
        auto bytes = device_->readAll();
        float samples_count = bytes.size() / source_->format().bytesPerSample();
        auto time_count     = samples_count / source_->format().sampleRate();
        std::vector<float> frame{ reinterpret_cast<const float *>(bytes.cbegin()),
                                 reinterpret_cast<const float *>(bytes.cend()) };
        speech_detected_->feedSamples(std::move(frame));
    });

    (void) QObject::connect(speech_detected_.get(), &SpeechDetected::speechDetected, this, [this](const auto &samples){
        QMetaObject::invokeMethod(whisper_.get(),
            "readSamples",
            Qt::QueuedConnection,
            Q_ARG(std::vector<float>, samples));
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
