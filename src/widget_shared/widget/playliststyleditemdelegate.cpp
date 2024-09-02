#include <QPainter>
#include <QHeaderView>

#include <thememanager.h>
#include <widget/imagecache.h>
#include <widget/playlisttableview.h>
#include <widget/playlisttablemodel.h>
#include <widget/playliststyleditemdelegate.h>

PlaylistStyledItemDelegate::PlaylistStyledItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void PlaylistStyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHints(QPainter::TextAntialiasing, true);

    QStyleOptionViewItem opt(option);
    QStyleOptionButton check_box_opt;
    check_box_opt.rect = opt.rect;

    opt.state &= ~QStyle::State_HasFocus;

    const auto* view = qobject_cast<const PlaylistTableView*>(opt.styleObject);
    const auto behavior = view->selectionBehavior();
    const auto hover_index = view->hoverIndex();

    if (!(option.state & QStyle::State_Selected) && behavior != QTableView::SelectItems) {
        if (behavior == QTableView::SelectRows && hover_index.row() == index.row())
            opt.state |= QStyle::State_MouseOver;
        if (behavior == QTableView::SelectColumns && hover_index.column() == index.column())
            opt.state |= QStyle::State_MouseOver;
    }

    const auto value = index.model()->data(index.model()->index(index.row(), index.column()));
    auto use_default_style = false;
    auto use_checkbox_style = false;

    opt.decorationSize = QSize(view->columnWidth(index.column()), view->verticalHeader()->defaultSectionSize());
    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
    opt.font.setFamily(qTEXT("MonoFont"));
    opt.font.setPointSize(qTheme.fontSize(9));

    switch (index.column()) {
    case PLAYLIST_TITLE:
    case PLAYLIST_ALBUM:
        opt.font.setFamily(qTEXT("UIFont"));
        opt.text = value.toString();
        opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
        break;
    case PLAYLIST_ARTIST:
        opt.font.setFamily(qTEXT("UIFont"));
        opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
        opt.text = value.toString();
        break;
    case PLAYLIST_TRACK:
    {
        auto is_playing = indexValue(index, PLAYLIST_IS_PLAYING);
        auto playing_state = is_playing.toInt();
        if (playing_state == PlayingState::PLAY_PLAYING) {
            opt.icon = qTheme.playlistPlayingIcon(kIconSize, 0.5);
            opt.features = QStyleOptionViewItem::HasDecoration;
            opt.decorationAlignment = Qt::AlignCenter;
            opt.displayAlignment = Qt::AlignCenter;
        }
        else if (playing_state == PlayingState::PLAY_PAUSE) {
            opt.icon = qTheme.playlistPauseIcon(kIconSize, 0.5);
            opt.features = QStyleOptionViewItem::HasDecoration;
            opt.decorationAlignment = Qt::AlignCenter;
            opt.displayAlignment = Qt::AlignCenter;
        }
        else {
            opt.displayAlignment = Qt::AlignCenter;
            opt.text = value.toString();
        }
    }
    break;
    case PLAYLIST_FILE_SIZE:
        opt.text = formatBytes(value.toULongLong());
        break;
    case PLAYLIST_BIT_RATE:
        opt.text = formatBitRate(value.toUInt());
        break;
    case PLAYLIST_ALBUM_PK:
    case PLAYLIST_ALBUM_RG:
    case PLAYLIST_TRACK_PK:
    case PLAYLIST_TRACK_RG:
    case PLAYLIST_TRACK_LOUDNESS:
        switch (index.column()) {
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TRACK_PK:
            opt.text = qFormat("%1")
                .arg(value.toDouble(), 4, 'f', 6, QLatin1Char('0'));
            break;
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_TRACK_RG:
            opt.text = qFormat("%1 dB")
                .arg(value.toDouble(), 4, 'f', 2, QLatin1Char('0'));
            break;
        case PLAYLIST_TRACK_LOUDNESS:
            opt.text = qFormat("%1 LUFS")
                .arg(value.toDouble(), 4, 'f', 2, QLatin1Char('0'));
            break;
        }
        break;
    case PLAYLIST_SAMPLE_RATE:
        opt.text = formatSampleRate(value.toUInt());
        break;
    case PLAYLIST_DURATION:
        opt.text = formatDuration(value.toDouble());
        break;
    case PLAYLIST_LAST_UPDATE_TIME:
        opt.text = formatTime(value.toULongLong());
        break;
    case PLAYLIST_LIKE:
    {
        auto is_heart_pressed = indexValue(index, PLAYLIST_LIKE).toInt();
        if (is_heart_pressed > 0) {
            QVariantMap font_options;
            font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(0.4));
            font_options.insert(FontIconOption::kColorAttr, QColor(Qt::red));

            opt.icon = qTheme.fontRawIconOption(is_heart_pressed ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART, font_options);
            // note: 解決圖示再選擇的時候會蓋掉顏色的問題
            opt.icon = qImageCache.uniformIcon(opt.icon, opt.decorationSize);

            opt.features = QStyleOptionViewItem::HasDecoration;
            opt.decorationAlignment = Qt::AlignCenter;
            opt.displayAlignment = Qt::AlignCenter;
        }
    }
    break;
    case PLAYLIST_ALBUM_COVER_ID:
    {
        auto music_cover_id = indexValue(index, PLAYLIST_MUSIC_COVER_ID).toString();
        auto id = value.toString();
        if (!music_cover_id.isEmpty()) {
            id = music_cover_id;
        }
        if (isNullOfEmpty(id)) {
            auto album_id = indexValue(index, PLAYLIST_ALBUM_ID).toInt();
            emit findAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
        }
        opt.icon = qImageCache.getOrAddIcon(id);
        opt.features = QStyleOptionViewItem::HasDecoration;
        opt.decorationAlignment = Qt::AlignCenter;
        opt.displayAlignment = Qt::AlignCenter;
    }
    break;
    default:
        use_default_style = true;
        break;
    }

    if (!use_default_style) {
        if (!use_checkbox_style) {
            option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
        }
        else {
            option.widget->style()->drawControl(QStyle::CE_CheckBox, &check_box_opt, painter, option.widget);
        }
    }
    else {
        QStyledItemDelegate::paint(painter, opt, index);
    }
}
