#include <widget/richplaylistpage.h>

#include <QAbstractTableModel>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QResizeEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPixmap>
#include <QSizePolicy>
#include <QScrollBar>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>

#include <thememanager.h>

#include <base/logger.h>
#include <base/rng.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/playerorder.h>
#include <widget/playlistentity.h>
#include <widget/scanfileprogresspage.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlisttableview.h>
#include <widget/util/image_util.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

namespace {
    constexpr auto kRichProgressPageHeight = 104;
    constexpr auto kRichCoverRadius = 8;

    QString richPlaylistQuery(int32_t playlist_id) {
        return qFormat(R"(
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
    musics.bitRate,
    musics.sampleRate,
    albumMusic.albumId,
    albumMusic.artistId,
    musics.fileExt,
    musics.parentPath,
    musics.dateTime,
    playlistMusics.playlistMusicsId,
    musics.albumReplayGain,
    musics.albumPeak,
    musics.trackReplayGain,
    musics.trackPeak,
    musicLoudness.trackLoudness,
    musics.genre,
    playlistMusics.isChecked,
    musics.heart,
    musics.duration,
    musics.comment,
    albums.year,
    musics.coverId as musicCoverId,
    musics.offset,
    musics.isCueFile,
    musics.isZipFile,
    musics.archiveEntryName
FROM
    playlistMusics
    JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
    JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
    LEFT JOIN musicLoudness ON playlistMusics.musicId = musicLoudness.musicId
    JOIN musics ON playlistMusics.musicId = musics.musicId
    JOIN albums ON albumMusic.albumId = albums.albumId
    JOIN artists ON albumMusic.artistId = artists.artistId
WHERE
    playlistMusics.playlistId = %1
ORDER BY
    playlistMusics.playlistMusicsId)").arg(playlist_id);
    }

    QString displayFileExt(QString file_ext) {
        file_ext.remove("."_str);
        return file_ext.isEmpty() ? "file"_str : file_ext;
    }
}

namespace {
    QString playerText(const char* text) { return QCoreApplication::translate("RichPlaylistPage", text); }
}

