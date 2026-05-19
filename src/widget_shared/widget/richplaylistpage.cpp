#include <widget/richplaylistpage.h>

#include <QAbstractTableModel>
#include <QApplication>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QSizePolicy>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVariantAnimation>
#include <QVBoxLayout>

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
    constexpr auto kRichAlbumHeaderHeight = 60;
    constexpr auto kRichProgressPageHeight = 104;
    constexpr auto kRichSearchCollapsedWidth = 42;
    constexpr auto kRichSearchExpandedWidth = 320;
    constexpr auto kRichSearchHeight = 32;
    constexpr QSize kRichPlaylistPlayingIconSize(20, 20);

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

class RichPlaylistSearchLineEdit final : public QLineEdit {
public:
    explicit RichPlaylistSearchLineEdit(const QString& placeholder, QWidget* parent = nullptr)
        : QLineEdit(parent)
        , placeholder_(placeholder)
        , animation_(new QVariantAnimation(this)) {
        setFixedWidth(kRichSearchCollapsedWidth);
        setFixedHeight(kRichSearchHeight);
        setPlaceholderText(kEmptyString);
        setCollapsed(true);

        animation_->setDuration(150);
        animation_->setEasingCurve(QEasingCurve::OutCubic);
        qApp->installEventFilter(this);
        (void)QObject::connect(animation_,
            &QVariantAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                setFixedWidth(value.toInt());
            });
    }

    ~RichPlaylistSearchLineEdit() override {
        qApp->removeEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonPress && width() > kRichSearchCollapsedWidth) {
            const auto* mouse_event = static_cast<QMouseEvent*>(event);
            if (!rect().contains(mapFromGlobal(mouse_event->globalPosition().toPoint()))) {
                collapse();
                clearFocus();
            }
        }
        return QLineEdit::eventFilter(watched, event);
    }

    void focusInEvent(QFocusEvent* event) override {
        expand();
        QLineEdit::focusInEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        const auto was_collapsed = width() <= kRichSearchCollapsedWidth;
        expand();
        setFocus(Qt::MouseFocusReason);
        if (was_collapsed) {
            setCursorPosition(text().size());
            event->accept();
            return;
        }
        QLineEdit::mousePressEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override {
        QLineEdit::focusOutEvent(event);
        collapse();
    }

private:
    void expand() {
        setCollapsed(false);
        setPlaceholderText(placeholder_);
        animateWidth(kRichSearchExpandedWidth);
    }

    void collapse() {
        setPlaceholderText(kEmptyString);
        setCollapsed(true);
        animateWidth(kRichSearchCollapsedWidth);
    }

    void animateWidth(int width) {
        if (width == this->width()) {
            return;
        }

        animation_->stop();
        animation_->setStartValue(this->width());
        animation_->setEndValue(width);
        animation_->start();
    }

    void setCollapsed(bool collapsed) {
        setProperty("collapsed", collapsed);
        style()->unpolish(this);
        style()->polish(this);
    }

    QString placeholder_;
    QVariantAnimation* animation_{ nullptr };
};

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

        painter.fillRect(content_rect, QColor(12, 13, 13));

        if (!has_now_playing_) {
            paintCoverImage(&painter, startup_cover_, scaledRect(content_rect, 0.85), Qt::KeepAspectRatio);
            return;
        }

        if (!cover_.isNull()) {
            paintCoverImage(&painter, cover_, content_rect, Qt::KeepAspectRatioByExpanding);
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
    QRect scaledRect(const QRect& rect, double scale) const {
        const auto width = qRound(rect.width() * scale);
        const auto height = qRound(rect.height() * scale);
        return QRect(rect.center().x() - width / 2,
            rect.center().y() - height / 2,
            width,
            height);
    }

    void paintCoverImage(QPainter* painter,
        const QPixmap& cover,
        const QRect& content_rect,
        Qt::AspectRatioMode aspect_ratio_mode) const {
        if (cover.isNull()) {
            return;
        }

        const auto scaled_cover = cover.scaled(content_rect.size(),
            aspect_ratio_mode,
            Qt::SmoothTransformation);
        const QRect cover_rect(content_rect.center().x() - scaled_cover.width() / 2,
            content_rect.center().y() - scaled_cover.height() / 2,
            scaled_cover.width(),
            scaled_cover.height());
        painter->drawPixmap(cover_rect, scaled_cover);
    }

    bool has_now_playing_{ false };
    QPixmap cover_;
    QPixmap startup_cover_{ QStringLiteral(":/xamp/xamp_startup.png") };
    QString album_;
    QString artist_;
    QString genre_;
    QString file_extension_;
    double duration_{ 0 };
};

