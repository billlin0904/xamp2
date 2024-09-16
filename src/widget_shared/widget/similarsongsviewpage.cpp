#include <QSplitter>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QCompleter>

#include <widget/database.h>
#include <widget/similarsongsviewpage.h>
#include <widget/playlisttableview.h>
#include <widget/dao/musicdao.h>
#include <widget/dao/playlistdao.h>
#include <thememanager.h>

SimilarSongViewPage::SimilarSongViewPage(QWidget* parent)
	: QFrame(parent) {
    auto *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName("verticalLayout");

    playlist_table_view_ = new PlaylistTableView();
    playlist_table_view_->setObjectName("listView");
    playlist_table_view_->setPlaylistId(kSimilarSongPlaylistId, kEmptyString);
    verticalLayout->addWidget(playlist_table_view_);
    setLayout(verticalLayout);
}

void SimilarSongViewPage::onQueryEmbeddingsReady(const QList<EmbeddingQueryResult>& results) {
	dao::PlaylistDao playlist_dao;
	QList<int32_t> music_ids;
    
    try {
        playlist_table_view_->removeAll();
        Q_FOREACH(const auto & result, results) {
            music_ids.append(result.audio_id.toInt());
        }
        playlist_dao.addMusicToPlaylist(music_ids, kSimilarSongPlaylistId);
	}
    catch (...) {
    }
    playlist_table_view_->reload(false, false);
}