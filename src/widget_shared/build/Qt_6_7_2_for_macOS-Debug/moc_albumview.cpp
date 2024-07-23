/****************************************************************************
** Meta object code from reading C++ file 'albumview.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/albumview.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'albumview.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSAlbumViewPageENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSAlbumViewPageENDCLASS = QtMocHelpers::stringData(
    "AlbumViewPage",
    "clickedArtist",
    "",
    "artist",
    "cover_id",
    "int32_t",
    "artist_id",
    "leaveAlbumView",
    "onThemeChangedFinished",
    "ThemeColor",
    "theme_color",
    "onRetranslateUi"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSAlbumViewPageENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    3,   38,    2, 0x06,    1 /* Public */,
       7,    0,   45,    2, 0x106,    5 /* Public | MethodIsConst  */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    1,   46,    2, 0x0a,    6 /* Public */,
      11,    0,   49,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, 0x80000000 | 5,    3,    4,    6,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject AlbumViewPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_CLASSAlbumViewPageENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSAlbumViewPageENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSAlbumViewPageENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AlbumViewPage, std::true_type>,
        // method 'clickedArtist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'leaveAlbumView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onThemeChangedFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeColor, std::false_type>,
        // method 'onRetranslateUi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void AlbumViewPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AlbumViewPage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->clickedArtist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[3]))); break;
        case 1: _t->leaveAlbumView(); break;
        case 2: _t->onThemeChangedFinished((*reinterpret_cast< std::add_pointer_t<ThemeColor>>(_a[1]))); break;
        case 3: _t->onRetranslateUi(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AlbumViewPage::*)(const QString & , const QString & , int32_t );
            if (_t _q_method = &AlbumViewPage::clickedArtist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AlbumViewPage::*)() const;
            if (_t _q_method = &AlbumViewPage::leaveAlbumView; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *AlbumViewPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AlbumViewPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSAlbumViewPageENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int AlbumViewPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
    }
    return _id;
}

// SIGNAL 0
void AlbumViewPage::clickedArtist(const QString & _t1, const QString & _t2, int32_t _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AlbumViewPage::leaveAlbumView()const
{
    QMetaObject::activate(const_cast< AlbumViewPage *>(this), &staticMetaObject, 1, nullptr);
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSAlbumViewENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSAlbumViewENDCLASS = QtMocHelpers::stringData(
    "AlbumView",
    "addPlaylist",
    "",
    "int32_t",
    "playlist_id",
    "QList<int32_t>",
    "music_ids",
    "clickedArtist",
    "artist",
    "cover_id",
    "artist_id",
    "clickedAlbum",
    "album",
    "album_id",
    "removeAll",
    "extractFile",
    "file_path",
    "onRetranslateUi",
    "onThemeChangedFinished",
    "ThemeColor",
    "theme_color",
    "reload",
    "filterByArtistId",
    "hideWidget",
    "search",
    "keyword",
    "onThemeColorChanged",
    "backgroundColor",
    "color",
    "append",
    "file_name",
    "showMenu",
    "pt",
    "showAlbumViewMenu",
    "enterEvent",
    "QEnterEvent*",
    "event"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSAlbumViewENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  110,    2, 0x06,    1 /* Public */,
       7,    3,  115,    2, 0x06,    4 /* Public */,
      11,    3,  122,    2, 0x06,    8 /* Public */,
      14,    0,  129,    2, 0x06,   12 /* Public */,
      15,    2,  130,    2, 0x06,   13 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    0,  135,    2, 0x0a,   16 /* Public */,
      18,    1,  136,    2, 0x0a,   17 /* Public */,
      21,    0,  139,    2, 0x0a,   19 /* Public */,
      22,    1,  140,    2, 0x0a,   20 /* Public */,
      23,    0,  143,    2, 0x0a,   22 /* Public */,
      24,    1,  144,    2, 0x0a,   23 /* Public */,
      26,    2,  147,    2, 0x0a,   25 /* Public */,
      29,    1,  152,    2, 0x0a,   28 /* Public */,
      31,    1,  155,    2, 0x0a,   30 /* Public */,
      33,    1,  158,    2, 0x0a,   32 /* Public */,
      34,    1,  161,    2, 0x0a,   34 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, 0x80000000 | 3,    8,    9,   10,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 3, QMetaType::QString,   12,   13,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 3,   16,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QColor, QMetaType::QColor,   27,   28,
    QMetaType::Void, QMetaType::QString,   30,
    QMetaType::Void, QMetaType::QPoint,   32,
    QMetaType::Void, QMetaType::QPoint,   32,
    QMetaType::Void, 0x80000000 | 35,   36,

       0        // eod
};

Q_CONSTINIT const QMetaObject AlbumView::staticMetaObject = { {
    QMetaObject::SuperData::link<QListView::staticMetaObject>(),
    qt_meta_stringdata_CLASSAlbumViewENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSAlbumViewENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSAlbumViewENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AlbumView, std::true_type>,
        // method 'addPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<int32_t> &, std::false_type>,
        // method 'clickedArtist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'clickedAlbum'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'extractFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onRetranslateUi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onThemeChangedFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeColor, std::false_type>,
        // method 'reload'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'filterByArtistId'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'hideWidget'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'search'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onThemeColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        // method 'append'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'showMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'showAlbumViewMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'enterEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QEnterEvent *, std::false_type>
    >,
    nullptr
} };

void AlbumView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AlbumView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->addPlaylist((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<int32_t>>>(_a[2]))); break;
        case 1: _t->clickedArtist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[3]))); break;
        case 2: _t->clickedAlbum((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 3: _t->removeAll(); break;
        case 4: _t->extractFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        case 5: _t->onRetranslateUi(); break;
        case 6: _t->onThemeChangedFinished((*reinterpret_cast< std::add_pointer_t<ThemeColor>>(_a[1]))); break;
        case 7: _t->reload(); break;
        case 8: _t->filterByArtistId((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 9: _t->hideWidget(); break;
        case 10: _t->search((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onThemeColorChanged((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QColor>>(_a[2]))); break;
        case 12: _t->append((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->showMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 14: _t->showAlbumViewMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 15: _t->enterEvent((*reinterpret_cast< std::add_pointer_t<QEnterEvent*>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AlbumView::*)(int32_t , const QList<int32_t> & );
            if (_t _q_method = &AlbumView::addPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AlbumView::*)(const QString & , const QString & , int32_t );
            if (_t _q_method = &AlbumView::clickedArtist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AlbumView::*)(const QString & , int32_t , const QString & );
            if (_t _q_method = &AlbumView::clickedAlbum; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AlbumView::*)();
            if (_t _q_method = &AlbumView::removeAll; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AlbumView::*)(const QString & , int32_t );
            if (_t _q_method = &AlbumView::extractFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *AlbumView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AlbumView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSAlbumViewENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QListView::qt_metacast(_clname);
}

int AlbumView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void AlbumView::addPlaylist(int32_t _t1, const QList<int32_t> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AlbumView::clickedArtist(const QString & _t1, const QString & _t2, int32_t _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AlbumView::clickedAlbum(const QString & _t1, int32_t _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AlbumView::removeAll()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AlbumView::extractFile(const QString & _t1, int32_t _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