class RichPlaylistModel final : public QAbstractTableModel {
public:
    explicit RichPlaylistModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
        , source_model_(new QSqlQueryModel(this)) {
    }

    void reload(int32_t playlist_id, const QString& keyword) {
        beginResetModel();
        const QSqlQuery query(richPlaylistQuery(playlist_id), qGuiDb.database());
        source_model_->setQuery(query);
        while (source_model_->canFetchMore()) {
            source_model_->fetchMore();
        }
        if (source_model_->lastError().type() != QSqlError::NoError) {
            XAMP_LOG_DEBUG("SqlException: {}", source_model_->lastError().text().toStdString());
        }
        rebuildRows(keyword.trimmed());
        endResetModel();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : rows_.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : PLAYLIST_MAX_COLUMN;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()
            || index.row() < 0
            || index.row() >= rows_.size()) {
            return {};
        }

        const auto& row = rows_[index.row()];
        if (role == PLAYLIST_ROW_TYPE_ROLE) {
            return row.is_header ? PLAYLIST_ROW_ALBUM_HEADER : PLAYLIST_ROW_TRACK;
        }
        if (role == PLAYLIST_ALBUM_TRACK_COUNT_ROLE) {
            return row.track_count;
        }
        if (role == PLAYLIST_ALBUM_DURATION_ROLE) {
            return row.duration;
        }

        const auto source_index = source_model_->index(row.source_row, index.column());
        if (!source_index.isValid()) {
            return {};
        }

        if (row.is_header && role == Qt::DisplayRole) {
            switch (index.column()) {
            case PLAYLIST_TRACK:
                return {};
            case PLAYLIST_TITLE:
                return source_model_->data(source_model_->index(row.source_row, PLAYLIST_ALBUM), role);
            default:
                break;
            }
        }

