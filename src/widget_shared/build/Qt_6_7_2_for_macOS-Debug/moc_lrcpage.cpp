/****************************************************************************
** Meta object code from reading C++ file 'lrcpage.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/lrcpage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lrcpage.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSLrcPageENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSLrcPageENDCLASS = QtMocHelpers::stringData(
    "LrcPage",
    "onThemeChangedFinished",
    "",
    "ThemeColor",
    "theme_color",
    "onThemeColorChanged",
    "background_color",
    "color",
    "setBackground",
    "cover",
    "clearBackground",
    "disappearBgProg",
    "appearBgProg"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSLrcPageENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       2,   50, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   38,    2, 0x0a,    3 /* Public */,
       5,    2,   41,    2, 0x0a,    5 /* Public */,
       8,    1,   46,    2, 0x0a,    8 /* Public */,
      10,    0,   49,    2, 0x0a,   10 /* Public */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QColor, QMetaType::QColor,    6,    7,
    QMetaType::Void, QMetaType::QImage,    9,
    QMetaType::Void,

 // properties: name, type, flags
      11, QMetaType::Int, 0x00015003, uint(-1), 0,
      12, QMetaType::Int, 0x00015003, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject LrcPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_CLASSLrcPageENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSLrcPageENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSLrcPageENDCLASS_t,
        // property 'disappearBgProg'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'appearBgProg'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LrcPage, std::true_type>,
        // method 'onThemeChangedFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeColor, std::false_type>,
        // method 'onThemeColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        // method 'setBackground'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'clearBackground'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void LrcPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LrcPage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onThemeChangedFinished((*reinterpret_cast< std::add_pointer_t<ThemeColor>>(_a[1]))); break;
        case 1: _t->onThemeColorChanged((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QColor>>(_a[2]))); break;
        case 2: _t->setBackground((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 3: _t->clearBackground(); break;
        default: ;
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<LrcPage *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->getDisappearBgProgress(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->getAppearBgProgress(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<LrcPage *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setDisappearBgProgress(*reinterpret_cast< int*>(_v)); break;
        case 1: _t->setAppearBgProgress(*reinterpret_cast< int*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *LrcPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LrcPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSLrcPageENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int LrcPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