class RichPlaylistCoverPanel final : public QFrame {
public:
    explicit RichPlaylistCoverPanel(QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName("richPlaylistCoverPanel"_str);
        setFixedWidth(286);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(22, 24, 22, 24);
        layout->setSpacing(12);
        auto* heading = new QLabel(playerText("Now playing"), this);
        heading->setProperty("translationSource", QStringLiteral("Now playing"));
        heading->setObjectName("panelHeading"_str);
        layout->addWidget(heading);
        layout->addSpacing(8);
        cover_label_ = new QLabel(this);
        cover_label_->setFixedSize(242, 242);
        layout->addWidget(cover_label_);
        layout->addSpacing(4);
        auto* title_row = new QHBoxLayout();
        title_label_ = new QLabel(this);
        title_label_->setObjectName("nowPlayingTitle"_str);
        title_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        title_row->addWidget(title_label_, 1);
        heart_button_ = new QToolButton(this);
        heart_button_->setFixedSize(32, 32);
        heart_button_->setToolTip(playerText("Favorite"));
        heart_button_->setAccessibleName(playerText("Favorite"));
        title_row->addWidget(heart_button_);
        layout->addLayout(title_row);
        artist_label_ = new QLabel(this);
        artist_label_->setObjectName("secondaryText"_str);
        layout->addWidget(artist_label_);
        format_label_ = new QLabel(this);
        format_label_->setObjectName("audioFormat"_str);
        layout->addWidget(format_label_);
        layout->addSpacing(12);
        auto* separator = new QFrame(this);
        separator->setObjectName("panelDivider"_str);
        separator->setFixedHeight(1);
        layout->addWidget(separator);
        auto* queue_heading = new QLabel(playerText("Up next"), this);
        queue_heading->setProperty("translationSource", QStringLiteral("Up next"));
        queue_heading->setObjectName("panelHeading"_str);
        layout->addWidget(queue_heading);
        queue_hint_ = new QLabel(this);
        queue_hint_->setObjectName("secondaryText"_str);
        queue_hint_->setWordWrap(true);
        layout->addWidget(queue_hint_);
        for (int i = 0; i < 3; ++i) {
            auto* button = new QToolButton(this);
            button->setObjectName("queueTrack"_str);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setIconSize(QSize(38, 38));
            button->setMinimumHeight(56);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            layout->addWidget(button);
            queue_buttons_.append(button);
            connect(button, &QToolButton::clicked, this, [this, i] {
                if (i < queue_.size() && play_requested) play_requested(queue_[i].playlist_music_id);
            });
        }
        layout->addStretch();
        connect(heart_button_, &QToolButton::clicked, this, [this] {
            if (current_.music_id <= 0) return;
            if (favorite_requested) favorite_requested(!current_.heart);
        });
        connect(&qTheme, &ThemeManager::themeChangedFinished, this, [this] { updateHeart(); });
        clearNowPlaying();
    }
    void setNowPlaying(const PlayListEntity& track_info, const QPixmap& cover) {
        has_now_playing_ = true;
        const auto title = track_info.title;
        title_label_->setText(title_label_->fontMetrics().elidedText(title, Qt::ElideRight, 200));
        title_label_->setToolTip(title);
        const auto artist = track_info.artist;
        artist_label_->setText(artist_label_->fontMetrics().elidedText(artist, Qt::ElideRight, 242));
        artist_label_->setToolTip(artist);
        const auto ext = track_info.file_extension;
        format_label_->setText(ext.toUpper() + QStringLiteral("  ·  ") + formatDuration(track_info.duration));
        setCover(cover.isNull() ? qTheme.unknownCover() : cover);
    }
    void setCurrentEntity(const PlayListEntity& entity) { current_ = entity; updateHeart(); }
    void clearNowPlaying() {
        has_now_playing_ = false;
        current_ = {};
        title_label_->setText(playerText("Nothing playing"));
        artist_label_->setText(playerText("Double-click a track to play"));
        format_label_->clear();
        setCover(qTheme.unknownCover());
        updateHeart();
    }
    void setQueue(const QList<PlayListEntity>& tracks, bool shuffle) {
        heart_button_->setToolTip(playerText("Favorite"));
        heart_button_->setAccessibleName(playerText("Favorite"));
        if (!has_now_playing_) {
            title_label_->setText(playerText("Nothing playing"));
            artist_label_->setText(playerText("Double-click a track to play"));
        }
        queue_ = tracks;
        queue_hint_->setText(shuffle ? playerText("Shuffle is on") : playerText("End of playlist"));
        queue_hint_->setVisible(tracks.isEmpty());
        for (int i = 0; i < queue_buttons_.size(); ++i) {
            auto* button = queue_buttons_[i];
            button->setVisible(i < tracks.size());
            if (i >= tracks.size()) continue;
            const auto& track = tracks[i];
            const auto title = button->fontMetrics().elidedText(track.title, Qt::ElideRight, 168);
            const auto artist = button->fontMetrics().elidedText(track.artist, Qt::ElideRight, 168);
            button->setText((title + QStringLiteral("\n") + artist).replace("&"_str, "&&"_str));
            button->setToolTip(track.title + QStringLiteral(" — ") + track.artist);
            button->setIcon(QIcon(qImageCache.getOrAddDefault(track.validCoverId())));
        }
    }
    std::function<void(int32_t)> play_requested;
    std::function<void(bool)> favorite_requested;
private:
    void updateHeart() {
        heart_button_->setEnabled(current_.music_id > 0);
        heart_button_->setIcon(qTheme.fontIcon(current_.heart ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART));
    }
    void setCover(const QPixmap& source) {
        QPixmap cover(242, 242); cover.fill(Qt::transparent);
        QPainter painter(&cover); painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path; path.addRoundedRect(cover.rect(), 6, 6); painter.setClipPath(path);
        const auto scaled = source.scaled(242, 242, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawPixmap((242 - scaled.width()) / 2, (242 - scaled.height()) / 2, scaled);
        cover_label_->setPixmap(cover);
    }
    QLabel* cover_label_{};
    QLabel* title_label_{};
    QLabel* artist_label_{};
    QLabel* format_label_{};
    QLabel* queue_hint_{};
    QToolButton* heart_button_{};
    QList<QToolButton*> queue_buttons_;
    QList<PlayListEntity> queue_;
    PlayListEntity current_;
    bool has_now_playing_{ false };
};

class RichPlaylistModel final : public QAbstractTableModel {
public:
    explicit RichPlaylistModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent), source_model_(new QSqlQueryModel(this)) {}
    void reload(int32_t playlist_id, const QString& keyword) {
        beginResetModel();
        playlist_id_ = playlist_id;
        source_model_->setQuery(QSqlQuery(richPlaylistQuery(playlist_id), qGuiDb.database()));
        while (source_model_->canFetchMore()) source_model_->fetchMore();
        rows_.clear();
        missing_album_cover_ids_.clear();
        for (int row = 0; row < source_model_->rowCount(); ++row) {
            bool matches = keyword.trimmed().isEmpty();
            for (int column : {PLAYLIST_TITLE, PLAYLIST_ARTIST, PLAYLIST_ALBUM, PLAYLIST_FILE_NAME})
                matches |= source_model_->index(row, column).data().toString().contains(keyword.trimmed(), Qt::CaseInsensitive);
            if (!matches) continue;
            rows_.push_back(row);
            const int album = source_model_->index(row, PLAYLIST_ALBUM_ID).data().toInt();
            const auto cover = source_model_->index(row, PLAYLIST_ALBUM_COVER_ID).data().toString();
            if (album > 0 && (cover.isEmpty() || cover == qImageCache.unknownCoverId()
                || (!qImageCache.contains(cover) && !qImageCache.isFileExists(cover))))
                missing_album_cover_ids_.insert(album);
        }
        endResetModel();
    }
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : rows_.size(); }
    int columnCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : PLAYLIST_MAX_COLUMN; }
    void applyPlayback(const PlaybackSnapshot& state) {
        playback_ = state;
        if (rowCount()) emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= rows_.size()) return {};
        if (index.column() == PLAYLIST_IS_PLAYING && role == Qt::DisplayRole)
            return !playback_.matches(playlist_id_, source_model_->index(rows_[index.row()], PLAYLIST_PLAYLIST_MUSIC_ID).data().toInt())
                ? PlayingState::PLAY_CLEAR : playback_.status == PlaybackStatus::Paused ? PlayingState::PLAY_PAUSE : PlayingState::PLAY_PLAYING;
        if (role == PLAYLIST_ROW_TYPE_ROLE) return PLAYLIST_ROW_TRACK;
        if (role == Qt::ToolTipRole) return source_model_->index(rows_[index.row()], index.column()).data();
        return source_model_->index(rows_[index.row()], index.column()).data(role);
    }
    QVariant headerData(int column, Qt::Orientation orientation, int role) const override {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
        switch (column) {
        case PLAYLIST_TRACK: return QStringLiteral("#");
        case PLAYLIST_TITLE: return QCoreApplication::translate("RichPlaylistPage", "Title");
        case PLAYLIST_ARTIST: return QCoreApplication::translate("RichPlaylistPage", "Artist");
        case PLAYLIST_ALBUM: return QCoreApplication::translate("RichPlaylistPage", "Album");
        case PLAYLIST_DURATION: return QCoreApplication::translate("RichPlaylistPage", "Time");
        default: return {};
        }
    }
    Qt::ItemFlags flags(const QModelIndex& index) const override {
        return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags;
    }
    bool isTrackRow(const QModelIndex& index) const { return index.isValid() && index.row() < rows_.size(); }
    PlayListEntity entity(const QModelIndex& index) const { return getEntity(index); }
    int32_t missingAlbumCoverId(int row) const {
        const auto id = data(index(row, PLAYLIST_ALBUM_ID)).toInt();
        return missing_album_cover_ids_.contains(id) ? id : kInvalidDatabaseId;
    }