        return source_model_->data(source_index, role);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        return source_model_->headerData(section, orientation, role);
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override {
        if (!index.isValid()) {
            return QAbstractTableModel::flags(index);
        }
        if (isHeaderRow(index.row())) {
            return Qt::ItemIsEnabled;
        }
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    bool isHeaderRow(int row) const {
        return row >= 0
            && row < rows_.size()
            && rows_[row].is_header;
    }

    bool isTrackRow(const QModelIndex& index) const {
        return index.isValid()
            && index.row() >= 0
            && index.row() < rows_.size()
            && !rows_[index.row()].is_header;
    }

    PlayListEntity entity(const QModelIndex& index) const {
        return getEntity(index);
    }

private:
    struct Row {
        int source_row{ -1 };
        bool is_header{ false };
        int track_count{ 0 };
        double duration{ 0 };
    };

    int sourceAlbumId(int source_row) const {
        return source_model_->data(source_model_->index(source_row, PLAYLIST_ALBUM_ID)).toInt();
    }

    bool matchesKeyword(int source_row, const QString& keyword) const {
        if (keyword.isEmpty()) {
            return true;
        }

        const QStringList values{
            source_model_->data(source_model_->index(source_row, PLAYLIST_TITLE)).toString(),
            source_model_->data(source_model_->index(source_row, PLAYLIST_FILE_NAME)).toString(),
            source_model_->data(source_model_->index(source_row, PLAYLIST_ARTIST)).toString(),
            source_model_->data(source_model_->index(source_row, PLAYLIST_ALBUM)).toString(),
            source_model_->data(source_model_->index(source_row, PLAYLIST_GENRE)).toString(),
        };
        for (const auto& value : values) {
            if (value.contains(keyword, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    void rebuildRows(const QString& keyword) {
        rows_.clear();

        const auto source_row_count = source_model_->rowCount();
        rows_.reserve(source_row_count + 16);

        auto source_row = 0;
        while (source_row < source_row_count) {
            const auto album_id = sourceAlbumId(source_row);
            QVector<int> track_rows;
            auto track_count = 0;
            auto duration = 0.0;
            auto next_row = source_row;

            while (next_row < source_row_count && sourceAlbumId(next_row) == album_id) {
                if (matchesKeyword(next_row, keyword)) {
                    track_rows.push_back(next_row);
                    ++track_count;
                    duration += source_model_->data(source_model_->index(next_row, PLAYLIST_DURATION)).toDouble();
                }
                ++next_row;
            }

            if (track_rows.isEmpty()) {
                source_row = next_row;
                continue;
            }

            rows_.push_back({ track_rows.front(), true, track_count, duration });
            for (const auto track_row : track_rows) {
                rows_.push_back({ track_row, false, track_count, duration });
            }

            source_row = next_row;
        }
    }

    QSqlQueryModel* source_model_{ nullptr };
    QVector<Row> rows_;
};

class RichPlaylistStyledItemDelegate final : public QStyledItemDelegate {
public:
    explicit RichPlaylistStyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.data(PLAYLIST_ROW_TYPE_ROLE).toInt() == PLAYLIST_ROW_ALBUM_HEADER) {
            return QSize(option.rect.width(), kRichAlbumHeaderHeight);
        }
        return QSize(option.rect.width(), PlaylistTableView::kColumnHeight);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.data(PLAYLIST_ROW_TYPE_ROLE).toInt() == PLAYLIST_ROW_ALBUM_HEADER) {
            paintAlbumHeader(painter, option, index);
            return;
        }
        paintTrackCell(painter, option, index);
    }

private:
    void paintAlbumHeader(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform
            | QPainter::TextAntialiasing,
            true);

        const auto row_rect = option.rect;
        painter->fillRect(row_rect, QColor(43, 43, 43));

        const auto cover_edge = qMin(48, row_rect.height() - 12);
        const QRect cover_rect(row_rect.left() + 14,
            row_rect.top() + (row_rect.height() - cover_edge) / 2,
            cover_edge,
            cover_edge);

        const auto cover = qImageCache.getOrAddDefault(index.model()->data(index.model()->index(index.row(), PLAYLIST_ALBUM_COVER_ID)).toString());
        if (!cover.isNull()) {
            const auto scaled_cover = cover.scaled(cover_rect.size(),
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation);
            const QRect source_rect((scaled_cover.width() - cover_rect.width()) / 2,
                (scaled_cover.height() - cover_rect.height()) / 2,
                cover_rect.width(),
                cover_rect.height());
            painter->drawPixmap(cover_rect, scaled_cover, source_rect);
        }
        else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(18, 23, 25));
            painter->drawRoundedRect(cover_rect, 4, 4);
            painter->setBrush(QColor(78, 78, 78));
            painter->drawEllipse(cover_rect.adjusted(14, 14, -14, -14));
            painter->setBrush(QColor(8, 8, 8));
            painter->drawEllipse(cover_rect.center(), 10, 10);
        }

        const auto artist = index.model()->data(index.model()->index(index.row(), PLAYLIST_ARTIST)).toString();
        const auto album = index.model()->data(index.model()->index(index.row(), PLAYLIST_ALBUM)).toString();
        const auto genre = index.model()->data(index.model()->index(index.row(), PLAYLIST_GENRE)).toString();
        const auto file_ext = displayFileExt(index.model()->data(index.model()->index(index.row(), PLAYLIST_FILE_EXT)).toString());
        const auto year = index.model()->data(index.model()->index(index.row(), PLAYLIST_YEAR)).toUInt();
        const auto track_count = index.data(PLAYLIST_ALBUM_TRACK_COUNT_ROLE).toInt();
        const auto duration = index.data(PLAYLIST_ALBUM_DURATION_ROLE).toDouble();

