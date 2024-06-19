#include <widget/util/image_util.h>
#include <widget/albumviewstyleddelegate.h>
#include <widget/util/ui_util.h>
#include <widget/imagecache.h>
#include <widget/databasefacade.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/tagio.h>
#include <thememanager.h>

#include <QApplication>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPainter>

namespace {
    constexpr auto kMoreIconSize = 24;
    constexpr auto kIconSize = 40;
    constexpr auto kImageCacheSize = 24;
    constexpr auto kPaddingSize = 10;

    inline QRect moreButtonRect(const QStyleOptionViewItem& option, const QSize& cover_size) noexcept {
        const QRect more_button_rect(
            option.rect.left() + cover_size.width() - 10,
            option.rect.top() + cover_size.height() + 35,
            kMoreIconSize, kMoreIconSize);
        return more_button_rect;
    }

    inline QRect hiResIconRect(const QStyleOptionViewItem& option, const QSize& cover_size) noexcept {
        const QRect hi_res_icon_rect(
            option.rect.left() + cover_size.width() - 10,
            option.rect.top() + cover_size.height() + 15,
            kMoreIconSize - 4, kMoreIconSize - 4);
        return hi_res_icon_rect;
    }

    inline QRect checkBoxIconRect(const QStyleOptionViewItem& option, const QSize& cover_size) noexcept {
        const QRect checkbox_icon_rect(
            option.rect.left() + cover_size.width() + 15,
            option.rect.top() + 2,
            kMoreIconSize, kMoreIconSize);
        return checkbox_icon_rect;
    }

    inline QRect albumTextRect(const QStyleOptionViewItem& option, const QSize& cover_size, uint32_t album_artist_text_width) noexcept {
        const QRect album_text_rect(option.rect.left() + kPaddingSize,
            option.rect.top() + cover_size.height() + 15,
            album_artist_text_width,
            15);
        return album_text_rect;
    }

    inline QRect artistTextRect(const QStyleOptionViewItem& option, const QSize& cover_size) noexcept {
        const QRect artist_text_rect(option.rect.left() + kPaddingSize,
            option.rect.top() + cover_size.height() + 30,
            cover_size.width(),
            20);
        return artist_text_rect;
    }

    inline QRect selectedAlbumRect(const QStyleOptionViewItem& option) noexcept {
        const QRect selected_rect(option.rect.left() + 5,
            option.rect.top() + 5,
            option.rect.width() - 5,
            option.rect.height() - 5);
        return selected_rect;
    }

    inline QRect playButtonRect(const QStyleOptionViewItem& option, const QSize& cover_size) noexcept {
        const QRect play_button_rect(
            option.rect.left() + cover_size.width() / 2 - 10,
            option.rect.top() + cover_size.height() / 2 - 10,
            kIconSize, kIconSize);
        return play_button_rect;
    }

    inline QRect coverRect(const QStyleOptionViewItem& option, const QSize& cover_size) {
        const QRect cover_rect(option.rect.left() + kPaddingSize,
            option.rect.top() + kPaddingSize,
            cover_size.width(),
            cover_size.height());
        return cover_rect;
    }

    QSet<int32_t> getVisibleAlbumId(const QStyleOptionViewItem& option) {
        QSet<int32_t> view_items;
        const auto* list_view = static_cast<const QAbstractItemView*>(option.widget);
        if (!list_view) {
            return view_items;
        }

        Q_FOREACH(auto index, getVisibleIndexes(list_view, 0)) {
            auto album_id = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();
            view_items.insert(album_id);
        }
        return view_items;
    }