private:
    PlaybackSnapshot playback_;
    int playlist_id_{-1};
    QSqlQueryModel* source_model_;
    QVector<int> rows_;
    QSet<int32_t> missing_album_cover_ids_;
};

class RichPlaylistStyledItemDelegate final : public QStyledItemDelegate {
public:
    explicit RichPlaylistStyledItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
        const auto rect = option.rect;
        const bool playing = index.siblingAtColumn(PLAYLIST_IS_PLAYING).data().toInt() != PlayingState::PLAY_CLEAR;
        const bool selected = option.state & QStyle::State_Selected;
        const auto* view = qobject_cast<const QTableView*>(option.widget);
        const bool hovered = view && view->indexAt(view->viewport()->mapFromGlobal(QCursor::pos())).row() == index.row();
        const bool backdrop = option.widget
            && option.widget->window()->property("backdropActive").toBool();
        if (playing || selected || hovered || !backdrop) {
            auto background = playing || selected ? qTheme.highlightColor()
                : hovered ? qTheme.hoverColor() : qTheme.backgroundColor();
            if (backdrop) background.setAlpha(playing || selected ? 200 : 110);
            painter->fillRect(rect, background);
        }
        auto font = qTheme.defaultFont();
        font.setPointSize(qTheme.fontSize(10));
        painter->setFont(font);
        const auto secondary = qTheme.isDarkTheme() ? QColor("#A2AEAA"_str) : QColor("#596661"_str);
        painter->setPen(playing ? qTheme.indicatorColor()
            : index.column() == PLAYLIST_TITLE ? qTheme.textColor() : secondary);
        auto text_rect = rect.adjusted(12, 0, -12, 0);
        if (index.column() == PLAYLIST_TRACK) {
            if (playing) qTheme.fontIcon(Glyphs::ICON_PLAYING).paint(painter, rect.adjusted(14, 16, -14, -16));
            else painter->drawText(rect, Qt::AlignCenter, QString::number(index.row() + 1));
        } else if (index.column() == PLAYLIST_TITLE) {
            const auto entity = getEntity(index);
            const auto cover = qImageCache.getOrAddDefault(entity.validCoverId());
            const QRect cover_rect(rect.left() + 8, rect.top() + 8, 36, 36);
            QPainterPath clip; clip.addRoundedRect(cover_rect, 4, 4);
            painter->save(); painter->setClipPath(clip);
            painter->drawPixmap(cover_rect, cover.scaled(cover_rect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            painter->restore();
            text_rect.setLeft(cover_rect.right() + 14);
            painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics(font).elidedText(entity.title.isEmpty() ? entity.file_name : entity.title, Qt::ElideRight, text_rect.width()));
        } else {
            const auto text = index.column() == PLAYLIST_DURATION ? formatDuration(index.data().toDouble()) : index.data().toString();
            painter->drawText(text_rect, (index.column() == PLAYLIST_DURATION ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter,
                QFontMetrics(font).elidedText(text, Qt::ElideRight, text_rect.width()));
        }
        painter->restore();
    }
};

class RichPlaylistView final : public QTableView {
public:
    explicit RichPlaylistView(QWidget* parent = nullptr)
        : QTableView(parent)
        , model_(new RichPlaylistModel(this)) {
        setObjectName("richPlaylistTableView"_str);
        setProperty("playlistStyle", QStringLiteral("modern"));
        setModel(model_);
        setItemDelegate(new RichPlaylistStyledItemDelegate(this));
        setShowGrid(false);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        horizontalHeader()->show();
        horizontalHeader()->setFixedHeight(40);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        verticalHeader()->hide();
        verticalHeader()->setDefaultSectionSize(PlaylistTableView::kColumnHeight);
        configureColumns();

        (void)QObject::connect(verticalScrollBar(),
            &QScrollBar::valueChanged,
            this,
            [this] {
                scheduleVisibleAlbumCoverRequest();
            });
    }

