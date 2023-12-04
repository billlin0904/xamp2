#include <QJsonDocument>
#include <QJsonObject>

#include <widget/cover_searcher/discogscoversearcher.h>
#include <widget/http.h>
#include <widget/str_utilts.h>

namespace {
	using ByteArrayPair = QPair<QByteArray, QByteArray>;
	using EncodedList = QList<ByteArrayPair>;

	constexpr ConstLatin1String kUrlSearch = "https://api.discogs.com/database/search";
	constexpr ConstLatin1String kUrlReleases = "https://api.discogs.com/releases";

	constexpr const char kAccessKeyB64[] = "YVR4Yk5JTnlmUkhFY0pTaldid2c=";
	constexpr const char kSecretKeyB64[] = "QkJNb2tMVXVUVFhSRWRUVmZDc0ZGamZmSWRjdHZRVno=";

	enum HashFunction {
		HASH_MD5,
		HASH_SHA1,
		HASH_SHA256,
	};

	QByteArray Hmac(const QByteArray& key, const QByteArray& data,
		HashFunction method) {
		const int kBlockSize = 64;  // bytes
		Q_ASSERT(key.length() <= kBlockSize);

		QByteArray inner_padding(kBlockSize, static_cast<char>(0x36));
		QByteArray outer_padding(kBlockSize, static_cast<char>(0x5c));

		for (int i = 0; i < key.length(); ++i) {
			inner_padding[i] = inner_padding[i] ^ key[i];
			outer_padding[i] = outer_padding[i] ^ key[i];
		}

		if (HASH_MD5 == method) {
			return QCryptographicHash::hash(
				outer_padding + QCryptographicHash::hash(inner_padding + data,
					QCryptographicHash::Md5),
				QCryptographicHash::Md5);
		}
		else if (HashFunction::HASH_SHA1 == method) {
			return QCryptographicHash::hash(
				outer_padding + QCryptographicHash::hash(inner_padding + data,
					QCryptographicHash::Sha1),
				QCryptographicHash::Sha1);
		}
		else {  // HASH_SHA256, currently default
			return QCryptographicHash::hash(
				outer_padding + QCryptographicHash::hash(inner_padding + data,
					QCryptographicHash::Sha256),
				QCryptographicHash::Sha256);
		}
	}

	QByteArray HmacSha256(const QByteArray& key, const QByteArray& data) {
		return Hmac(key, data, HASH_SHA256);
	}

	void MakeHttpParams(http::HttpClient & http_client, std::function<void(EncodedList &)> func) {
		const QUrl url(kUrlSearch);
		EncodedList args;

		args.push_back(qMakePair<QByteArray, QByteArray>("key", QByteArray::fromBase64(kAccessKeyB64)));
		args.push_back(qMakePair<QByteArray, QByteArray>("secret", QByteArray::fromBase64(kSecretKeyB64)));

		/*args.push_back(qMakePair<QByteArray, QByteArray>("type", "release"));
		args.push_back(qMakePair<QByteArray, QByteArray>("artist", artist.toLower()));
		args.push_back(qMakePair<QByteArray, QByteArray>("release_title", album.toLower()));*/
		func(args);

		for (auto& arg : args) {
			arg.first = QUrl::toPercentEncoding(arg.first);
			arg.second = QUrl::toPercentEncoding(arg.second);
		}

		QStringList query_items;
		for (auto& arg : args) {
			query_items << QString(arg.first + "=" + arg.second);
		}

		const QByteArray data_to_sign =
			QString("GET\n%1\n%2\n%3")
			.arg(url.host(), url.path(), query_items.join("&"))
			.toUtf8();
		const QByteArray signature(HmacSha256(
			QByteArray::fromBase64(kSecretKeyB64), data_to_sign));

		for (auto& arg : args) {
			http_client.param(arg.first, arg.second);
		}
		http_client.param(qTEXT("Signature"),
			QUrl::toPercentEncoding(signature.toBase64()));
	}
}

DiscogsCoverSearcher::DiscogsCoverSearcher(QObject* parent)
	: ICoverSearcher(parent) {
}

void DiscogsCoverSearcher::Search(const QString& artist, const QString& album, int id) {
	const QUrl url(kUrlSearch);
	http::HttpClient http_client(kUrlSearch);
	MakeHttpParams(http_client, [&artist, &album](auto &args) {
		args.push_back(qMakePair<QByteArray, QByteArray>("type", "release"));
		args.push_back(qMakePair<QByteArray, QByteArray>("artist", artist.toLower().toUtf8()));
		args.push_back(qMakePair<QByteArray, QByteArray>("release_title", album.toLower().toUtf8()));
		});
	http_client.success([id, this](const auto& url, const auto& content) {
		auto json_doc = QJsonDocument::fromJson(content.toUtf8());
		if ((json_doc.isNull()) || (!json_doc.isObject())) {
			return;
		}

		auto json_obj = json_doc.object();
		if (json_obj.isEmpty()) {
			return;
		}

		auto reply_map = json_obj.toVariantMap();
		if (!reply_map.contains("results")) {
			return;
		}

		auto results = reply_map["results"].toList();
		int i = 0;
		for (const QVariant& result : results) {
			auto result_map = result.toMap();
			if ((result_map.contains("id")) 
				&& (result_map.contains("resource_url"))) {
				int r_id = result_map["id"].toInt();
				auto title = result_map["title"].toString();
				auto resource_url = result_map["resource_url"].toString();
				if (resource_url.isEmpty()) continue;
				RequestRelease(r_id, resource_url);
			}
		}
		}).get();
}

void DiscogsCoverSearcher::RequestRelease(int32_t id, const QString& resource_url) {
	QUrl url(resource_url);
}
