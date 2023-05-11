#include <widget/genre_view.h>

GenreView::GenreView(QWidget* parent)
	: AlbumView(parent) {
}

void GenreView::SetGenre(const QString& genre) {
    genre_ = genre;
}

void GenreView::ShowAllAlbum(int32_t limit) {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId,
    albums.year
FROM
    albums
LEFT JOIN artists ON artists.artistId = albums.artistId
WHERE
    (
    albums.genre LIKE '%%1%' OR albums.genre LIKE '%1,%' OR albums.genre LIKE '%,%1' OR albums.genre LIKE '%1'
    ) AND (albums.isPodcast = 0)
ORDER BY
    albums.album DESC
LIMIT %2
    )").arg(genre_.toLower()).arg(limit);
}

void GenreView::ShowAll() {
    ShowAllAlbum(5);
}