    void applyPlayback(const PlaybackSnapshot& state) { model_->applyPlayback(state); }
    QList<PlayListEntity> tracks() const {
        QList<PlayListEntity> result;
        for (int row = 0; row < model_->rowCount(); ++row) result.append(item(model_->index(row, 0)));
        return result;
    }
    void reload(int32_t playlist_id, bool keep_scroll = false) {
        const auto scroll_value = keep_scroll ? verticalScrollBar()->value() : 0;
        playlist_id_ = playlist_id;
        model_->reload(playlist_id_, search_text_);
        album_songs_id_cache_.clear();
        updateRowLayout();
        if (keep_scroll) {
            verticalScrollBar()->setValue(scroll_value);
        }
        scheduleVisibleAlbumCoverRequest();
    }

    PlayListEntity firstTrack() const { return firstIndex().isValid() ? item(firstIndex()) : PlayListEntity{}; }
    int trackCount() const { return model_->rowCount(); }
    double totalDuration() const {
        double total = 0;
        for (int row = 0; row < model_->rowCount(); ++row) total += model_->index(row, PLAYLIST_DURATION).data().toDouble();
        return total;
    }
    void playFirstOrRandom(bool random, std::function<void(const QModelIndex&)> callback) {
        if (!model_->rowCount()) return;
        const int row = random ? rng_.nextInt32(0, model_->rowCount() - 1) : 0;
        callback(model_->index(row, PLAYLIST_TITLE));
    }


    QSet<int32_t> visibleMissingAlbumCoverIds() const {
        QSet<int32_t> album_ids;
        const auto viewport_rect = viewport()->rect();
        if (viewport_rect.isEmpty()) {
            return album_ids;
        }

        for (auto row = 0; row < model_->rowCount(); ++row) {
            const auto index = model_->index(row, PLAYLIST_TRACK);
            const auto row_rect = visualRect(index);
            if (!row_rect.isValid() || !viewport_rect.intersects(row_rect)) {
                continue;
            }

            const auto album_id = model_->missingAlbumCoverId(row);
            if (album_id > 0) {
                album_ids.insert(album_id);
            }
        }
        return album_ids;
    }

    void setVisibleAlbumCoverRequestCallback(std::function<void()>&& callback) {
        visible_album_cover_request_callback_ = std::move(callback);
    }

    void search(const QString& keyword) {
        search_text_ = keyword;
        model_->reload(playlist_id_, search_text_);
        album_songs_id_cache_.clear();
        updateRowLayout();
        scheduleVisibleAlbumCoverRequest();
    }

    bool isTrackRow(const QModelIndex& index) const {
        return model_->isTrackRow(index);
    }

    PlayListEntity item(const QModelIndex& index) const {
        return model_->entity(index);
    }

    QModelIndex firstIndex() const {
        return model_->rowCount() > 0 ? model_->index(0, PLAYLIST_IS_PLAYING) : QModelIndex();
    }

    QModelIndex nextIndex(int32_t forward) const {
        const auto count = model_->rowCount();
        if (count == 0) {
            return {};
        }

        auto current_row = playingRow();
        if (current_row < 0 && currentIndex().isValid()) current_row = currentIndex().row();
        if (current_row < 0) {
            return model_->index(forward < 0 ? count - 1 : 0, PLAYLIST_IS_PLAYING);
        }

        current_row = (current_row + forward + count) % count;
        return model_->index(current_row, PLAYLIST_IS_PLAYING);
    }

    QModelIndex playOrderIndex(PlayerOrder order, int32_t forward) {
        QModelIndex index;
        switch (order) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
            index = nextIndex(forward);
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
            index = playingIndex();
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM:
            index = shuffleAlbumIndex();
            break;
        default:
            break;
        }

        if (!index.isValid()) {
            index = firstIndex();
        }
        return index;
    }

    QModelIndex indexForPlaylistMusicId(int32_t playlist_music_id) const {
        for (auto row = 0; row < model_->rowCount(); ++row) {
            const auto index = model_->index(row, PLAYLIST_PLAYLIST_MUSIC_ID);
            if (index.data().toInt() == playlist_music_id) {
                return index;
            }
        }
        return {};
    }