        const auto text_left = cover_rect.right() + 14;
        const auto year_width = 86;
        const auto text_right = row_rect.right() - year_width - 18;
        const auto text_width = qMax(0, text_right - text_left);
        const auto top = row_rect.top() + 5;

        auto artist_font = option.font;
        artist_font.setFamily("UIFont"_str);
        artist_font.setPointSize(10);
        artist_font.setBold(true);
        painter->setFont(artist_font);
        painter->setPen(QColor(139, 206, 161));
        QFontMetrics artist_metrics(artist_font);
        painter->drawText(QRect(text_left, top, text_width, 18),
            Qt::AlignLeft | Qt::AlignVCenter,
            artist_metrics.elidedText(artist, Qt::ElideRight, text_width));

        const auto line_left = text_left + qMin(artist_metrics.horizontalAdvance(artist) + 24, text_width / 2);
        painter->setPen(QColor(255, 255, 255, 50));
        painter->drawLine(line_left, top + 10, text_right, top + 10);

        auto album_font = option.font;
        album_font.setFamily("UIFont"_str);
        album_font.setPointSize(9);
        painter->setFont(album_font);
        painter->setPen(QColor(255, 255, 255, 235));
        QFontMetrics album_metrics(album_font);
        painter->drawText(QRect(text_left, top + 20, text_width, 17),
            Qt::AlignLeft | Qt::AlignVCenter,
            album_metrics.elidedText(album, Qt::ElideRight, text_width));

        auto info_font = option.font;
        info_font.setFamily("UIFont"_str);
        info_font.setPointSize(8);
        painter->setFont(info_font);
        painter->setPen(QColor(255, 255, 255, 165));
        const auto info = qFormat("%1 | %2 | %3 Tracks | Time: %4")
            .arg(genre.isEmpty() ? "Unknown"_str : genre)
            .arg(file_ext)
            .arg(track_count)
            .arg(formatDuration(duration));
        QFontMetrics info_metrics(info_font);
        painter->drawText(QRect(text_left, top + 38, text_width, 16),
            Qt::AlignLeft | Qt::AlignVCenter,
            info_metrics.elidedText(info, Qt::ElideRight, text_width));

        if (year > 0) {
            auto year_font = option.font;
            year_font.setFamily("MonoFont"_str);
            year_font.setPointSize(14);
            year_font.setItalic(true);
            painter->setFont(year_font);
            painter->setPen(QColor(255, 255, 255, 235));
            painter->drawText(QRect(text_right + 16,
                top + 2,
                year_width,
                24),
                Qt::AlignLeft | Qt::AlignVCenter,
                QString::number(year));
        }

