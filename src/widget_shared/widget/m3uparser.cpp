#include <widget/m3uparser.h>
#include <widget/widget_shared.h>
#include <base/text_encoding.h>

#include <QString>
#include <QTextCodec>
#include <QList>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QUrl>
#include <QStringConverter>
#include <QIODevice>
#include <QRegularExpression>
#include <QFile>
#include <QDir>

namespace {
    const char kStandardM3uTextEncoding[] = "Windows-1250";
	constexpr auto kM3uHeader = "#EXTM3U"_str;
	constexpr auto kM3uCommentPrefix = "#"_str;
	const auto kUniveralEndOfLineRegEx = QRegularExpression("\r\n|\r|\n"_str);
}

bool M3uParser::isPlaylistFilenameSupported(const QString& fileName) {
	return fileName.endsWith(".m3u"_str, Qt::CaseInsensitive) ||
		fileName.endsWith(".m3u8"_str, Qt::CaseInsensitive);
}

bool M3uParser::writeM3UFile(const QString& file_name,
    const QList<QString>& items,
    const QString& playlist,
    bool useRelativePath,
    bool utf8_encoding) {    
    QDir base_directory(QFileInfo(file_name).canonicalPath());    

    QString file_contents("#EXTM3U\n"_str);
    if (!playlist.isEmpty()) {
        file_contents += "#PLAYLIST:"_str + playlist + "\n"_str;
    }

    for (const QString& item : items) {
        file_contents += "#EXTINF\n"_str;
        if (useRelativePath) {
            file_contents += base_directory.relativeFilePath(item) + "\n"_str;
        }
        else {
            file_contents += item + "\n"_str;
        }
    }

    QByteArray output;
    if (!utf8_encoding) {
        auto* codec = QTextCodec::codecForName(kStandardM3uTextEncoding);
        if (!codec) {
            return false;
        }
        output = QTextCodec::codecForName(kStandardM3uTextEncoding)->fromUnicode(file_contents);
    }
    else {
        output = file_contents.toUtf8();
    }

    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    file.write(output);
    file.close();
    return true;
}

QList<QString> M3uParser::parseM3UFile(const QString& file_name) {
    QList<QString> paths;

    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly)) {
        return paths;
    }

    auto byte_array = file.readAll();
	if (byte_array.isEmpty()) {
		return paths;	
    }

    QString file_contents;

    if (!TextEncoding().IsUtf8(byte_array.constData())) {
        file_contents = QTextCodec::codecForName(kStandardM3uTextEncoding)
            ->toUnicode(byte_array);
    }
    else {
        file_contents = QString::fromUtf8(byte_array);
    }

    if (!file_contents.startsWith(kM3uHeader)) {
        return paths;
    }

    const auto file_lines = file_contents.split(kUniveralEndOfLineRegEx);
    for (const QString& line : file_lines) {
        if (line.startsWith(kM3uCommentPrefix)) {
            continue;
        }
        paths.append(line);
    }
    return paths;
}