    bool scrollToPlayingTrack() {
        auto index = playingIndex();
        if (!index.isValid()) {
            return false;
        }

        setCurrentIndex(index);
        scrollTo(index, QAbstractItemView::PositionAtCenter);
        return true;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QTableView::paintEvent(event);
        if (model_->rowCount() == 0) {
            QPainter painter(viewport());
            painter.setPen(qTheme.textColor());
            painter.setFont(qTheme.defaultFont());
            painter.drawText(viewport()->rect().adjusted(24, 24, -24, -24),
                Qt::AlignCenter | Qt::TextWordWrap,
                search_text_.isEmpty()
                    ? QCoreApplication::translate("RichPlaylistPage", "Add local music with the + button")
                    : QCoreApplication::translate("RichPlaylistPage", "No matching tracks"));
        }
        scheduleVisibleAlbumCoverRequest();
    }

    void resizeEvent(QResizeEvent* event) override {
        QTableView::resizeEvent(event);
        configureColumns();
        scheduleVisibleAlbumCoverRequest();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        QTableView::mouseMoveEvent(event);
        viewport()->update();
    }

    void leaveEvent(QEvent* event) override {
        QTableView::leaveEvent(event);
        viewport()->update();
    }

private:
    void scheduleVisibleAlbumCoverRequest() {
        if (visible_album_cover_request_pending_) {
            return;
        }

        visible_album_cover_request_pending_ = true;
        QTimer::singleShot(0, this, [this] {
            visible_album_cover_request_pending_ = false;
            if (visible_album_cover_request_callback_) {
                visible_album_cover_request_callback_();
            }
        });
    }

    void configureColumns() {
        for (int column = 0; column < PLAYLIST_MAX_COLUMN; ++column) setColumnHidden(column, true);
        for (int column : {PLAYLIST_TRACK, PLAYLIST_TITLE, PLAYLIST_ARTIST, PLAYLIST_ALBUM, PLAYLIST_DURATION})
            setColumnHidden(column, false);
        horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        setColumnWidth(PLAYLIST_TRACK, 44);
        horizontalHeader()->setSectionResizeMode(PLAYLIST_TITLE, QHeaderView::Stretch);
        setColumnWidth(PLAYLIST_ARTIST, qBound(100, viewport()->width() / 5, 190));
        setColumnWidth(PLAYLIST_ALBUM, qBound(110, viewport()->width() / 5, 210));
        setColumnWidth(PLAYLIST_DURATION, 72);
        setColumnHidden(PLAYLIST_ALBUM, viewport()->width() < 650);
    }

    void updateRowLayout() {
        verticalHeader()->setDefaultSectionSize(52);
    }

    int playingRow() const {
        for (auto row = 0; row < model_->rowCount(); ++row) {
            const auto playing = model_->index(row, PLAYLIST_IS_PLAYING).data().toInt();
            if (playing != PlayingState::PLAY_CLEAR) {
                return row;
            }
        }
        return -1;
    }

    QModelIndex playingIndex() const {
        const auto row = playingRow();
        return row >= 0 ? model_->index(row, PLAYLIST_IS_PLAYING) : QModelIndex();
    }

    QModelIndex shuffleAlbumIndex() {
        const auto current_index = playingIndex();
        const auto count = model_->rowCount();
        if (count == 0 || !current_index.isValid()) {
            return {};
        }

        const auto current_album_id = model_->index(current_index.row(), PLAYLIST_ALBUM_ID).data().toInt();
        const auto current_playlist_music_id = model_->index(current_index.row(), PLAYLIST_PLAYLIST_MUSIC_ID).data().toInt();
        if (current_album_id == 0) {
            return {};
        }

        rng_.setSeed(current_album_id);

        if (album_songs_id_cache_.isEmpty()) {
            for (auto row = 0; row < count; ++row) {
                const auto album_id = model_->index(row, PLAYLIST_ALBUM_ID).data().toInt();
                if (album_id != 0) {
                    album_songs_id_cache_[album_id].append(row);
                }
            }
        }

        const auto album_ids = album_songs_id_cache_.keys();
        auto selected_album_id = current_album_id;
        if (album_ids.size() > 1) {
            do {
                const auto selected_album_index = rng_.nextInt32(0, album_ids.size() - 1);
                selected_album_id = album_ids[selected_album_index];
            } while (selected_album_id == current_album_id);
        }

        if (current_playlist_music_id != 0) {
            rng_.setSeed(current_playlist_music_id);
        }

        const auto& selected_album_songs = album_songs_id_cache_[selected_album_id];
        if (selected_album_songs.isEmpty()) {
            return {};
        }

        const auto selected_song_index = rng_.nextInt32(0, selected_album_songs.size() - 1);
        return model_->index(selected_album_songs[selected_song_index], PLAYLIST_IS_PLAYING);
    }

