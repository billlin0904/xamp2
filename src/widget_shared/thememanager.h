//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

class XAMP_WIDGET_SHARED_EXPORT ThemeManager final : public QObject {
    Q_OBJECT
public:
    ThemeManager();

    const QPalette& palette() const {
        return palette_;
    }

    const QFont& defaultFont() const {
        return ui_font_;
    }

    const QPixmap& unknownCover() noexcept;

	const QPixmap& defaultSizeUnknownCover() noexcept;

    QString countryFlagFilePath(const QString& country_iso_code);
    
    QIcon applicationIcon() const;

    void setPlayOrPauseButton(QToolButton* playButton, bool is_playing);

    void setHeartButton(QToolButton* heartButton, bool press = false);

    void updateMaximumIcon(QToolButton* maxWinButton, bool is_maximum);

    void setBackgroundColor(QColor color);

    const QSize& defaultCoverSize() const noexcept;

    QSize cacheCoverSize() const noexcept;

    QSize albumCoverSize() const noexcept;

    QIcon playCircleIcon() const;

    QIcon playingIcon() const;

    QIcon hdIcon() const;

    QIcon cloudIcon() const;

    QPixmap githubIcon() const;

    QIcon playlistPlayingIcon(QSize icon_size, double scale_factor = 1.0) const;

    QIcon playlistPauseIcon(QSize icon_size, double scale_factor = 1.0) const;

    void setThemeColor(ThemeColor theme_color);    

    void setThemeQssFile();

    void setBackgroundColor(QWidget* widget);

    QLatin1String themeColorPath() const;

    QLatin1String themeColorPath(ThemeColor theme_color) const;

    void setMenuStyle(QWidget* menu);

    QColor textColor() const;

    QString backgroundColorString() const;

    QColor backgroundColor() const noexcept;

    QColor hoverColor() const;

    QColor highlightColor() const;

    QColor titleBarColor() const;

    QColor coverShadowColor() const;

    QColor indicatorColor() const;

    ThemeColor themeColor() const {
        return theme_color_;
    }

    bool isDarkTheme() const {
		return theme_color_ == ThemeColor::DARK_THEME;
    }

    QString linearGradientStyle() const;

    QSize tabIconSize() const;

    void setTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button);

    QIcon fontIcon(const Glyphs code, std::optional<ThemeColor> theme_color = std::nullopt);

    QIcon fontRawIconOption(const Glyphs code, const QVariantMap& options = QVariantMap());

    QIcon fontRawIcon(const Glyphs code);

    void setFrameBackgroundColor(QFrame* frame);

    void setMuted(QAbstractButton* button, bool is_muted);

    void setVolume(QSlider* slider, QAbstractButton* button, uint32_t volume);

    void setSliderTheme(QSlider* slider, bool enter = false);    

    void setAlbumNaviBarTheme(QListView* tab) const;

    QIcon connectTypeIcon(DeviceConnectType type);

    Glyphs connectTypeGlyphs(DeviceConnectType type) const;

    void setDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type);

    int32_t defaultFontSize() const;

    int32_t fontSize(int32_t base_size) const;    

    static QSize titleButtonIconSize();

    QFont formatFont() const;

    QFont uiFont() const;

    QFont displayFont() const;

    QFont monoFont() const;

    QFont debugFont() const;

    void setLineEditStyle(QLineEdit *line_edit, const QString &object_name);

    void setComboBoxStyle(QComboBox *combo_box, const QString &object_name);

    void setRecordIcon(QToolButton *record_button, bool is_recording);

    void setCancelRecordIcon(QToolButton *cancel_button);

signals:
    void themeChangedFinished(ThemeColor theme_color);

private:
    static QString fontNamePath(const QString& file_name);

    static int32_t titleBarIconHeight();

    QFont loadFonts();

    void installFileFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void installFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts);

    void setPalette();

    void setGoogleMaterialFontIcons();
    
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
#define qIconCache SharedSingleton<LruCache<QString, QIcon>>::GetInstance()
