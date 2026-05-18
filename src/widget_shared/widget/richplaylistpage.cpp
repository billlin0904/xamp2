#include <widget/richplaylistpage.h>

#include <QAbstractItemView>
#include <QCursor>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QScrollArea>
#include <QSizePolicy>

#include <widget/appsettingnames.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/playlistentity.h>
#include <widget/playlisttableview.h>
#include <widget/util/image_util.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

class RichPlaylistCoverPanel final : public QFrame {
public:
    explicit RichPlaylistCoverPanel(QWidget* parent = nullptr)
        : QFrame(parent) {
        setObjectName("richPlaylistCoverPanel"_str);
        setMinimumWidth(360);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setNowPlaying(const xamp::base::TrackInfo& track_info, const QPixmap& cover) {
        cover_ = cover;
        album_ = QString::fromStdWString(track_info.album);
        artist_ = QString::fromStdWString(track_info.artist);
        genre_ = QString::fromStdWString(track_info.genre);
        file_extension_ = track_info.file_ext()
            ? QString::fromStdWString(track_info.file_ext().value()).remove("."_str)
            : kEmptyString;
        duration_ = track_info.duration;
        has_now_playing_ = true;
        update();
    }

    void clearNowPlaying() {
        cover_ = {};
        album_.clear();
        artist_.clear();
        genre_.clear();
        file_extension_.clear();
        duration_ = 0;
        has_now_playing_ = false;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QFrame::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform
            | QPainter::TextAntialiasing,
            true);

        const auto content_rect = rect();
        if (!content_rect.isValid()) {
            return;
        }

        if (!has_now_playing_) {
            QLinearGradient background(content_rect.topLeft(), content_rect.bottomRight());
            background.setColorAt(0.0, QColor(214, 216, 214));
            background.setColorAt(0.42, QColor(246, 247, 245));
            background.setColorAt(1.0, QColor(198, 203, 204));
            painter.fillRect(content_rect, background);

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(120, 120, 120, 45));
            painter.drawRect(QRect(content_rect.left(),
                content_rect.top(),
                content_rect.width() * 0.78,
                content_rect.height()));

            const auto art_edge = qMin(content_rect.width(), content_rect.height()) * 0.62;
            const QRectF disc_rect(content_rect.center().x() - art_edge / 2.0,
                content_rect.center().y() - art_edge / 2.0,
                art_edge,
                art_edge);
            QRadialGradient disc_gradient(disc_rect.center(), art_edge * 0.5);
            disc_gradient.setColorAt(0.0, QColor(255, 255, 255));
            disc_gradient.setColorAt(0.2, QColor(220, 230, 236));
            disc_gradient.setColorAt(0.55, QColor(126, 214, 206));
            disc_gradient.setColorAt(0.75, QColor(170, 78, 222));
            disc_gradient.setColorAt(1.0, QColor(60, 66, 72));
            painter.setBrush(disc_gradient);
            painter.drawEllipse(disc_rect);

            painter.setBrush(QColor(15, 17, 18));
            painter.drawEllipse(QRectF(disc_rect.center().x() - art_edge * 0.24,
                disc_rect.center().y() - art_edge * 0.24,
                art_edge * 0.48,
                art_edge * 0.48));
            painter.setBrush(QColor(245, 245, 245));
            painter.drawEllipse(QRectF(disc_rect.center().x() - art_edge * 0.06,
                disc_rect.center().y() - art_edge * 0.06,
                art_edge * 0.12,
                art_edge * 0.12));

            auto title_font = font();
            title_font.setFamily("UIFont"_str);
            title_font.setPointSize(28);
            title_font.setBold(true);
            painter.setFont(title_font);
            painter.setPen(QColor(55, 55, 55));
            painter.drawText(QRect(content_rect.left() + 58,
                content_rect.top() + 54,
                content_rect.width() - 116,
                54),
                Qt::AlignHCenter | Qt::AlignVCenter,
                "SUNPOCRISY"_str);

            auto subtitle_font = font();
            subtitle_font.setFamily("UIFont"_str);
            subtitle_font.setPointSize(14);
            subtitle_font.setLetterSpacing(QFont::AbsoluteSpacing, 2);
            painter.setFont(subtitle_font);
            painter.setPen(QColor(70, 70, 70, 210));
            painter.drawText(QRect(content_rect.left() + 58,
                content_rect.top() + 104,
                content_rect.width() - 116,
                34),
                Qt::AlignHCenter | Qt::AlignVCenter,
                "EYEGASM, HALLELUJAH!"_str);
            return;
        }