    RichPlaylistModel* model_{ nullptr };
    int32_t playlist_id_{ kDefaultPlaylistId };
    QString search_text_;
    PRNG rng_;
    QHash<int32_t, QList<int32_t>> album_songs_id_cache_;
    std::function<void()> visible_album_cover_request_callback_;
    bool visible_album_cover_request_pending_{ false };
};

RichPlaylistPage::RichPlaylistPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName("richPlaylistPage"_str);
    initial();
}

void RichPlaylistPage::reload() {
    rich_playlist_view_->reload(playlist_id_);
    rich_playlist_view_->scrollToPlayingTrack();
    requestMissingAlbumCovers();
}

void RichPlaylistPage::showImportMenu(const QPoint& pos) {
    QMenu menu(this);

    auto* load_file_act = menu.addAction(tr("load local file"));
    auto* load_dir_act = menu.addAction(tr("load file directory"));
    menu.addSeparator();
    auto* clear_all_act = menu.addAction(tr("Clear all"));

    const auto* selected_action = menu.exec(pos);
    if (selected_action == load_file_act) {
        loadLocalFile();
    }
    else if (selected_action == load_dir_act) {
        loadFileDirectory();
    }
    else if (selected_action == clear_all_act) {
        clearAll();
    }
}

void RichPlaylistPage::showPlaylistContextMenu(const QPoint& pos) {
    const auto index = rich_playlist_view_->indexAt(pos);
    const auto is_track_row = index.isValid() && rich_playlist_view_->isTrackRow(index);

    PlayListEntity entity;
    QMenu menu(this);
    QAction* copy_artist_act = nullptr;
    QAction* copy_album_act = nullptr;
    QAction* copy_title_act = nullptr;
    QAction* open_parent_path_act = nullptr;

    if (is_track_row) {
        entity = rich_playlist_view_->item(index);
        copy_artist_act = menu.addAction(qTheme.fontIcon(Glyphs::ICON_COPY), tr("Copy artist"));
        copy_album_act = menu.addAction(tr("Copy album"));
        copy_title_act = menu.addAction(tr("Copy title"));
        menu.addSeparator();
        open_parent_path_act = menu.addAction(tr("open file location"));
        menu.addSeparator();
    }

    auto* load_file_act = menu.addAction(tr("load local file"));
    auto* load_dir_act = menu.addAction(tr("load file directory"));
    menu.addSeparator();
    auto* clear_all_act = menu.addAction(tr("Clear all"));

    const auto* selected_action = menu.exec(rich_playlist_view_->mapToGlobal(pos));
    if (selected_action == nullptr) {
        return;
    }

    if (selected_action == copy_artist_act) {
        QApplication::clipboard()->setText(entity.artist);
    }
    else if (selected_action == copy_album_act) {
        QApplication::clipboard()->setText(entity.album);
    }
    else if (selected_action == copy_title_act) {
        QApplication::clipboard()->setText(entity.title);
    }
    else if (selected_action == open_parent_path_act) {
        const auto parent_path = entity.parent_path.isEmpty()
            ? QFileInfo(entity.file_path).absolutePath()
            : entity.parent_path;
        if (!parent_path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(parent_path));
        }
    }
    else if (selected_action == load_file_act) {
        loadLocalFile();
    }
    else if (selected_action == load_dir_act) {
        loadFileDirectory();
    }
    else if (selected_action == clear_all_act) {
        clearAll();
    }
}

void RichPlaylistPage::loadLocalFile() {
    getOpenMusicFileName(this, tr("open file"), tr("Music Files "), [this](const auto& file_name) {
        showProgressPage();
        emit extractFile(file_name, playlist_id_);
    });
}

void RichPlaylistPage::loadFileDirectory() {
    const auto dir_name = getExistingDirectory(this, tr("Select a directory"));
    if (dir_name.isEmpty()) {
        return;
    }
    loadPath(dir_name, true);
}

void RichPlaylistPage::loadPath(const QString& file_path, bool append_to_playlist) {
    if (file_path.isEmpty()) {
        return;
    }
    if (!append_to_playlist) {
        qDaoFacade.playlist_dao.removePlaylistAllMusic(playlist_id_);
        requested_album_cover_ids_.clear();
        reload();
    }
    showProgressPage();
    emit extractFile(file_path, playlist_id_);
}

void RichPlaylistPage::clearAll() {
    qDaoFacade.playlist_dao.removePlaylistAllMusic(playlist_id_);
    requested_album_cover_ids_.clear();
    reload();
}

ScanFileProgressPage* RichPlaylistPage::progressPage() const {
    return progress_page_;
}

void RichPlaylistPage::showProgressPage() {
    progress_page_->setFixedHeight(kRichProgressPageHeight);
    progress_page_->show();
}

bool RichPlaylistPage::playNextItem(int32_t forward) {
    const auto order = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);
    const auto index = rich_playlist_view_->playOrderIndex(order, forward);
    if (!index.isValid()) {
        return false;
    }
    playIndex(index, true);
    return true;
}

