#include <widget/util/json_util.h>
#include <widget/widget_shared.h>
#include <base/stl.h>
#include <widget/neteaseparser.h>

std::optional<QList<NeteaseSong>> parseNeteaseSong(const QString& jsonString) {
	QJsonParseError json_error;
	auto parse_doucment = QJsonDocument::fromJson(jsonString.toUtf8(), &json_error);
	if (json_error.error != QJsonParseError::NoError) {
		return std::nullopt;
	}

    QJsonObject rootObj = parse_doucment.object();
    int code = rootObj.value("code"_str).toInt();
    QJsonObject resultObj = rootObj.value("result"_str).toObject();
    bool hasMore = resultObj.value("hasMore"_str).toBool();
    int songCount = resultObj.value("songCount"_str).toInt();
    QJsonArray songsArray = resultObj.value("songs"_str).toArray();

    QList<NeteaseSong> songs;
    songs.reserve(songsArray.size());

    for (auto songValue : songsArray) {
        if (!songValue.isObject()) continue;
        QJsonObject songObj = songValue.toObject();

        NeteaseSong song;
        song.id = songObj.value("id"_str).toInt();
        song.name = songObj.value("name"_str).toString();
        song.duration = songObj.value("duration"_str).toVariant().toLongLong();
        song.copyrightId = songObj.value("copyrightId"_str).toInt();
        song.status = songObj.value("status"_str).toInt();

        QJsonArray artistsArray = songObj.value("artists"_str).toArray();
        for (auto artistValue : artistsArray) {
            if (!artistValue.isObject()) continue;
            QJsonObject artistObj = artistValue.toObject();

            NeteaseArtist artist;
            artist.id = artistObj.value("id"_str).toInt();
            artist.name = artistObj.value("name"_str).toString();
            artist.picUrl = artistObj.value("picUrl"_str).toString();

            song.artists.push_back(artist);
        }

        QJsonObject albumObj = songObj.value("album"_str).toObject();
        song.album.id = albumObj.value("id"_str).toInt();
        song.album.name = albumObj.value("name"_str).toString();
        song.album.publishTime = albumObj.value("publishTime"_str).toVariant().toLongLong();
        song.album.size = albumObj.value("size"_str).toInt();
        song.album.picId = albumObj.value("picId"_str).toVariant().toLongLong();

        songs.push_back(song);
    }
    return MakeOptional<QList<NeteaseSong>>(std::move(songs));
}

std::optional<NeteaseLyricData> parseNeteaseLyric(const QString& jsonString) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
		return std::nullopt;
    }

    QJsonObject rootObj = doc.object();

    NeteaseLyricData result;

    result.sgc = rootObj.value("sgc"_str).toBool(false);
    result.sfy = rootObj.value("sfy"_str).toBool(false);
    result.qfy = rootObj.value("qfy"_str).toBool(false);

    result.code = rootObj.value("code"_str).toInt();

    QJsonObject transUserObj = rootObj.value("transUser"_str).toObject();
    result.transUser.id = transUserObj.value("id"_str).toInt();
    result.transUser.status = transUserObj.value("status"_str).toInt();
    result.transUser.demand = transUserObj.value("demand"_str).toInt();
    result.transUser.userid = transUserObj.value("userid"_str).toVariant().toLongLong();
    result.transUser.nickname = transUserObj.value("nickname"_str).toString();
    result.transUser.uptime = transUserObj.value("uptime"_str).toVariant().toLongLong();

    QJsonObject lyricUserObj = rootObj.value("lyricUser"_str).toObject();
    result.lyricUser.id = lyricUserObj.value("id"_str).toInt();
    result.lyricUser.status = lyricUserObj.value("status"_str).toInt();
    result.lyricUser.demand = lyricUserObj.value("demand"_str).toInt();
    result.lyricUser.userid = lyricUserObj.value("userid"_str).toVariant().toLongLong();
    result.lyricUser.nickname = lyricUserObj.value("nickname"_str).toString();
    result.lyricUser.uptime = lyricUserObj.value("uptime"_str).toVariant().toLongLong();

    QJsonObject lrcObj = rootObj.value("lrc"_str).toObject();
    result.lrc.version = lrcObj.value("version"_str).toInt();
    result.lrc.lyric = lrcObj.value("lyric"_str).toString();

    QJsonObject klyricObj = rootObj.value("klyric"_str).toObject();
    result.klyric.version = klyricObj.value("version"_str).toInt();
    result.klyric.lyric = klyricObj.value("lyric"_str).toString();

    QJsonObject tlyricObj = rootObj.value("tlyric"_str).toObject();
    result.tlyric.version = tlyricObj.value("version"_str).toInt();
    result.tlyric.lyric = tlyricObj.value("lyric"_str).toString();

    return MakeOptional<NeteaseLyricData>(std::move(result));
}