        painter->setPen(QColor(255, 255, 255, 45));
        painter->drawLine(row_rect.left(), row_rect.bottom(), row_rect.right(), row_rect.bottom());
        painter->restore();
    }

    void paintTrackCell(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform
            | QPainter::TextAntialiasing,
            true);

        const auto rect = option.rect;
        const auto playing_state = index.model()->data(index.model()->index(index.row(), PLAYLIST_IS_PLAYING)).toInt();
        const auto is_selected = option.state & QStyle::State_Selected;
        painter->fillRect(rect, playing_state == PlayingState::PLAY_CLEAR
            ? (is_selected ? QColor(48, 58, 70) : QColor(13, 13, 13))
            : QColor(54, 68, 84));

        painter->setPen(QColor(255, 255, 255, 35));
        painter->drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());

        auto font = option.font;
        font.setFamily(index.column() == PLAYLIST_TITLE ? "UIFont"_str : "MonoFont"_str);
        font.setPointSize(qTheme.fontSize(8));
        painter->setFont(font);
        painter->setPen(QColor(240, 245, 250));
        QFontMetrics metrics(font);

        switch (index.column()) {
        case PLAYLIST_TRACK:
        {
            if (playing_state == PlayingState::PLAY_PLAYING) {
                qTheme.playlistPlayingIcon(kRichPlaylistPlayingIconSize, 0.5).paint(painter, rect, Qt::AlignCenter);
            }
            else if (playing_state == PlayingState::PLAY_PAUSE) {
                qTheme.playlistPauseIcon(kRichPlaylistPlayingIconSize, 0.5).paint(painter, rect, Qt::AlignCenter);
            }
            else {
                painter->drawText(rect.adjusted(4, 0, -4, 0),
                    Qt::AlignCenter,
                    index.data().toString());
            }
            break;
        }
        case PLAYLIST_TITLE:
            painter->drawText(rect.adjusted(8, 0, -8, 0),
                Qt::AlignLeft | Qt::AlignVCenter,
                metrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width() - 16));
            break;
        case PLAYLIST_SAMPLE_RATE:
            painter->drawText(rect.adjusted(8, 0, -8, 0),
                Qt::AlignRight | Qt::AlignVCenter,
                formatSampleRate(index.data().toUInt()));
            break;
        case PLAYLIST_DURATION:
            painter->drawText(rect.adjusted(8, 0, -8, 0),
                Qt::AlignRight | Qt::AlignVCenter,
                formatDuration(index.data().toDouble()));
            break;
        default:
            break;
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
        setModel(model_);
        setItemDelegate(new RichPlaylistStyledItemDelegate(this));
        setShowGrid(false);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        horizontalHeader()->hide();
        verticalHeader()->hide();
        verticalHeader()->setDefaultSectionSize(PlaylistTableView::kColumnHeight);
        configureColumns();
    }

    void reload(int32_t playlist_id) {
        playlist_id_ = playlist_id;
        model_->reload(playlist_id_, search_text_);
        album_songs_id_cache_.clear();
        updateRowLayout();
    }

    void search(const QString& keyword) {
        search_text_ = keyword;
        model_->reload(playlist_id_, search_text_);
        album_songs_id_cache_.clear();
        updateRowLayout();
    }

    bool isTrackRow(const QModelIndex& index) const {
        return model_->isTrackRow(index);
    }

    PlayListEntity item(const QModelIndex& index) const {
        return model_->entity(index);
    }

    QModelIndex firstIndex() const {
        for (auto row = 0; row < model_->rowCount(); ++row) {
            if (!model_->isHeaderRow(row)) {
                return model_->index(row, PLAYLIST_IS_PLAYING);
            }
        }
        return {};
    }

    QModelIndex nextIndex(int32_t forward) const {
        const auto count = model_->rowCount();
        if (count == 0) {
            return {};
        }

        auto current_row = currentIndex().isValid() ? currentIndex().row() : playingRow();
        if (current_row < 0) {
            current_row = 0;
        }

        for (auto i = 0; i < count; ++i) {
            current_row = (current_row + forward + count) % count;
            if (!model_->isHeaderRow(current_row)) {
                return model_->index(current_row, PLAYLIST_IS_PLAYING);
            }
        }
        return {};
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
            if (model_->isHeaderRow(row)) {
                continue;
            }
            const auto index = model_->index(row, PLAYLIST_PLAYLIST_MUSIC_ID);
            if (index.data().toInt() == playlist_music_id) {
                return index;
            }
        }
        return {};
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QTableView::resizeEvent(event);
        configureColumns();
    }

