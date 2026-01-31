#include <widget/playbackqueueviewpage.h>
#include <widget/playlisttableview.h>
#include <widget/dao/playlistdao.h>
#include <widget/util/str_util.h>
#include <widget/scrolllabel.h>
#include <ui_playbackqueueviewpage.h>
#include <QPainter>
#include <thememanager.h>

PlaybackQueueViewPage::PlaybackQueueViewPage(QWidget* parent)
	: QDialog(parent)
	, ui_(new Ui::PlaybackQueueViewPage()) {
	ui_->setupUi(this);
	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
    ui_->label->setStyleSheet("background-color: transparent;"_str);
    ui_->label_2->setStyleSheet("background-color: transparent;"_str);
    ui_->label_3->setStyleSheet("background-color: transparent;"_str);

    qDaoFacade.playlist_dao.removePlaylistAllMusic(kNowPlayingPlaylistId);

    ui_->nowPlayingPlaylist->playlist()->setPlaylistId(kNowPlayingPlaylistId, ""_str);
    ui_->nextInQueuePlaylist->playlist()->setPlaylistId(kNextInQueuePlaylistId, ""_str);

    ui_->nowPlayingPlaylist->hidePlaybackInformation(true);
    ui_->nextInQueuePlaylist->hidePlaybackInformation(true);

    ui_->nowPlayingPlaylist->pageTitle()->hide();
    ui_->nextInQueuePlaylist->pageTitle()->hide();

    ui_->nowPlayingPlaylist->searchLineEdit()->hide();
    ui_->nextInQueuePlaylist->searchLineEdit()->hide();

    ui_->nowPlayingPlaylist->title()->hide();
    ui_->nextInQueuePlaylist->title()->hide();
	setFixedSize(600, 500);

    (void)QObject::connect(ui_->clearQueueButton, &QPushButton::clicked, [this]() {
        clearQueue();
        });

    (void)QObject::connect(ui_->nextInQueuePlaylist->playlist(), &PlaylistTableView::playMusic,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
        auto items = ui_->nowPlayingPlaylist->playlist()->items();
        if (!items.isEmpty()) {
            qDaoFacade.playlist_dao.removePlaylistAllMusic(kNowPlayingPlaylistId);
        }
        qDaoFacade.playlist_dao.addMusicToPlaylist(QList<int32_t>{item.music_id}, kNowPlayingPlaylistId);
        qDaoFacade.playlist_dao.removePlaylistMusic(QList<int32_t>{item.music_id}, kNextInQueuePlaylistId);
        ui_->nextInQueuePlaylist->reload();
        ui_->nowPlayingPlaylist->reload();
        emit playFile(item.file_path, false);
        });
}

PlaybackQueueViewPage::~PlaybackQueueViewPage() {
	delete ui_;
}

QString PlaybackQueueViewPage::popQueue() {
    auto now_items = ui_->nowPlayingPlaylist->playlist()->items();
    auto next_items = ui_->nextInQueuePlaylist->playlist()->items();

    auto reload_both = [&]() {
        ui_->nowPlayingPlaylist->playlist()->reload();
        ui_->nextInQueuePlaylist->playlist()->reload();
        };

    auto move_next_to_now = [&]() {
        if (next_items.isEmpty()) return false;

        const auto next_id = next_items.first().music_id;
        qDaoFacade.playlist_dao.removePlaylistMusic({ next_id }, kNextInQueuePlaylistId);
        qDaoFacade.playlist_dao.addMusicToPlaylist({ next_id }, kNowPlayingPlaylistId);
        return true;
        };

    // Case 1: NowPlaying 空 -> 直接把 Next 的第一首搬到 NowPlaying，並回傳它的 path
    if (now_items.isEmpty()) {
        if (!move_next_to_now()) return kEmptyString;
        reload_both();

        // 重新取一次 nowItems（或也可以直接用 nextItems.first() 的 file_path）
        now_items = ui_->nowPlayingPlaylist->playlist()->items();
        return now_items.isEmpty() ? kEmptyString : now_items.first().file_path;
    }

    // Case 2: NowPlaying 有歌 -> pop 掉 NowPlaying 第一首，並把 Next 第一首補上（如果有）
    const auto poppedPath = now_items.first().file_path;
    const auto poppedId = now_items.first().music_id;

    qDaoFacade.playlist_dao.removePlaylistMusic({ poppedId }, kNowPlayingPlaylistId);

    // Next 有就補，沒有就不補（但 pop 仍然成功）
    move_next_to_now();
    reload_both();

    return poppedPath;
}

void PlaybackQueueViewPage::clearQueue() {
    qDaoFacade.playlist_dao.removePlaylistAllMusic(kNextInQueuePlaylistId);
    ui_->nextInQueuePlaylist->playlist()->reload();
}

void PlaybackQueueViewPage::addQueue(const TrackInfo& track_info) {
    ui_->nowPlayingPlaylist->playlist()->reload();

    qDatabaseFacade.insertTrackInfo(std::forward_list<TrackInfo>{track_info}, kNextInQueuePlaylistId);
    ui_->nextInQueuePlaylist->playlist()->reload();
}

void PlaybackQueueViewPage::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        p.setPen(QColor(15, 15, 15));
        p.setBrush(QColor(31, 31, 31));
        break;
    case ThemeColor::LIGHT_THEME:
        p.setPen(QColor(190, 190, 190));
        p.setBrush(QColor(235, 235, 235));
        break;
    }
    p.drawRoundedRect(rect(), 10, 10);
}