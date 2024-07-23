/****************************************************************************
** Meta object code from reading C++ file 'artistinfopage.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/artistinfopage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'artistinfopage.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSArtistInfoPageENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSArtistInfoPageENDCLASS = QtMocHelpers::stringData(
    "ArtistInfoPage",
    "onThemeColorChanged",
    "",
    "background_color",
    "color",
    "setArtistId",
    "artist",
    "cover_id",
    "int32_t",
    "artist_id",
    "setAlbumCount",
    "album_count",
    "setTracks",
    "tracks",
    "setTotalDuration",
    "durations"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSArtistInfoPageENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   44,    2, 0x0a,    1 /* Public */,
       5,    3,   49,    2, 0x0a,    4 /* Public */,
      10,    1,   56,    2, 0x0a,    8 /* Public */,
      12,    1,   59,    2, 0x0a,   10 /* Public */,
      14,    1,   62,    2, 0x0a,   12 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor, QMetaType::QColor,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, 0x80000000 | 8,    6,    7,    9,
    QMetaType::Void, 0x80000000 | 8,   11,
    QMetaType::Void, 0x80000000 | 8,   13,
    QMetaType::Void, QMetaType::Double,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject ArtistInfoPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_CLASSArtistInfoPageENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSArtistInfoPageENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSArtistInfoPageENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ArtistInfoPage, std::true_type>,
        // method 'onThemeColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        // method 'setArtistId'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'setAlbumCount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'setTracks'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'setTotalDuration'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>
    >,
    nullptr
} };

void ArtistInfoPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ArtistInfoPage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onThemeColorChanged((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QColor>>(_a[2]))); break;
        case 1: _t->setArtistId((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[3]))); break;
        case 2: _t->setAlbumCount((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 3: _t->setTracks((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 4: _t->setTotalDuration((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *ArtistInfoPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ArtistInfoPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSArtistInfoPageENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int ArtistInfoPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