private:
    void configureColumns() {
        for (auto column = 0; column < PLAYLIST_MAX_COLUMN; ++column) {
            setColumnHidden(column, true);
        }

        const QList<int> visible_columns{
            PLAYLIST_TRACK,
            PLAYLIST_TITLE,
            PLAYLIST_SAMPLE_RATE,
            PLAYLIST_DURATION,
        };
        for (const auto column : visible_columns) {
            setColumnHidden(column, false);
        }

        horizontalHeader()->setSectionResizeMode(PLAYLIST_TRACK, QHeaderView::Fixed);
        horizontalHeader()->resizeSection(PLAYLIST_TRACK, 72);
        horizontalHeader()->setSectionResizeMode(PLAYLIST_TITLE, QHeaderView::Stretch);
        horizontalHeader()->setSectionResizeMode(PLAYLIST_SAMPLE_RATE, QHeaderView::Fixed);
        horizontalHeader()->resizeSection(PLAYLIST_SAMPLE_RATE, 160);
        horizontalHeader()->setSectionResizeMode(PLAYLIST_DURATION, QHeaderView::Fixed);
        horizontalHeader()->resizeSection(PLAYLIST_DURATION, 96);
    }

    void updateRowLayout() {
        clearSpans();
        for (auto row = 0; row < model_->rowCount(); ++row) {
            if (model_->isHeaderRow(row)) {
                setRowHeight(row, kRichAlbumHeaderHeight);
                setSpan(row, PLAYLIST_TRACK, 1, PLAYLIST_MAX_COLUMN - PLAYLIST_TRACK);
            }
            else {
                setRowHeight(row, PlaylistTableView::kColumnHeight);
            }
        }
    }

    int playingRow() const {
        for (auto row = 0; row < model_->rowCount(); ++row) {
            if (model_->isHeaderRow(row)) {
                continue;
            }
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

        rng_.SetSeed(current_album_id);

        if (album_songs_id_cache_.isEmpty()) {
            for (auto row = 0; row < count; ++row) {
                if (model_->isHeaderRow(row)) {
                    continue;
                }
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
                const auto selected_album_index = rng_.NextInt32(0, album_ids.size() - 1);
                selected_album_id = album_ids[selected_album_index];
            } while (selected_album_id == current_album_id);
        }

        if (current_playlist_music_id != 0) {
            rng_.SetSeed(current_playlist_music_id);
        }

        const auto& selected_album_songs = album_songs_id_cache_[selected_album_id];
        if (selected_album_songs.isEmpty()) {
            return {};
        }

        const auto selected_song_index = rng_.NextInt32(0, selected_album_songs.size() - 1);
        return model_->index(selected_album_songs[selected_song_index], PLAYLIST_IS_PLAYING);
    }

    RichPlaylistModel* model_{ nullptr };
    int32_t playlist_id_{ kDefaultPlaylistId };
    QString search_text_;
    PRNG rng_;
    QHash<int32_t, QList<int32_t>> album_songs_id_cache_;
};

RichPlaylistPage::RichPlaylistPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName("richPlaylistPage"_str);
    initial();
}

void RichPlaylistPage::reload() {
    rich_playlist_view_->reload(kDefaultPlaylistId);
}