        if (!cover_.isNull()) {
            const auto scaled_cover = cover_.scaled(content_rect.size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation);
            const QRect cover_rect(content_rect.center().x() - scaled_cover.width() / 2,
                content_rect.center().y() - scaled_cover.height() / 2,
                scaled_cover.width(),
                scaled_cover.height());
            painter.drawPixmap(cover_rect, scaled_cover);
            return;
        }

        constexpr auto margin = 34;
        constexpr auto text_height = 118;
        const auto fallback_rect = rect().adjusted(margin, margin, -margin, -margin);
        if (!fallback_rect.isValid()) {
            return;
        }

        const auto cover_area = QRect(fallback_rect.left(),
            fallback_rect.top(),
            fallback_rect.width(),
            qMax(0, fallback_rect.height() - text_height));
        const auto cover_edge = qMin(cover_area.width(), cover_area.height());
        if (cover_edge > 0) {
            const QRect cover_rect(cover_area.center().x() - cover_edge / 2,
                cover_area.top(),
                cover_edge,
                cover_edge);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(22, 26, 28));
            painter.drawRoundedRect(cover_rect, 6, 6);
            painter.setBrush(QColor(70, 70, 70));
            painter.drawEllipse(cover_rect.adjusted(14, 14, -14, -14));
            painter.setBrush(QColor(12, 12, 12));
            painter.drawEllipse(cover_rect.center(), 12, 12);
        }

        const auto text_top = fallback_rect.bottom() - text_height + 18;

        auto title_font = font();
        title_font.setFamily("UIFont"_str);
        title_font.setPointSize(28);
        title_font.setBold(true);
        painter.setFont(title_font);
        painter.setPen(QColor(255, 255, 255));
        QFontMetrics title_metrics(title_font);
        painter.drawText(QRect(fallback_rect.left(),
            text_top,
            fallback_rect.width(),
            46),
            Qt::AlignLeft | Qt::AlignVCenter,
            title_metrics.elidedText(album_, Qt::ElideRight, fallback_rect.width()));

        auto info_font = font();
        info_font.setFamily("UIFont"_str);
        info_font.setPointSize(10);
        painter.setFont(info_font);
        painter.setPen(QColor(255, 255, 255, 185));
        QFontMetrics info_metrics(info_font);
        const auto info = qFormat("%1 | %2 | %3 | Time: %4")
            .arg(artist_)
            .arg(genre_)
            .arg(file_extension_)
            .arg(formatDuration(duration_));
        painter.drawText(QRect(fallback_rect.left(),
            text_top + 62,
            fallback_rect.width(),
            28),
            Qt::AlignLeft | Qt::AlignVCenter,
            info_metrics.elidedText(info, Qt::ElideRight, fallback_rect.width()));
    }

private:
    bool has_now_playing_{ false };
    QPixmap cover_;
    QString album_;
    QString artist_;
    QString genre_;
    QString file_extension_;
    double duration_{ 0 };
};

