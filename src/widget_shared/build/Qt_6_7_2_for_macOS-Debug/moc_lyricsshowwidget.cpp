/****************************************************************************
** Meta object code from reading C++ file 'lyricsshowwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/lyricsshowwidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lyricsshowwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSLyricsShowWidgetENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSLyricsShowWidgetENDCLASS = QtMocHelpers::stringData(
    "LyricsShowWidget",
    "stop",
    "",
    "onSetLrc",
    "lrc",
    "trlyc",
    "onSetLrcTime",
    "int32_t",
    "length",
    "onSetLrcFont",
    "font",
    "onSetLrcHighLight",
    "color",
    "onSetLrcColor",
    "onAddFullLrc"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSLyricsShowWidgetENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x0a,    1 /* Public */,
       3,    2,   63,    2, 0x0a,    2 /* Public */,
       3,    1,   68,    2, 0x2a,    5 /* Public | MethodCloned */,
       6,    1,   71,    2, 0x0a,    7 /* Public */,
       9,    1,   74,    2, 0x0a,    9 /* Public */,
      11,    1,   77,    2, 0x0a,   11 /* Public */,
      13,    1,   80,    2, 0x0a,   13 /* Public */,
      14,    1,   83,    2, 0x0a,   15 /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    4,    5,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::QFont,   10,
    QMetaType::Void, QMetaType::QColor,   12,
    QMetaType::Void, QMetaType::QColor,   12,
    QMetaType::Void, QMetaType::QString,    4,

       0        // eod
};

Q_CONSTINIT const QMetaObject LyricsShowWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<WheelableWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSLyricsShowWidgetENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSLyricsShowWidgetENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSLyricsShowWidgetENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LyricsShowWidget, std::true_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSetLrc'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSetLrc'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSetLrcTime'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onSetLrcFont'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QFont &, std::false_type>,
        // method 'onSetLrcHighLight'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>,
        // method 'onSetLrcColor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>,
        // method 'onAddFullLrc'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void LyricsShowWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LyricsShowWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stop(); break;
        case 1: _t->onSetLrc((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->onSetLrc((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onSetLrcTime((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 4: _t->onSetLrcFont((*reinterpret_cast< std::add_pointer_t<QFont>>(_a[1]))); break;
        case 5: _t->onSetLrcHighLight((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1]))); break;
        case 6: _t->onSetLrcColor((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1]))); break;
        case 7: _t->onAddFullLrc((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *LyricsShowWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LyricsShowWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSLyricsShowWidgetENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return WheelableWidget::qt_metacast(_clname);
}

int LyricsShowWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = WheelableWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