void RichPlaylistPage::showImportMenu(const QPoint& pos) {
    QMenu menu(this);

    auto* load_file_act = menu.addAction(tr("Load local file"));
    auto* load_dir_act = menu.addAction(tr("Load file directory"));
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

void RichPlaylistPage::loadLocalFile() {
    getOpenMusicFileName(this, tr("Open file"), tr("Music Files "), [this](const auto& file_name) {
        showProgressPage();
        emit extractFile(file_name, kDefaultPlaylistId);
    });
}

void RichPlaylistPage::loadFileDirectory() {
    const auto dir_name = getExistingDirectory(this, tr("Select a directory"));
    if (dir_name.isEmpty()) {
        return;
    }
    showProgressPage();
    emit extractFile(dir_name, kDefaultPlaylistId);
}

void RichPlaylistPage::clearAll() {
    qDaoFacade.playlist_dao.removePlaylistAllMusic(kDefaultPlaylistId);
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
    qDaoFacade.playlist_dao.clearNowPlaying(kDefaultPlaylistId);
    qDaoFacade.playlist_dao.setNowPlayingState(kDefaultPlaylistId,
        entity.playlist_music_id,
        PlayingState::PLAY_PLAYING);

    rich_playlist_view_->reload(kDefaultPlaylistId);
    const auto current_index = rich_playlist_view_->indexForPlaylistMusicId(entity.playlist_music_id);
    if (current_index.isValid()) {
        rich_playlist_view_->setCurrentIndex(current_index);
        rich_playlist_view_->scrollTo(current_index, QAbstractItemView::PositionAtCenter);
    }

    emit playMusic(kDefaultPlaylistId, entity, is_play);
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

    auto* list_panel = new QFrame(this);
    list_panel->setObjectName("richPlaylistListPanel"_str);
    list_panel->setMinimumWidth(460);
    list_panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* list_layout = new QVBoxLayout(list_panel);
    list_layout->setContentsMargins(0, 0, 0, 0);
    list_layout->setSpacing(0);

    auto* search_panel = new QFrame(list_panel);
    search_panel->setObjectName("richPlaylistSearchPanel"_str);
    search_panel->setFixedHeight(52);

    auto* search_layout = new QHBoxLayout(search_panel);
    search_layout->setContentsMargins(16, 10, 16, 10);
    search_layout->setSpacing(0);

    auto* search_line_edit = new RichPlaylistSearchLineEdit(tr("Search Album/Artist/Title"), search_panel);
    search_line_edit->setObjectName("richPlaylistSearchLineEdit"_str);
    search_line_edit->setClearButtonEnabled(true);
    search_line_edit->setFocusPolicy(Qt::ClickFocus);
    auto search_font = search_line_edit->font();
    search_font.setFamily("UIFont"_str);
    search_font.setPointSize(qTheme.fontSize(8));
    search_line_edit->setFont(search_font);
    search_line_edit->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
    search_layout->addStretch(1);
    search_layout->addWidget(search_line_edit);

    rich_playlist_view_ = new RichPlaylistView(list_panel);
    (void)QObject::connect(search_line_edit,
        &QLineEdit::textChanged,
        rich_playlist_view_,
        &RichPlaylistView::search);

    list_layout->addWidget(search_panel);
    list_layout->addWidget(rich_playlist_view_);

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

    rich_playlist_view_->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(rich_playlist_view_->viewport(),
        &QWidget::customContextMenuRequested,
        this,
        [this](const QPoint& pos) {
            showImportMenu(rich_playlist_view_->viewport()->mapToGlobal(pos));
        });

    (void)QObject::connect(rich_playlist_view_,
        &QTableView::doubleClicked,
        this,
        [this](const QModelIndex& index) {
            playIndex(index, true);
        });

    main_layout->addWidget(cover_panel_, 5);
    main_layout->addWidget(list_panel, 5);
    root_layout->addWidget(content_panel, 1);
    root_layout->addWidget(progress_page_);

    setStyleSheet(R"(
        QFrame#richPlaylistPage {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistContentPanel {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistCoverPanel {
            border: none;
            background-color: transparent;
        }
        QFrame#richPlaylistListPanel,
        QTableView#richPlaylistTableView {
            border: none;
            background-color: transparent;
            gridline-color: transparent;
        }
        QFrame#richPlaylistSearchPanel {
            border: none;
            background-color: transparent;
        }
        QLineEdit#richPlaylistSearchLineEdit {
            min-height: 32px;
            max-height: 32px;
            border-radius: 16px;
            border: none;
            background-color: rgba(255, 255, 255, 18);
            color: rgb(236, 241, 247);
            padding-left: 6px;
            padding-right: 10px;
            selection-background-color: rgba(139, 206, 161, 150);
        }
        QLineEdit#richPlaylistSearchLineEdit:focus {
            border: none;
            background-color: rgba(255, 255, 255, 24);
        }
        QLineEdit#richPlaylistSearchLineEdit[collapsed="true"] {
            color: transparent;
        }
        QTableView#richPlaylistTableView::viewport {
            background-color: transparent;
        }
    )"_str);

    clearNowPlaying();
    rich_playlist_view_->reload(kDefaultPlaylistId);
}

void RichPlaylistPage::setNowPlaying(const xamp::base::TrackInfo& track_info, const QPixmap& cover) {
    cover_panel_->setNowPlaying(track_info, cover);
    rich_playlist_view_->reload(kDefaultPlaylistId);
}

void RichPlaylistPage::clearNowPlaying() {
    cover_panel_->clearNowPlaying();
}
