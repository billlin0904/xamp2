//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPixmap>
#include <QIcon>
#include <QMenu>
#include <QFrame>

#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/fonticon.h>
#include <widget/themecolor.h>
#include <widget/playerorder.h>

class QAbstractButton;
class QPushButton;
class QToolButton;
class QSlider;
class QLabel;

namespace Ui {
class XampWindow;
}

class ThemeManager : public QObject {
    Q_OBJECT
public:
    friend class SharedSingleton<ThemeManager>;

    bool UseNativeWindow() const;

    const QPalette& palette() const {
        return palette_;
    }

    const QFont& DefaultFont() const {
        return ui_font_;
    }

    const QPixmap& UnknownCover() const noexcept;

	const QPixmap& DefaultSizeUnknownCover() const noexcept;

    QString GetCountryFlagFilePath(const QString& country_iso_code);
    
    QIcon GetApplicationIcon() const;

    void SetPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void SetBitPerfectButton(Ui::XampWindow& ui, bool enable);

    void SetWidgetStyle(Ui::XampWindow &ui);    

    const QSize& GetDefaultCoverSize() const noexcept;

    QSize GetCacheCoverSize() const noexcept;

    QSize GetAlbumCoverSize() const noexcept;

    QSize GetSaveCoverArtSize() const noexcept;

    QIcon GetPlayCircleIcon() const;

    QIcon GetHiResIcon() const;

    QIcon GetPlayingIcon() const;

    QPixmap GetGithubIcon() const;

    QIcon GetPlaylistPlayingIcon(QSize icon_size) const;

    QIcon PlaylistPauseIcon(QSize icon_size) const;

    void UpdateMaximumIcon(Ui::XampWindow& ui, bool is_maximum) const;

    void SetThemeIcon(Ui::XampWindow& ui) const;

    void SetShufflePlayOrder(Ui::XampWindow& ui) const;

    void SetRepeatOnePlayOrder(Ui::XampWindow& ui) const;

    void SetRepeatOncePlayOrder(Ui::XampWindow& ui) const;

    void SetThemeColor(ThemeColor theme_color);

    void SetBackgroundColor(Ui::XampWindow& ui, QColor color);

    void LoadAndApplyQssTheme();

    void SetBackgroundColor(QWidget* widget);

    QLatin1String GetThemeColorPath() const;

    void SetMenuStyle(QWidget* menu);

    QColor GetThemeTextColor() const;

    QString BackgroundColorString() const;

    QColor BackgroundColor() const noexcept;

    QColor GetHoverColor() const;

    QColor GetHighlightColor() const;

    QColor GetTitleBarColor() const;

    QColor GetCoverShadowColor() const;

    ThemeColor GetThemeColor() const {
        return theme_color_;
    }

    QSize GetTabIconSize() const;

    void SetTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const;

    QIcon GetFontIcon(const char32_t code) const;

    void SetTextSeparator(QFrame* frame);

    void SetMuted(Ui::XampWindow& ui, bool is_muted);

    void SetMuted(QAbstractButton* button, bool is_muted);

    void SetVolume(QSlider* slider, QAbstractButton* button, uint32_t volume);

    void SetSliderTheme(QSlider* slider, bool enter = false);

    void SetSliderBarTheme(Ui::XampWindow& ui);

    QIcon GetConnectTypeIcon(DeviceConnectType type) const;

    Glyphs GetConnectTypeGlyphs(DeviceConnectType type) const;

    void SetDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type);

    int32_t GetDefaultFontSize() const;

    int32_t GetFontSize(int32_t base_size) const;

    void SetRepeatButtonIcon(Ui::XampWindow& ui, PlayerOrder order);

    static QSize GetTitleButtonIconSize();
signals:
    void CurrentThemeChanged(ThemeColor theme_color);

private:
    static qreal GetPixelRatio();

    static QString GetFontNamePath(const QString& file_name);

    static int32_t GetTitleBarIconHeight();

    void CollectAllLabel();

    QFont LoadFonts();

    void InstallFileFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void InstallFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts);

    void SetPalette();

    void SetFontAwesomeIcons();

    ThemeManager();
    
    bool use_native_window_;
    qreal width_ratio_;
    qreal height_ratio_;
    qreal font_ratio_;
    QSize base_size_;
    ThemeColor theme_color_;
    QVariantMap font_icon_opts_;
    QSize album_cover_size_;
    QSize cover_size_;
    QSize cache_cover_size_;
    QSize save_cover_art_size_;
    QColor background_color_;
    QPalette palette_;
    QFont ui_font_;
    QPixmap unknown_cover_;
    QPixmap default_size_unknown_cover_;
    QList<QLabel*> all_labels_;
};

#define qTheme SharedSingleton<ThemeManager>::GetInstance()