class RichPlaylistAlbumHeaderPanel final : public QFrame {
public:
    explicit RichPlaylistAlbumHeaderPanel(QWidget* parent = nullptr)
        : QFrame(parent) {
        setObjectName("richPlaylistAlbumHeaderPanel"_str);
        setFixedHeight(78);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setAlbum(const xamp::base::TrackInfo& track_info,
        const QPixmap& cover,
        int track_count,
        double duration) {
        cover_ = cover;
        artist_ = QString::fromStdWString(track_info.artist);
        album_ = QString::fromStdWString(track_info.album);
        genre_ = QString::fromStdWString(track_info.genre);
        file_extension_ = track_info.file_ext()
            ? QString::fromStdWString(track_info.file_ext().value()).remove("."_str)
            : kEmptyString;
        year_ = track_info.year > 0 ? QString::number(track_info.year) : kEmptyString;
        track_count_ = track_count;
        duration_ = duration;
        has_album_ = true;
        update();
    }

    void setAlbum(const PlayListEntity& entity, int track_count, double duration) {
        cover_ = qImageCache.getOrAddDefault(entity.validCoverId());
        artist_ = entity.artist;
        album_ = entity.album;
        genre_ = entity.genre;
        file_extension_ = entity.file_extension;
        file_extension_.remove("."_str);
        year_ = entity.year > 0 ? QString::number(entity.year) : kEmptyString;
        track_count_ = track_count;
        duration_ = duration;
        has_album_ = true;
        update();
    }

    void clearAlbum() {
        cover_ = {};
        artist_.clear();
        album_.clear();
        genre_.clear();
        file_extension_.clear();
        year_.clear();
        track_count_ = 0;
        duration_ = 0;
        has_album_ = false;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QFrame::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform
            | QPainter::TextAntialiasing,
            true);

        const auto row_rect = rect();
        painter.fillRect(row_rect, QColor(43, 43, 43));

        if (!has_album_) {
            painter.setPen(QColor(255, 255, 255, 40));
            painter.drawLine(row_rect.left(), row_rect.bottom(), row_rect.right(), row_rect.bottom());
            return;
        }

        const auto cover_edge = qMin(58, row_rect.height() - 18);
        const QRect cover_rect(row_rect.left() + 18,
            row_rect.top() + (row_rect.height() - cover_edge) / 2,
            cover_edge,
            cover_edge);
        if (!cover_.isNull()) {
            const auto scaled_cover = cover_.scaled(cover_rect.size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation);
            const QRect source_rect((scaled_cover.width() - cover_rect.width()) / 2,
                (scaled_cover.height() - cover_rect.height()) / 2,
                cover_rect.width(),
                cover_rect.height());
            painter.drawPixmap(cover_rect, scaled_cover, source_rect);
        }
        else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(18, 23, 25));
            painter.drawRoundedRect(cover_rect, 4, 4);
            painter.setBrush(QColor(78, 78, 78));
            painter.drawEllipse(cover_rect.adjusted(14, 14, -14, -14));
            painter.setBrush(QColor(8, 8, 8));
            painter.drawEllipse(cover_rect.center(), 10, 10);
        }

        const auto text_left = cover_rect.right() + 18;
        const auto year_width = 96;
        const auto text_right = row_rect.right() - year_width - 20;
        const auto text_width = qMax(0, text_right - text_left);
        const auto top = row_rect.top() + 8;

        auto artist_font = font();
        artist_font.setFamily("UIFont"_str);
        artist_font.setPointSize(13);
        artist_font.setBold(true);
        painter.setFont(artist_font);
        painter.setPen(QColor(139, 206, 161));
        QFontMetrics artist_metrics(artist_font);
        painter.drawText(QRect(text_left, top, text_width, 24),
            Qt::AlignLeft | Qt::AlignVCenter,
            artist_metrics.elidedText(artist_, Qt::ElideRight, text_width));

        const auto line_left = text_left + qMin(artist_metrics.horizontalAdvance(artist_) + 24, text_width / 2);
        painter.setPen(QColor(255, 255, 255, 50));
        painter.drawLine(line_left, top + 13, text_right, top + 13);

        auto album_font = font();
        album_font.setFamily("UIFont"_str);
        album_font.setPointSize(10);
        painter.setFont(album_font);
        painter.setPen(QColor(255, 255, 255, 235));
        QFontMetrics album_metrics(album_font);
        painter.drawText(QRect(text_left, top + 25, text_width, 20),
            Qt::AlignLeft | Qt::AlignVCenter,
            album_metrics.elidedText(album_, Qt::ElideRight, text_width));

        auto info_font = font();
        info_font.setFamily("UIFont"_str);
        info_font.setPointSize(8);
        painter.setFont(info_font);
        painter.setPen(QColor(255, 255, 255, 165));
        const auto info = qFormat("%1 | %2 | %3 Tracks | Time: %4")
            .arg(genre_.isEmpty() ? "Unknown"_str : genre_)
            .arg(file_extension_.isEmpty() ? "file"_str : file_extension_)
            .arg(track_count_)
            .arg(formatDuration(duration_));
        QFontMetrics info_metrics(info_font);
        painter.drawText(QRect(text_left, top + 47, text_width, 18),
            Qt::AlignLeft | Qt::AlignVCenter,
            info_metrics.elidedText(info, Qt::ElideRight, text_width));

        auto year_font = font();
        year_font.setFamily("MonoFont"_str);
        year_font.setPointSize(16);
        year_font.setItalic(true);
        painter.setFont(year_font);
        painter.setPen(QColor(255, 255, 255, 235));
        painter.drawText(QRect(row_rect.right() - year_width - 12,
            top + 1,
            year_width,
            28),
            Qt::AlignRight | Qt::AlignVCenter,
            year_);

        painter.setPen(QColor(255, 255, 255, 45));
        painter.drawLine(row_rect.left(), row_rect.bottom(), row_rect.right(), row_rect.bottom());
    }

