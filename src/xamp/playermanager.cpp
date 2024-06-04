#include <playermanager.h>

void PlayerManager::playOrPause(QToolButton* playButton, QWidget* mainWindow, PlaylistPage* page, PlaylistTabWidget* tab) {
    try {
        switch (player_->GetState()) {
        case PlayerState::PLAYER_STATE_RUNNING:
            qTheme.setPlayOrPauseButton(playButton, false);
            player_->Pause();
            if (page) {
                page->playlist()->setNowPlayState(PlayingState::PLAY_PAUSE);
            }
            if (tab) {
                tab->setPlaylistTabIcon(qTheme.playlistPauseIcon(PlaylistTabWidget::kTabIconSize, 1.0));
            }
            mainWindow->setTaskbarPlayerPaused();
            break;
        case PlayerState::PLAYER_STATE_PAUSED:
            qTheme.setPlayOrPauseButton(playButton, true);
            player_->Resume();
            if (page) {
                page->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
            }
            if (tab) {
                tab->setPlaylistTabIcon(qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0));
            }
            mainWindow->setTaskbarPlayingResume();
            break;
        case PlayerState::PLAYER_STATE_STOPPED:
        case PlayerState::PLAYER_STATE_USER_STOPPED:
            if (!page || !page->playlist()->selectFirstItem()) return;
            play_index_ = page->playlist()->model()->index(play_index_.row(), PLAYLIST_IS_PLAYING);
            if (play_index_.row() == -1) {
                XMessageBox::showInformation(tr("Not found any playing item."));
                return;
            }
            page->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
            page->playlist()->setNowPlaying(play_index_);
            page->playlist()->onPlayIndex(play_index_);
            break;
        }
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
    }
}

void PlayerManager::stop() {
    player_->Stop(true, true);
    lrc_page_->spectrum()->reset();
    ui_.seekSlider->setEnabled(false);
    main_window_->resetTaskbarProgress();
    current_entity_ = std::nullopt;
    playlist_tab_page_->forEachPlaylist([](auto* playlist) {
        playlist->setNowPlayState(PlayingState::PLAY_CLEAR);
        });
    playlist_tab_page_->resetAllTabIcon();
    music_library_page_->album()->albumViewPage()->playlistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.setPlayOrPauseButton(ui_.playButton, true);
}

void PlayerManager::playNext() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    }
    else {
        stop();
    }
}

void PlayerManager::playPrevious() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(-1);
    }
    else {
        stop();
    }
}

void PlayerManager::setVolume(uint32_t volume) {
    ui_.mutedButton->onVolumeChanged(volume);
    ui_.mutedButton->showDialog();
}