    QSet<QString> getVisibleCoverId(const QStyleOptionViewItem& option) {
        QSet<QString> view_items;
        const auto* list_view = static_cast<const QAbstractItemView*>(option.widget);
        if (!list_view) {
            return view_items;
        }

        Q_FOREACH(auto index, getVisibleIndexes(list_view, 0)) {
            auto cover_id = indexValue(index, ALBUM_INDEX_COVER).toString();
            if (!isNullOfEmpty(cover_id)) {
                view_items.insert(cover_id);
            }
        }
        return view_items;
    }
}
AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , album_text_color_(Qt::black)
    , more_album_opt_button_(new QPushButton())
    , play_button_(new QPushButton())
    , edit_mode_checkbox_(new QCheckBox()) {
    more_album_opt_button_->setStyleSheet(qTEXT("background-color: transparent"));
    play_button_->setStyleSheet(qTEXT("background-color: transparent"));

    edit_mode_checkbox_->setObjectName(qTEXT("editModeCheckbox"));
    edit_mode_checkbox_->setStyleSheet(qSTR(R"(
                                         QCheckBox#editModeCheckbox {
        			                        background-color: transparent;
    							            color: white;
                                         }
                                         )"));
    mask_image_ = image_util::roundDarkImage(qTheme.defaultCoverSize(),
        image_util::kDarkAlpha, image_util::kSmallImageRadius);
    cover_size_ = qTheme.defaultCoverSize();
}

void AlbumViewStyledDelegate::setAlbumTextColor(QColor color) {
    album_text_color_ = color;
}

void AlbumViewStyledDelegate::enableAlbumView(bool enable) {
    enable_album_view_ = enable;
}