private:
    bool has_album_{ false };
    QPixmap cover_;
    QString artist_;
    QString album_;
    QString genre_;
    QString file_extension_;
    QString year_;
    int track_count_{ 0 };
    double duration_{ 0 };
};

RichPlaylistPage::RichPlaylistPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName("richPlaylistPage"_str);
    initial();
}

void RichPlaylistPage::reload() {
    playlist_->reload();
    rebuildAlbumGroups();
}

void RichPlaylistPage::showImportMenu(const QPoint& pos) {
    QMenu menu(this);

    auto* load_file_act = menu.addAction(tr("Load local file"));
    auto* load_dir_act = menu.addAction(tr("Load file directory"));

    const auto* selected_action = menu.exec(pos);
    if (selected_action == load_file_act) {
        loadLocalFile();
    }
    else if (selected_action == load_dir_act) {
        loadFileDirectory();
    }
}

void RichPlaylistPage::loadLocalFile() {
    getOpenMusicFileName(this, tr("Open file"), tr("Music Files "), [this](const auto& file_name) {
        playlist_->append(file_name);
    });
}

void RichPlaylistPage::loadFileDirectory() {
    const auto dir_name = getExistingDirectory(this, tr("Select a directory"));
    if (dir_name.isEmpty()) {
        return;
    }
    playlist_->append(dir_name);
}

void RichPlaylistPage::initial() {
    auto* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    cover_panel_ = new RichPlaylistCoverPanel(this);

    auto* list_panel = new QFrame(this);
    list_panel->setObjectName("richPlaylistListPanel"_str);
    list_panel->setMinimumWidth(460);
    list_panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* list_layout = new QVBoxLayout(list_panel);
    list_layout->setContentsMargins(0, 0, 0, 0);
    list_layout->setSpacing(0);

    album_groups_scroll_area_ = new QScrollArea(list_panel);
    album_groups_scroll_area_->setObjectName("richPlaylistAlbumGroupsScrollArea"_str);
    album_groups_scroll_area_->setWidgetResizable(true);
    album_groups_scroll_area_->setFrameShape(QFrame::NoFrame);
    album_groups_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    album_groups_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    album_groups_widget_ = new QWidget(album_groups_scroll_area_);
    album_groups_widget_->setObjectName("richPlaylistAlbumGroupsWidget"_str);
    album_groups_layout_ = new QVBoxLayout(album_groups_widget_);
    album_groups_layout_->setContentsMargins(0, 0, 0, 0);
    album_groups_layout_->setSpacing(0);
    album_groups_scroll_area_->setWidget(album_groups_widget_);

    playlist_ = new PlaylistTableView(list_panel);
    playlist_->setObjectName("richPlaylistSourceTableView"_str);
    playlist_->setPlayListGroup(PlayListGroup::PLAYLIST_GROUP_ALBUM);
    playlist_->setPlaylistId(kDefaultPlaylistId, kAppSettingPlaylistColumnName);
    playlist_->setHeaderViewHidden(true);
    playlist_->setShowGrid(false);
    playlist_->setSelectionMode(QAbstractItemView::NoSelection);
    playlist_->setFocusPolicy(Qt::NoFocus);
    playlist_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playlist_->setProgressPageHost(list_panel);
    playlist_->setShowProgressOnAppend(false);

    cover_panel_->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(cover_panel_,
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint& pos) {
            showImportMenu(cover_panel_->mapToGlobal(pos));
        });

    list_layout->addWidget(album_groups_scroll_area_);

    main_layout->addWidget(cover_panel_, 5);
    main_layout->addWidget(list_panel, 5);

    setStyleSheet(R"(
        QFrame#richPlaylistPage {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistCoverPanel {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistAlbumHeaderPanel {
            border: none;
            background-color: transparent;
        }
        QScrollArea#richPlaylistAlbumGroupsScrollArea,
        QWidget#richPlaylistAlbumGroupsWidget {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistListPanel,
        QTableView#richPlaylistTableView,
        QTableView#richPlaylistSourceTableView {
            border: none;
            background-color: transparent;
            selection-background-color: #87a985;
            selection-color: #ffffff;
            gridline-color: transparent;
        }
        QTableView#richPlaylistTableView::viewport,
        QTableView#richPlaylistSourceTableView::viewport {
            background-color: transparent;
        }
    )"_str);

    clearNowPlaying();
    rebuildAlbumGroups();
}

