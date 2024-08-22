#include <widget/genre_view.h>

GenreView::GenreView(QWidget* parent)
	: AlbumView(parent) {
}

void GenreView::setGenre(const QString& genre) {
    genre_ = genre;
}

void GenreView::showAllAlbum(int32_t limit) {
    last_query_ = qFormat(R"(
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
    )
ORDER BY
    albums.album DESC
LIMIT %2
    )").arg(genre_.toLower()).arg(limit);
    show_all_ = limit == kMaxShowAlbum;
}

void GenreView::showAll() {
    auto album_count = this->width() / kAlbumViewCoverSize;
    album_count = (std::max)(album_count, 6);
    showAllAlbum(album_count);
}

void GenreView::resizeEvent(QResizeEvent* event) {
    if (!show_all_) {
	    const auto album_count = this->width() / kAlbumViewCoverSize;
        showAllAlbum(album_count);
    }    
    AlbumView::resizeEvent(event);
    reload();
}