bool AlbumViewStyledDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!enable_album_view_) {
        return true;
    }

    const auto* ev = dynamic_cast<QMouseEvent*> (event);
    mouse_point_ = ev->pos();
    const auto current_cursor = QApplication::overrideCursor();

    switch (ev->type()) {
    case QEvent::MouseButtonPress:
        if (ev->button() == Qt::LeftButton) {
            if (current_cursor != nullptr) {
                if (current_cursor->shape() == Qt::PointingHandCursor) {
                    emit enterAlbumView(index);
                }
            }
            if (moreButtonRect(option, cover_size_).contains(mouse_point_)) {
                emit showAlbumMenu(index, mouse_point_);
            }
            if (checkBoxIconRect(option, cover_size_).contains(mouse_point_)) {
                auto is_selected = indexValue(index, ALBUM_INDEX_IS_SELECTED).toBool();
                emit editAlbumView(index, is_selected);
            }
        }
        break;
    default:
        break;
    }
    return true;
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    auto album_id = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();

    if (album_id == 0) {
        // Note: Qt6 no more data.
        return;
    }

    auto* style = option.widget ? option.widget->style() : QApplication::style();

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHints(QPainter::TextAntialiasing, true);

    auto album = indexValue(index, ALBUM_INDEX_ALBUM).toString();
    auto cover_id = indexValue(index, ALBUM_INDEX_COVER).toString();
    auto artist = indexValue(index, ALBUM_INDEX_ARTIST).toString();
    auto album_year = indexValue(index, ALBUM_INDEX_ALBUM_YEAR).toInt();
    auto is_hires = indexValue(index, ALBUM_INDEX_IS_HIRES).toBool();
    auto is_selected = indexValue(index, ALBUM_INDEX_IS_SELECTED).toBool();

    // Process edit album view 
    if (enable_album_view_ && enable_selected_mode_ && is_selected) {
        painter->save();
        QPainterPath path;
        path.addRoundedRect(selectedAlbumRect(option), 8, 8);
        if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
            painter->fillPath(path, qTheme.highlightColor().darker());
        }
        else {
            painter->fillPath(path, qTheme.hoverColor().darker());
        }
        painter->restore();
    }

    if (show_mode_ == SHOW_YEAR) {
        artist = album_year <= 0 ? qDatabaseFacade.unknown() : QString::number(album_year);
    }

    // Draw album and artist text

    auto album_artist_text_width = cover_size_.width();
    auto album_text_rect = albumTextRect(option, cover_size_, album_artist_text_width);
    auto artist_text_rect = artistTextRect(option, cover_size_);

    if (is_selected && qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        painter->setPen(QPen(Qt::white));
    }
    else {
        painter->setPen(QPen(album_text_color_));
    }

    auto f = painter->font();
    f.setPointSize(qTheme.fontSize(8));
    f.setBold(true);
    painter->setFont(f);

    // Draw hi res icon

    if (is_hires) {
        album_artist_text_width -= kMoreIconSize;
        painter->drawPixmap(hiResIconRect(option, cover_size_), qTheme.hdIcon().pixmap(QSize(kMoreIconSize, kMoreIconSize)));
    }

    QFontMetrics album_metrics(painter->font());
    painter->drawText(album_text_rect, Qt::AlignVCenter,
        album_metrics.elidedText(album, Qt::ElideRight, album_artist_text_width));

    if (is_selected && qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        QColor artist_color(Qt::gray);
        painter->setPen(QPen(artist_color.lighter()));
    }
    else {
        painter->setPen(QPen(Qt::gray));
    }

    f.setBold(false);
    painter->setFont(f);

    QFontMetrics artist_metrics(painter->font());
    painter->drawText(artist_text_rect, Qt::AlignVCenter,
        artist_metrics.elidedText(artist, Qt::ElideRight, cover_size_.width() - kMoreIconSize));

    // Perform search album cover
    // Note: Calculate view visible is not necessary, Qt already done it. 
    if (isNullOfEmpty(cover_id)) {
        auto visible_albums = getVisibleAlbumId(option);
        if (visible_albums.contains(album_id)) {
            emit findAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
        }
        painter->drawPixmap(coverRect(option, cover_size_), qTheme.unknownCover());
    }
    else {
        auto cover_rect = coverRect(option, cover_size_);
        auto cover = qImageCache.getOrDefault(kAlbumCacheTag, cover_id);
        Q_ASSERT(!cover.isNull());
        painter->drawPixmap(cover_rect, cover);
    }

    // Draw hit play button
    bool hit_play_button = false;
    if (enable_album_view_
        && show_mode_ != SHOW_NORMAL
        && option.state & QStyle::State_MouseOver
        && coverRect(option, cover_size_).contains(mouse_point_)) {
        painter->drawPixmap(coverRect(option, cover_size_), mask_image_);
        QStyleOptionButton play_button_style;
        play_button_style.rect = playButtonRect(option, cover_size_);
        play_button_style.icon = qTheme.playCircleIcon();
        play_button_style.state |= QStyle::State_Enabled;
        play_button_style.iconSize = QSize(kIconSize, kIconSize);
        style->drawControl(QStyle::CE_PushButton, &play_button_style, painter, play_button_.get());
        if (playButtonRect(option, cover_size_).contains(mouse_point_)) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            hit_play_button = true;
        }
    }

    // Draw edit mode checkbox

    if (enable_album_view_ && enable_selected_mode_) {
        QStyleOptionButton checkbox_style;
        edit_mode_checkbox_->setIconSize(QSize(kMoreIconSize, kMoreIconSize));
        checkbox_style.rect = checkBoxIconRect(option, cover_size_);
        if (is_selected) {
            checkbox_style.state |= QStyle::State_On;
        }
        else {
            checkbox_style.state |= QStyle::State_Off;
        }
        style->drawControl(QStyle::CE_CheckBox, &checkbox_style, painter, edit_mode_checkbox_.get());
    }

    QStyleOptionButton more_option_style;
    // Draw more button
    if (enable_album_view_
        && option.rect.contains(mouse_point_)
        && show_mode_ != SHOW_NORMAL) {
        more_option_style.initFrom(more_album_opt_button_.get());
        more_option_style.rect = moreButtonRect(option, cover_size_);
        more_option_style.icon = qTheme.fontIcon(Glyphs::ICON_MORE);
        more_option_style.state |= QStyle::State_Enabled;
        if (moreButtonRect(option, cover_size_).contains(mouse_point_)) {
            more_option_style.state |= QStyle::State_Sunken;
            painter->setPen(qTheme.hoverColor());
            painter->setBrush(QBrush(qTheme.hoverColor()));
            painter->drawEllipse(moreButtonRect(option, cover_size_));
        }
        emit stopRefreshCover();
    }

    if (more_album_opt_button_->isDefault()) {
        more_option_style.features = QStyleOptionButton::DefaultButton;
    }
    more_option_style.iconSize = QSize(kMoreIconSize, kMoreIconSize);
    style->drawControl(QStyle::CE_PushButton, &more_option_style, painter, more_album_opt_button_.get());

    if (!hit_play_button) {
        QApplication::restoreOverrideCursor();
    }
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    const auto default_cover = qTheme.defaultCoverSize();
    result.setWidth(default_cover.width() + 40);
    result.setHeight(default_cover.height() + 80);
    return result;
}

