//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMenu>
#include <QFrame>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/fonticon.h>
#include <widget/themecolor.h>

class QAbstractButton;
class QPushButton;
class QToolButton;
class QSlider;
class QLabel;
class QListView;
class QSlider;
class QLineEdit;
class QToolButton;
class QComboBox;

class XAMP_WIDGET_SHARED_EXPORT ThemeManager : public QObject {
    Q_OBJECT
public:
    friend class SharedSingleton<ThemeManager>;

    const QPalette& GetPalette() const {
        return palette_;
    }

    const QFont& GetDefaultFont() const {
        return ui_font_;
    }

    const QPixmap& GetUnknownCover() const noexcept;

	const QPixmap& DefaultSizeUnknownCover() const noexcept;

    QString GetCountryFlagFilePath(const QString& country_iso_code);
    
    QIcon GetApplicationIcon() const;

    void SetPlayOrPauseButton(QToolButton* playButton, bool is_playing);

    void SetBitPerfectButton(QToolButton* bitPerfectButton, bool enable);

    void SetHeartButton(QToolButton* heartButton, bool press = false);

    void UpdateMaximumIcon(QToolButton* maxWinButton, bool is_maximum) const;

    void SetBackgroundColor(QColor color);

    const QSize& GetDefaultCoverSize() const noexcept;

    QSize GetCacheCoverSize() const noexcept;

    QSize GetAlbumCoverSize() const noexcept;

    QIcon GetPlayCircleIcon() const;

    QIcon GetHiResIcon() const;

    QIcon GetPlayingIcon() const;

    QPixmap GetGithubIcon() const;

    QIcon GetPlaylistPlayingIcon(QSize icon_size) const;

    QIcon PlaylistPauseIcon(QSize icon_size) const;    

    void SetThemeColor(ThemeColor theme_color);    

    void LoadAndApplyQssTheme();

    void SetBackgroundColor(QWidget* widget);

    QLatin1String GetThemeColorPath() const;

    QLatin1String GetThemeColorPath(ThemeColor theme_color) const;

    void SetMenuStyle(QWidget* menu);

    QColor GetThemeTextColor() const;

    QString GetBackgroundColorString() const;

    QColor GetBackgroundColor() const noexcept;

    QColor GetHoverColor() const;

    QColor GetHighlightColor() const;

    QColor GetTitleBarColor() const;

    QColor GetCoverShadowColor() const;

    ThemeColor GetThemeColor() const {
        return theme_color_;
    }

    void SetLinearGradient(QLinearGradient &gradient) const;

    QString GetLinearGradientStyle() const;

    QSize GetTabIconSize() const;

    void SetTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const;

    QIcon GetFontIcon(const char32_t code, std::optional<ThemeColor> theme_color = std::nullopt) const;

    QIcon GetFontIcon(const char32_t& code, QVariantMap options);

    void SetTextSeparator(QFrame* frame);    

    void SetMuted(QAbstractButton* button, bool is_muted);

    void SetVolume(QSlider* slider, QAbstractButton* button, uint32_t volume);

    void SetSliderTheme(QSlider* slider, bool enter = false);    

    void SetAlbumNaviBarTheme(QListView* tab) const;

    QIcon GetConnectTypeIcon(DeviceConnectType type) const;

    Glyphs GetConnectTypeGlyphs(DeviceConnectType type) const;

    void SetDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type);

    int32_t GetDefaultFontSize() const;

    int32_t GetFontSize(int32_t base_size) const;    

    static QSize GetTitleButtonIconSize();

    QFont GetFormatFont() const;

    QFont GetUiFont() const;

    QFont GetDisplayFont() const;

    QFont GetMonoFont() const;

    QFont GetDebugFont() const;

    void SetLineEditStyle(QLineEdit *line_edit, const QString &object_name);

    void SetComboBoxStyle(QComboBox *combo_box, const QString &object_name);
signals:
    void CurrentThemeChanged(ThemeColor theme_color);

private:
    static QString GetFontNamePath(const QString& file_name);

    static int32_t GetTitleBarIconHeight();

    QFont LoadFonts();

    void InstallFileFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void InstallFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts);

    void SetPalette();

    void SetFontAwesomeIcons();

    ThemeManager();
    
    qreal font_ratio_;
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
};

#define qTheme SharedSingleton<ThemeManager>::GetInstance()

