#include <QRegularExpression>
#include <QDebug>
#include <widget/musixmatchclient.h>

inline ConstLatin1String kAPIKey = Q_UTF8("");
inline ConstLatin1String kUrl = Q_UTF8("http://api.musixmatch.com/ws/1.1/");

static QString stripped(QString s) {
	return s.replace(Q_UTF8("/"), Q_UTF8("-"))
		.remove(QRegularExpression(Q_UTF8("[^\\w0-9\\- ]"), QRegularExpression::UseUnicodePropertiesOption))
		.simplified()
		.replace(Q_UTF8(" "), Q_UTF8("-"))
		.replace(QRegularExpression(Q_UTF8("(-)\\1+")), Q_UTF8("-"))
		.toLower();
}

MusixmatchClient::MusixmatchClient(QNetworkAccessManager* manager, QObject* parent)
	: QObject(parent)
	, manager_(manager) {
}

QString MusixmatchClient::getUrl(QString const& url) const {
	return QString(Q_UTF8("%1%2&apikey=%3")).arg(kUrl)
	.arg(url)
	.arg(kAPIKey);
}

void MusixmatchClient::matcherLyrics(QString const& title, QString const& artist, QString format) {
	const auto url = QUrl(
		Q_UTF8("https://www.musixmatch.com/lyrics/%1/%2").
		arg(stripped(artist), stripped(title))
	);

	auto handler = [=](const QString& msg) {
		QString content = msg;
		QString data_begin = Q_UTF8("var __mxmState = ");
		QString data_end = Q_UTF8(";</script>");
		int begin_idx = content.indexOf(data_begin);
		QString content_json;
		if (begin_idx > 0) {
			begin_idx += data_begin.length();
			int end_idx = content.indexOf(data_end, begin_idx);
			if (end_idx > begin_idx) {
				content_json = content.mid(begin_idx, end_idx - begin_idx);
			}
		}		
		if (content_json.contains(QRegularExpression(Q_UTF8("<[^>]*>")))) {
			return;
		}
		qDebug() << content_json;
	};

	http::HttpClient(url, manager_)
		.success(handler)
		.get();
}

