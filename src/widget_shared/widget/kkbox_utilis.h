//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QList>

#include <optional>
#include <cstdint>

namespace kkbox {

struct ImageData {
    int32_t width;
    int32_t height;
    QString url;
};

struct ArtistData {
    QString id;
    QString name;
    QString url;
    QList<ImageData> images;
};

struct Credential {
    QString access_token;
    QString token_type;
    QString expires_in;
    QString refresh_token;
    QString scope;
};

std::optional<ArtistData> ParseArtistData(const QString& json, const QString& find_artist);

}