void RichPlaylistPage::setNowPlaying(const xamp::base::TrackInfo& track_info, const QPixmap& cover) {
    cover_panel_->setNowPlaying(track_info, cover);
}

void RichPlaylistPage::clearNowPlaying() {
    cover_panel_->clearNowPlaying();
}

void RichPlaylistPage::updateAlbumHeaderFromPlaylist() {
    rebuildAlbumGroups();
}

void RichPlaylistPage::rebuildAlbumGroups() {
    while (auto* item = album_groups_layout_->takeAt(0)) {
        if (auto* widget = item->widget()) {
            if (widget == playlist_) {
                widget->hide();
                widget->setParent(album_groups_widget_);
            }
            else {
                widget->deleteLater();
            }
        }
        delete item;
    }

    const auto items = playlist_->items();
    if (items.isEmpty()) {
        playlist_->setParent(album_groups_widget_);
        playlist_->show();
        playlist_->setAlbumFilterId(std::nullopt);
        album_groups_layout_->addWidget(playlist_);
        return;
    }

    playlist_->hide();

    QHash<int32_t, QList<PlayListEntity>> albums;
    QList<int32_t> album_order;
    for (const auto& item : items) {
        if (!albums.contains(item.album_id)) {
            album_order.push_back(item.album_id);
        }
        albums[item.album_id].push_back(item);
    }

    for (const auto album_id : album_order) {
        const auto album_items = albums.value(album_id);
        if (album_items.isEmpty()) {
            continue;
        }

        auto track_count = 0;
        auto duration = 0.0;
        for (const auto& item : album_items) {
            ++track_count;
            duration += item.duration;
        }

        auto* group_panel = new QFrame(album_groups_widget_);
        group_panel->setObjectName("richPlaylistAlbumGroupPanel"_str);
        auto* group_layout = new QVBoxLayout(group_panel);
        group_layout->setContentsMargins(0, 0, 0, 0);
        group_layout->setSpacing(0);

        auto* album_header_panel = new RichPlaylistAlbumHeaderPanel(group_panel);
        album_header_panel->setAlbum(album_items.front(), track_count, duration);

        auto* album_playlist = new PlaylistTableView(group_panel);
        album_playlist->setObjectName("richPlaylistTableView"_str);
        album_playlist->setPlayListGroup(PlayListGroup::PLAYLIST_GROUP_ALBUM);
        album_playlist->setAlbumFilterId(album_id);
        album_playlist->setPlaylistId(kDefaultPlaylistId, kAppSettingPlaylistColumnName);
        album_playlist->setHeaderViewHidden(true);
        album_playlist->setShowGrid(false);
        album_playlist->setSelectionMode(QAbstractItemView::NoSelection);
        album_playlist->setFocusPolicy(Qt::NoFocus);
        album_playlist->disableLoadFile(false);
        album_playlist->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        album_playlist->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        album_playlist->setFixedHeight(qMax(PlaylistTableView::kColumnHeight, track_count * PlaylistTableView::kColumnHeight));

        (void)QObject::connect(album_playlist,
            &PlaylistTableView::playMusic,
            playlist_,
            &PlaylistTableView::playMusic);

        group_layout->addWidget(album_header_panel);
        group_layout->addWidget(album_playlist);
        album_groups_layout_->addWidget(group_panel);
    }

    album_groups_layout_->addStretch(1);
}
