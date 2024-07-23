/****************************************************************************
** Meta object code from reading C++ file 'playlisttabwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/playlisttabwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playlisttabwidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSPlaylistTabWidgetENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSPlaylistTabWidgetENDCLASS = QtMocHelpers::stringData(
    "PlaylistTabWidget",
    "createNewPlaylist",
    "",
    "createCloudPlaylist",
    "removeAllPlaylist",
    "reloadAllPlaylist",
    "reloadPlaylist",
    "int32_t",
    "tab_index",
    "deletePlaylist",
    "playlist_id",
    "saveToM3UFile",
    "playlist_name",
    "loadPlaylistFile",
    "onThemeChangedFinished",
    "ThemeColor",
    "theme_color",
    "onRetranslateUi"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSPlaylistTabWidgetENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x06,    1 /* Public */,
       3,    0,   75,    2, 0x06,    2 /* Public */,
       4,    0,   76,    2, 0x06,    3 /* Public */,
       5,    0,   77,    2, 0x06,    4 /* Public */,
       6,    1,   78,    2, 0x06,    5 /* Public */,
       9,    1,   81,    2, 0x06,    7 /* Public */,
      11,    2,   84,    2, 0x06,    9 /* Public */,
      13,    1,   89,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      14,    1,   92,    2, 0x0a,   14 /* Public */,
      17,    0,   95,    2, 0x0a,   16 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, 0x80000000 | 7, QMetaType::QString,   10,   12,
    QMetaType::Void, 0x80000000 | 7,   10,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlaylistTabWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QTabWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSPlaylistTabWidgetENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSPlaylistTabWidgetENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSPlaylistTabWidgetENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaylistTabWidget, std::true_type>,
        // method 'createNewPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createCloudPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeAllPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reloadAllPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reloadPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'deletePlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'saveToM3UFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadPlaylistFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onThemeChangedFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeColor, std::false_type>,
        // method 'onRetranslateUi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PlaylistTabWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PlaylistTabWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->createNewPlaylist(); break;
        case 1: _t->createCloudPlaylist(); break;
        case 2: _t->removeAllPlaylist(); break;
        case 3: _t->reloadAllPlaylist(); break;
        case 4: _t->reloadPlaylist((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 5: _t->deletePlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->saveToM3UFile((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->loadPlaylistFile((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 8: _t->onThemeChangedFinished((*reinterpret_cast< std::add_pointer_t<ThemeColor>>(_a[1]))); break;
        case 9: _t->onRetranslateUi(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PlaylistTabWidget::*)();
            if (_t _q_method = &PlaylistTabWidget::createNewPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)();
            if (_t _q_method = &PlaylistTabWidget::createCloudPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)();
            if (_t _q_method = &PlaylistTabWidget::removeAllPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)();
            if (_t _q_method = &PlaylistTabWidget::reloadAllPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)(int32_t );
            if (_t _q_method = &PlaylistTabWidget::reloadPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)(const QString & );
            if (_t _q_method = &PlaylistTabWidget::deletePlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)(int32_t , const QString & );
            if (_t _q_method = &PlaylistTabWidget::saveToM3UFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (PlaylistTabWidget::*)(int32_t );
            if (_t _q_method = &PlaylistTabWidget::loadPlaylistFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
    }
}

const QMetaObject *PlaylistTabWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaylistTabWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSPlaylistTabWidgetENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QTabWidget::qt_metacast(_clname);
}

int PlaylistTabWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void PlaylistTabWidget::createNewPlaylist()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PlaylistTabWidget::createCloudPlaylist()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PlaylistTabWidget::removeAllPlaylist()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PlaylistTabWidget::reloadAllPlaylist()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PlaylistTabWidget::reloadPlaylist(int32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PlaylistTabWidget::deletePlaylist(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void PlaylistTabWidget::saveToM3UFile(int32_t _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void PlaylistTabWidget::loadPlaylistFile(int32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
