#include <widget/youtubedl/youtubedl.h>

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValueRef>
#include <QString>
#include <QProcess>

XAMP_DECLARE_LOG_NAME(YoutubeDL);

YoutubeDL::YoutubeDL() {
#ifdef Q_OS_WIN
    program_ = QApplication::applicationDirPath() + "/youtube-dl.exe";
#else
    program = "youtube-dl"; // assuming it is in PATH on other platforms
#endif
    QObject* parent = QApplication::instance();
    process_ = new QProcess(parent);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    logger_ = LoggerManager::GetInstance().GetLogger(kYoutubeDLLoggerName);
    (void)QObject::connect(process_, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    (void)QObject::connect(process_, SIGNAL(finished(int, QProcess::ExitStatus)), 
        this, SLOT(onDownloadFinished(int, QProcess::ExitStatus)));
}

QJsonObject YoutubeDL::createJsonObject(const QString& url) {
    arguments_ << "-j" << "--no-playlist" << "--flat-playlist" << url;
    process_->setProcessChannelMode(QProcess::SeparateChannels);
    process_->start(program_, arguments_);
    process_->waitForFinished();
    const QByteArray output(this->process_->readAllStandardOutput());
    const auto json = QJsonDocument::fromJson(output);
    return json.object();
}

void YoutubeDL::availableFormats(const QString& url) {
    QVector<MediaFormat> formats;
    auto json_object = createJsonObject(url);
    auto json_formats = json_object["formats"].toArray();
    
    for (auto&& json_format : json_formats) {
        QJsonValue value = json_format;
        QJsonObject format_object = value.toObject();

    	MediaFormat format;
        format.format_id = format_object["format_id"].toString();
        format.format = format_object["format"].toString();
        format.extension = format_object["ext"].toString();
        format.note = format_object["format_note"].toString();

        if (format_object.contains("height") && !format_object["height"].isNull()) {
	        const auto resolution = QString::number(format_object["width"].toDouble()) + "x"
                + QString::number(format_object["height"].toDouble());
            format.resolution = resolution;
        }
        format.vcodec = format_object["vcodec"].toString().trimmed();
        format.acodec = format_object["acodec"].toString().trimmed();
        formats.append(format);
    }
    formats_ = formats;
}

QString YoutubeDL::getUrl(const QString& url) {
    arguments_ << "-g" << url;
    process_->start(this->program_, this->arguments_);
    process_->waitForFinished();
    QString output(this->process_->readAllStandardOutput());
    return output;
}

void YoutubeDL::setFormat(const QString& format) {
    arguments_ << "-f" << format;
}

void YoutubeDL::startDownload(const QString& url, const QString& working_directory) {
    arguments_ << url;
    process_->setWorkingDirectory(working_directory);
    process_->start(this->program_, this->arguments_);
}

QVector<MediaFormat> YoutubeDL::formats() const {
    return formats_;
}

void YoutubeDL::setFormats(const QVector<MediaFormat>& value) {
    formats_ = value;
}

const QProcess* YoutubeDL::process() const {
    return process_;
}

void YoutubeDL::addArguments(const QString& arg) {
    arguments_ << arg;
}

void YoutubeDL::resetArguments() {
    arguments_.clear();
}

void YoutubeDL::onReadyRead() {
	
}

void YoutubeDL::onDownloadFinished(int exit_code, QProcess::ExitStatus exit_status) {
	
}