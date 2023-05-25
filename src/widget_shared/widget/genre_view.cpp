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
    show_all_ = limit == kMaxShowAlbum;
}

void GenreView::ShowAll() {
    auto album_count = this->width() / kAlbumViewCoverSize;
    album_count = (std::max)(album_count, 6);
    ShowAllAlbum(album_count);
}

void GenreView::resizeEvent(QResizeEvent* event) {
    if (!show_all_) {
        auto album_count = this->width() / kAlbumViewCoverSize;
        ShowAllAlbum(album_count);
    }    
    AlbumView::resizeEvent(event);
    Update();
}