#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QVBoxLayout>

#include "thememanager.h"
#include <widget/playlisttablemodel.h>
#include <widget/database.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/albumentity.h>
#include <widget/pendingplaylistpage.h>

static PlayListEntity GetEntity(const QModelIndex& index) {
    PlayListEntity entity;
    entity.music_id = GetIndexValue(index, PLAYLIST_MUSIC_ID).toInt();
    entity.playing = GetIndexValue(index, PLAYLIST_PLAYING).toInt();
    entity.track = GetIndexValue(index, PLAYLIST_TRACK).toUInt();
    entity.file_path = GetIndexValue(index, PLAYLIST_FILE_PATH).toString();
    entity.file_size = GetIndexValue(index, PLAYLIST_FILE_SIZE).toULongLong();
    entity.title = GetIndexValue(index, PLAYLIST_TITLE).toString();
    entity.file_name = GetIndexValue(index, PLAYLIST_FILE_NAME).toString();
    entity.artist = GetIndexValue(index, PLAYLIST_ARTIST).toString();
    entity.album = GetIndexValue(index, PLAYLIST_ALBUM).toString();
    entity.duration = GetIndexValue(index, PLAYLIST_DURATION).toDouble();
    entity.bit_rate = GetIndexValue(index, PLAYLIST_BIT_RATE).toUInt();
    entity.sample_rate = GetIndexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    entity.rating = GetIndexValue(index, PLAYLIST_RATING).toUInt();
    entity.album_id = GetIndexValue(index, PLAYLIST_ALBUM_ID).toInt();
    entity.artist_id = GetIndexValue(index, PLAYLIST_ARTIST_ID).toInt();
    entity.cover_id = GetIndexValue(index, PLAYLIST_COVER_ID).toString();
    entity.file_extension = GetIndexValue(index, PLAYLIST_FILE_EXT).toString();
    entity.parent_path = GetIndexValue(index, PLAYLIST_FILE_PARENT_PATH).toString();
    entity.timestamp = GetIndexValue(index, PLAYLIST_LAST_UPDATE_TIME).toULongLong();
    entity.playlist_music_id = GetIndexValue(index, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    entity.album_replay_gain = GetIndexValue(index, PLAYLIST_ALBUM_RG).toDouble();
    entity.album_peak = GetIndexValue(index, PLAYLIST_ALBUM_PK).toDouble();
    entity.track_replay_gain = GetIndexValue(index, PLAYLIST_TRACK_RG).toDouble();
    entity.track_peak = GetIndexValue(index, PLAYLIST_TRACK_PK).toDouble();
    entity.track_loudness = GetIndexValue(index, PLAYLIST_TRACK_LOUDNESS).toDouble();
    entity.genre = GetIndexValue(index, PLAYLIST_GENRE).toString();
    entity.year = GetIndexValue(index, PLAYLIST_YEAR).toUInt();
    return entity;
}

PendingPlayTableView::PendingPlayTableView(QWidget* parent)
	: QTableView(parent) {
    model_ = new QSqlQueryModel(this);
    setModel(model_);    
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

    switch (qTheme.GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qTEXT("background: #121212; border: none;"));
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qTEXT("background: #f9f9f9; border: none;"));
        break;
    }    

    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));
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
            PLAYLIST_TRACK,
            PLAYLIST_ALBUM,
            PLAYLIST_FILE_PATH,
            PLAYLIST_FILE_NAME,
            PLAYLIST_FILE_SIZE,
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

    auto* header = horizontalHeader();
    for (auto column = 0; column < header->count(); ++column) {
        switch (column) {
        case PLAYLIST_TITLE:
            header->resizeSection(column,
                (std::max)(sizeHintForColumn(column), 400));
            break;
        case PLAYLIST_ALBUM:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        default:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 80);
            break;
        }
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

PendingPlaylistPage::PendingPlaylistPage(const QList<QModelIndex>& indexes, QWidget* parent)
	: QFrame(parent)
    , indexes_(indexes) {
    setFrameStyle(QFrame::StyledPanel);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));

    playlist_ = new PendingPlayTableView(this);
    playlist_->SetPlaylistId(1);
    playlist_->setObjectName(QString::fromUtf8("tableView"));
    default_layout->addWidget(playlist_);

    default_layout->setContentsMargins(5, 5, 5, 5);
    setFixedSize(450, 300);    

    (void)QObject::connect(playlist_, &QTableView::doubleClicked, [this](const auto& index) {
        PlayMusic(indexes_[index.row()]);
        });
}
