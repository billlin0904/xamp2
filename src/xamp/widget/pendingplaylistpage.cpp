#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QVBoxLayout>

#include <widget/playlisttablemodel.h>
#include <widget/database.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/pendingplaylistpage.h>

PendingPlayTableView::PendingPlayTableView(QWidget* parent)
	: QTableView(parent) {
    model_ = new QSqlQueryModel(this);
    setModel(model_);

	setStyleSheet(qTEXT("border : none;"));
    setUpdatesEnabled(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setShowGrid(false);
    setMouseTracking(true);
    setAlternatingRowColors(false);

    setDragDropMode(InternalMove);
    setFrameShape(NoFrame);
    setFocusPolicy(Qt::NoFocus);

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(SelectRows);

    viewport()->setAttribute(Qt::WA_StaticContents);

    verticalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(46);

    horizontalScrollBar()->setDisabled(true);

    horizontalHeader()->setVisible(false);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // note: Fix QTableView select color issue.
    setFocusPolicy(Qt::StrongFocus);
}

void PendingPlayTableView::SetPlaylistId(int32_t playlist_id) {
	Reload();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("ID"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr("IS.PLAYING"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("#"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, tr("FILE.PATH"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("TITLE"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FILE.NAME"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, tr("FILE.SIZE"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("ALBUM"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("ARTIST"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("DURATION"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, tr("BIT.RATE"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SAMPLE.RATE"));
    model_->setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("RATING"));
    model_->setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, tr("ALBUM.RG"));
    model_->setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, tr("ALBUM.PK"));
    model_->setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, tr("LAST.UPDATE.TIME"));
    model_->setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, tr("TRACK.RG"));
    model_->setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, tr("TRACK.PK"));
    model_->setHeaderData(PLAYLIST_TRACK_LOUDNESS, Qt::Horizontal, tr("LOUDNESS"));
    model_->setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, tr("GENRE"));
    model_->setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, tr("YEAR"));
    model_->setHeaderData(PLAYLIST_ALBUM_ID, Qt::Horizontal, tr("ALBUM.ID"));
    model_->setHeaderData(PLAYLIST_PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("PLAYLIST.ID"));
    model_->setHeaderData(PLAYLIST_FILE_EXT, Qt::Horizontal, tr("FILE.EXT"));
    model_->setHeaderData(PLAYLIST_FILE_PARENT_PATH, Qt::Horizontal, tr("PARENT.PATH"));
    model_->setHeaderData(PLAYLIST_COVER_ID, Qt::Horizontal, tr(""));
    model_->setHeaderData(PLAYLIST_ARTIST_ID, Qt::Horizontal, tr("ARTIST.ID"));

    const QList<int> hidden_columns{
            PLAYLIST_MUSIC_ID,
            PLAYLIST_PLAYING,
            PLAYLIST_FILE_PATH,
            PLAYLIST_FILE_NAME,
            PLAYLIST_FILE_SIZE,
            PLAYLIST_ALBUM,
            PLAYLIST_DURATION,
            PLAYLIST_ARTIST,
            PLAYLIST_BIT_RATE,
            PLAYLIST_SAMPLE_RATE,
            PLAYLIST_RATING,
            PLAYLIST_ALBUM_RG,
            PLAYLIST_ALBUM_PK,
            PLAYLIST_LAST_UPDATE_TIME,
            PLAYLIST_TRACK_RG,
            PLAYLIST_TRACK_PK,
            PLAYLIST_TRACK_LOUDNESS,
            PLAYLIST_GENRE,
            PLAYLIST_YEAR,
            PLAYLIST_ALBUM_ID,
            PLAYLIST_ARTIST_ID,
            PLAYLIST_COVER_ID,
            PLAYLIST_FILE_EXT,
            PLAYLIST_FILE_PARENT_PATH,
            PLAYLIST_PLAYLIST_MUSIC_ID
    };

    for (auto column : qAsConst(hidden_columns)) {
        hideColumn(column);
    }
}

void PendingPlayTableView::Reload() {
    // 呼叫此函數就會更新index, 會導致playing index失效
    const QString s = qTEXT(R"(
SELECT
	albums.coverId,
	musics.musicId,
	playlistMusics.playing,
	musics.track,
	musics.path,
	musics.fileSize,
	musics.title,
	musics.fileName,
	artists.artist,
	albums.album,
	musics.duration,
	musics.bit_rate,
	musics.sample_rate,
	musics.rating,
	albumMusic.albumId,
	albumMusic.artistId,
	musics.fileExt,
	musics.parentPath,
	musics.dateTime,
	playlistMusics.playlistMusicsId,
	musics.album_replay_gain,
	musics.album_peak,
	musics.track_replay_gain,
	musics.track_peak,
	musicLoudness.track_loudness,
	musics.genre,
	musics.year 
FROM
	pendingPlaylist
	JOIN playlistMusics ON pendingPlaylist.playlistMusicsId = playlistMusics.playlistMusicsId
	JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
	LEFT JOIN musicLoudness ON playlistMusics.musicId = musicLoudness.musicId
	JOIN musics ON playlistMusics.musicId = musics.musicId
	JOIN albums ON albumMusic.albumId = albums.albumId
	JOIN artists ON albumMusic.artistId = artists.artistId 
WHERE
	playlistMusics.playlistId = %1
    )");
    const QSqlQuery query(s.arg(1), qDatabase.database());
    model_->setQuery(query);
    if (model_->lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_->lastError().text().toStdString());
    }
    model_->dataChanged(QModelIndex(), QModelIndex());
}

PendingPlaylistPage::PendingPlaylistPage(QWidget* parent)
	: QFrame(parent) {
    setFrameStyle(QFrame::StyledPanel);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(0, 0, 0, 0);

    playlist_ = new PendingPlayTableView(this);
    playlist_->SetPlaylistId(1);
    playlist_->setObjectName(QString::fromUtf8("tableView"));
    default_layout->addWidget(playlist_);

    default_layout->setContentsMargins(5, 5, 5, 5);
    setFixedSize(500, 300);
}