void RichPlaylistPage::playIndex(const QModelIndex& index, bool is_play) {
    if (!rich_playlist_view_->isTrackRow(index)) {
        return;
    }

    const auto entity = rich_playlist_view_->item(index);
    const auto current_index = rich_playlist_view_->indexForPlaylistMusicId(entity.playlist_music_id);
    if (current_index.isValid()) {
        rich_playlist_view_->setCurrentIndex(current_index);
        rich_playlist_view_->scrollTo(current_index, QAbstractItemView::PositionAtCenter);
    }

    emit playMusic(playlist_id_, entity, is_play);
}

void RichPlaylistPage::initial() {
    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    auto* content_panel = new QFrame(this);
    content_panel->setObjectName("richPlaylistContentPanel"_str);
    content_panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* main_layout = new QHBoxLayout(content_panel);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    cover_panel_ = new RichPlaylistCoverPanel(this);
    cover_panel_->setVisible(queue_visible_);

    auto* list_panel = new QFrame(this);
    list_panel->setObjectName("richPlaylistListPanel"_str);
    list_panel->setMinimumWidth(460);
    list_panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* list_layout = new QVBoxLayout(list_panel);
    list_layout->setContentsMargins(28, 22, 28, 0);
    list_layout->setSpacing(0);

    auto* breadcrumb = new QLabel(tr("Library / Playlists"), list_panel);
        breadcrumb->setProperty("translationSource", QStringLiteral("Library / Playlists"));
    breadcrumb->setObjectName("playlistBreadcrumb"_str);
    list_layout->addWidget(breadcrumb);
    auto* hero = new QFrame(list_panel);
    hero->setObjectName("playlistHero"_str);
    auto* hero_layout = new QHBoxLayout(hero);
    hero_layout->setContentsMargins(0, 20, 0, 28);
    hero_layout->setSpacing(24);
    playlist_cover_ = new QLabel(hero);
    playlist_cover_->setObjectName("playlistCover"_str);
    playlist_cover_->setAlignment(Qt::AlignCenter);
    playlist_cover_->setFixedSize(150, 150);
    hero_layout->addWidget(playlist_cover_);
    auto* details = new QVBoxLayout();
    details->setSpacing(8);
    auto* eyebrow = new QLabel(tr("Playlist"), hero);
        eyebrow->setProperty("translationSource", QStringLiteral("Playlist"));
    eyebrow->setObjectName("secondaryText"_str);
    details->addWidget(eyebrow);
    playlist_title_ = new QLabel(tr("My music"), hero);
    playlist_title_->setObjectName("playlistTitle"_str);
    playlist_title_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    details->addWidget(playlist_title_);
    playlist_stats_ = new QLabel(hero);
    playlist_stats_->setObjectName("secondaryText"_str);
    details->addWidget(playlist_stats_);
    details->addSpacing(6);
    auto* actions = new QHBoxLayout();
    actions->setSpacing(10);
    play_button_ = new QPushButton(tr("Play"), hero);
    play_button_->setObjectName("primaryPlay"_str);
    play_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY));
    play_button_->setMinimumHeight(36);
    shuffle_button_ = new QPushButton(tr("Shuffle"), hero);
    shuffle_button_->setMinimumHeight(36);
    shuffle_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
    auto* more = new QToolButton(hero);
    more->setIcon(qTheme.fontIcon(Glyphs::ICON_MORE));
    more->setFixedSize(36, 36);
    more->setToolTip(tr("Playlist options"));
    actions->addWidget(play_button_);
    actions->addWidget(shuffle_button_);
    actions->addWidget(more);
    actions->addStretch();
    details->addLayout(actions);
    hero_layout->addLayout(details, 1);
    list_layout->addWidget(hero);
    rich_playlist_view_ = new RichPlaylistView(list_panel);
    list_layout->addWidget(rich_playlist_view_, 1);
    connect(play_button_, &QPushButton::clicked, this, [this] {
        qAppSettings.setEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
        rich_playlist_view_->playFirstOrRandom(false, [this](const QModelIndex& index) { playIndex(index, true); });
        emit playOrderChanged();
    });
    connect(shuffle_button_, &QPushButton::clicked, this, [this] {
        qAppSettings.setEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM);
        rich_playlist_view_->playFirstOrRandom(true, [this](const QModelIndex& index) { playIndex(index, true); });
        emit playOrderChanged();
    });
    connect(more, &QToolButton::clicked, this, [this, more] {
        showImportMenu(more->mapToGlobal(QPoint(0, more->height())));
    });
    connect(rich_playlist_view_->model(), &QAbstractItemModel::modelReset, this, [this] { refreshPresentation(); });
    cover_panel_->play_requested = [this](int32_t id) { emit playQueuedTrack(id); };
    cover_panel_->favorite_requested = [this](bool favorite) { emit favoriteRequested(favorite); };

    progress_page_ = new ScanFileProgressPage(this);
    progress_page_->setFixedHeight(kRichProgressPageHeight);
    progress_page_->hide();

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this,
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint& pos) {
            showImportMenu(mapToGlobal(pos));
        });

    cover_panel_->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(cover_panel_,
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint& pos) {
            showImportMenu(cover_panel_->mapToGlobal(pos));
        });

    rich_playlist_view_->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(rich_playlist_view_,
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint& pos) {
            showPlaylistContextMenu(pos);
        });

    (void)QObject::connect(rich_playlist_view_,
        &QTableView::doubleClicked,
        this,
        [this](const QModelIndex& index) {
            playIndex(index, true);
        });

    rich_playlist_view_->setVisibleAlbumCoverRequestCallback([this] {
        requestMissingAlbumCovers();
    });

    main_layout->addWidget(list_panel, 1);
    main_layout->addWidget(cover_panel_);
    root_layout->addWidget(content_panel, 1);
    root_layout->addWidget(progress_page_);

    connect(&qTheme, &ThemeManager::themeChangedFinished, this, [this] {
        cover_panel_->update();
        rich_playlist_view_->viewport()->update();
    });
    applyPlayback({});
    rich_playlist_view_->reload(playlist_id_);
    refreshPresentation();
    rich_playlist_view_->scrollToPlayingTrack();
    requestMissingAlbumCovers();
}

