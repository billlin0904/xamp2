/*
###############################################################################
#                                                                             #
# The MIT License                                                             #
#                                                                             #
# Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)          #
#               >> https://github.com/Jorgen-VikingGod                        #
#                                                                             #
# Sources: https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle  #
#                                                                             #
###############################################################################
*/

#include <widget/str_utilts.h>
#include "DarkStyle.h"

DarkStyle::DarkStyle() : DarkStyle(styleBase()) {}

DarkStyle::DarkStyle(QStyle *style) : QProxyStyle(style) {}

QStyle *DarkStyle::styleBase(QStyle *style) const {
    static QStyle *base =
        !style ? QStyleFactory::create(QStringLiteral("Fusion")) : style;
    return base;
}

QStyle *DarkStyle::baseStyle() const { return styleBase(); }

void DarkStyle::polish(QPalette &palette) {
    // modify palette to dark
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor(127, 127, 127));
    palette.setColor(QPalette::Base, QColor(42, 42, 42));
    palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    palette.setColor(QPalette::Dark, QColor(35, 35, 35));
    palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor(127, 127, 127));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(127, 127, 127));
}

void DarkStyle::initFallbackFont() {
    QList<QString> fallback_fonts;

#ifdef Q_OS_WIN
    fallback_fonts.append(Q_UTF8("Segoe UI"));
    fallback_fonts.append(Q_UTF8("Segoe UI Bold"));
    fallback_fonts.append(Q_UTF8("Microsoft Yahei UI"));
    fallback_fonts.append(Q_UTF8("Microsoft Yahei UI Bold"));
    fallback_fonts.append(Q_UTF8("Meiryo UI"));
    fallback_fonts.append(Q_UTF8("Meiryo UI Bold"));
    fallback_fonts.append(Q_UTF8("Arial"));
#else
    fallback_fonts.append(Q_UTF8("SF Pro Display"));
    fallback_fonts.append(Q_UTF8("SF Pro Text"));
    fallback_fonts.append(Q_UTF8("Helvetica Neue"));
    fallback_fonts.append(Q_UTF8("Helvetica"));
#endif
    QFont::insertSubstitutions(Q_UTF8("UI"), fallback_fonts);
}

void DarkStyle::polish(QApplication *app) {
    if (!app) return;

    initFallbackFont();

    QFile qfDarkstyle(QStringLiteral(":/darkstyle/darkstyle.qss"));
    if (qfDarkstyle.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString qsStylesheet = QString::fromLatin1(qfDarkstyle.readAll());
        app->setStyleSheet(qsStylesheet);
        qfDarkstyle.close();
    }
}
