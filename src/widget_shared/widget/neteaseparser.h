//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/base.h>
#include <widget/util/str_util.h>
#include <widget/ilrrcparser.h>

struct XAMP_WIDGET_SHARED_EXPORT NeteaseArtist {
	int id{ 0 };
	QString name;
	QString picUrl;
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseAlbum {
	int id{ 0 };	
	int size{ 0 };
	qint64 publishTime;	
	qint64 picId;
	QString name;
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseSong {
	int id{ 0 };
	int copyrightId{ 0 };
	int status{ 0 };
	qint64 duration{ 0 };
	QString name;
	QList<NeteaseArtist> artists;
	NeteaseAlbum album;	
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseLyricTransUser {
	int id{ 0 };
	int status{ 0 };
	int demand{ 0 };
	long long userid{ 0 };
	long long uptime{ 0 };
	QString nickname;	
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseLyricUser {
	int id{ 0 };
	int status{ 0 };
	int demand{ 0 };
	long long userid{ 0 };
	long long uptime{ 0 };
	QString nickname;
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseLyricSection {
	int version{ 0 };
	QString lyric;
};

struct XAMP_WIDGET_SHARED_EXPORT NeteaseLyricData {
	bool sgc{ false };
	bool sfy{ false };
	bool qfy{ false };
	int code{ 0 };
	NeteaseLyricTransUser transUser;
	NeteaseLyricUser lyricUser;
	NeteaseLyricSection lrc;
	NeteaseLyricSection klyric;
	NeteaseLyricSection tlyric;	
};

XAMP_WIDGET_SHARED_EXPORT std::optional<QList<NeteaseSong>> parseNeteaseSong(const QString& jsonString);

XAMP_WIDGET_SHARED_EXPORT std::optional<NeteaseLyricData> parseNeteaseLyric(const QString& jsonString);