void RichPlaylistPage::applyPlayback(const PlaybackSnapshot& state) {
    const bool changed_track = playback_.track_revision != state.track_revision;
    playback_ = state;
    rich_playlist_view_->applyPlayback(state);
    if (state.hasTrack()) {
        cover_panel_->setNowPlaying(state.track, state.cover);
        cover_panel_->setCurrentEntity(state.track);
    } else {
        cover_panel_->clearNowPlaying();
    }
    refreshPresentation();
    if (changed_track) rich_playlist_view_->scrollToPlayingTrack();
}

QList<PlayListEntity> RichPlaylistPage::tracks() const { return rich_playlist_view_->tracks(); }

void RichPlaylistPage::onAlbumCoverLoaded(int32_t album_id) {
    if (album_id <= 0) {
        return;
    }

    rich_playlist_view_->reload(playlist_id_, true);
    requestMissingAlbumCovers();
}

void RichPlaylistPage::requestMissingAlbumCovers() {
    const auto album_ids = rich_playlist_view_->visibleMissingAlbumCoverIds();
    if (album_ids.isEmpty()) {
        return;
    }

    for (const auto album_id : album_ids) {
        if (requested_album_cover_ids_.contains(album_id)) {
            continue;
        }

        requested_album_cover_ids_.insert(album_id);
        emit findAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
    }
}

void RichPlaylistPage::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    cover_panel_->setVisible(queue_visible_ && event->size().width() >= 850);
}

void RichPlaylistPage::setPlaylist(int32_t id, const QString& name) {
    playlist_id_ = id;
    playlist_name_ = name;
    reload();
    refreshPresentation();
}

void RichPlaylistPage::search(const QString& text) {
    rich_playlist_view_->search(text);
    refreshPresentation();
}

void RichPlaylistPage::setPlaylistName(const QString& name) {
    playlist_name_ = name;
    refreshPresentation();
}

void RichPlaylistPage::toggleQueue() {
    queue_visible_ = !queue_visible_;
    cover_panel_->setVisible(queue_visible_ && width() >= 850);
}

void RichPlaylistPage::refreshPresentation() {
    if (!rich_playlist_view_ || !playlist_title_) return;
    const auto name = playlist_name_.isEmpty() ? tr("My music") : playlist_name_;
    playlist_title_->setText(name);
    playlist_title_->setToolTip(name);
    const auto count = rich_playlist_view_->trackCount();
    playlist_stats_->setText(tr("%1 tracks · %2").arg(count).arg(formatDuration(rich_playlist_view_->totalDuration())));
    const auto first = rich_playlist_view_->firstTrack();
    const auto cover = !playback_.hasTrack() || playback_.cover.isNull()
        ? qImageCache.getOrAddDefault(first.validCoverId()) : playback_.cover;
    const auto scaled_cover = cover.scaled(142, 142, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    playlist_cover_->setPixmap(scaled_cover.copy((scaled_cover.width() - 142) / 2,
        (scaled_cover.height() - 142) / 2, 142, 142));
    QVariantMap play_options;
    play_options.insert(FontIconOption::kColorAttr, qTheme.isDarkTheme() ? QColor("#10231C"_str) : qTheme.textColor());
    play_button_->setIcon(qTheme.fontRawIconOption(Glyphs::ICON_PLAY, play_options));
    play_button_->setEnabled(count > 0);
    shuffle_button_->setEnabled(count > 0);
    cover_panel_->setQueue(playback_.upcoming, playback_.shuffle);
}

void RichPlaylistPage::retranslate() {
    for (auto* label : findChildren<QLabel*>()) {
        const auto source = label->property("translationSource").toByteArray();
        if (!source.isEmpty()) label->setText(QCoreApplication::translate("RichPlaylistPage", source.constData()));
    }
    play_button_->setText(tr("Play"));
    shuffle_button_->setText(tr("Shuffle"));
    rich_playlist_view_->horizontalHeader()->viewport()->update();
    rich_playlist_view_->viewport()->update();
    refreshPresentation();
}
