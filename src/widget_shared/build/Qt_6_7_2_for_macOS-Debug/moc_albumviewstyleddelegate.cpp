/****************************************************************************
** Meta object code from reading C++ file 'albumviewstyleddelegate.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/albumviewstyleddelegate.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'albumviewstyleddelegate.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSAlbumViewStyledDelegateENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSAlbumViewStyledDelegateENDCLASS = QtMocHelpers::stringData(
    "AlbumViewStyledDelegate",
    "enterAlbumView",
    "",
    "QModelIndex",
    "index",
    "editAlbumView",
    "state",
    "showAlbumMenu",
    "pt",
    "findAlbumCover",
    "DatabaseCoverId",
    "id",
    "stopRefreshCover"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSAlbumViewStyledDelegateENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x106,    1 /* Public | MethodIsConst  */,
       5,    2,   47,    2, 0x106,    3 /* Public | MethodIsConst  */,
       7,    2,   52,    2, 0x106,    6 /* Public | MethodIsConst  */,
       9,    1,   57,    2, 0x106,    9 /* Public | MethodIsConst  */,
      12,    0,   60,    2, 0x106,   11 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool,    4,    6,
    QMetaType::Void, 0x80000000 | 3, QMetaType::QPoint,    4,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject AlbumViewStyledDelegate::staticMetaObject = { {
    QMetaObject::SuperData::link<QStyledItemDelegate::staticMetaObject>(),
    qt_meta_stringdata_CLASSAlbumViewStyledDelegateENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSAlbumViewStyledDelegateENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSAlbumViewStyledDelegateENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AlbumViewStyledDelegate, std::true_type>,
        // method 'enterAlbumView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        // method 'editAlbumView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'showAlbumMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'findAlbumCover'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const DatabaseCoverId &, std::false_type>,
        // method 'stopRefreshCover'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void AlbumViewStyledDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AlbumViewStyledDelegate *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->enterAlbumView((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 1: _t->editAlbumView((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 2: _t->showAlbumMenu((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 3: _t->findAlbumCover((*reinterpret_cast< std::add_pointer_t<DatabaseCoverId>>(_a[1]))); break;
        case 4: _t->stopRefreshCover(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< DatabaseCoverId >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AlbumViewStyledDelegate::*)(const QModelIndex & ) const;
            if (_t _q_method = &AlbumViewStyledDelegate::enterAlbumView; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AlbumViewStyledDelegate::*)(const QModelIndex & , bool ) const;
            if (_t _q_method = &AlbumViewStyledDelegate::editAlbumView; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AlbumViewStyledDelegate::*)(const QModelIndex & , const QPoint & ) const;
            if (_t _q_method = &AlbumViewStyledDelegate::showAlbumMenu; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AlbumViewStyledDelegate::*)(const DatabaseCoverId & ) const;
            if (_t _q_method = &AlbumViewStyledDelegate::findAlbumCover; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AlbumViewStyledDelegate::*)() const;
            if (_t _q_method = &AlbumViewStyledDelegate::stopRefreshCover; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *AlbumViewStyledDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AlbumViewStyledDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSAlbumViewStyledDelegateENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QStyledItemDelegate::qt_metacast(_clname);
}

int AlbumViewStyledDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStyledItemDelegate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void AlbumViewStyledDelegate::enterAlbumView(const QModelIndex & _t1)const
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(const_cast< AlbumViewStyledDelegate *>(this), &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AlbumViewStyledDelegate::editAlbumView(const QModelIndex & _t1, bool _t2)const
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(const_cast< AlbumViewStyledDelegate *>(this), &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AlbumViewStyledDelegate::showAlbumMenu(const QModelIndex & _t1, const QPoint & _t2)const
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(const_cast< AlbumViewStyledDelegate *>(this), &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AlbumViewStyledDelegate::findAlbumCover(const DatabaseCoverId & _t1)const
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(const_cast< AlbumViewStyledDelegate *>(this), &staticMetaObject, 3, _a);
}

// SIGNAL 4
void AlbumViewStyledDelegate::stopRefreshCover()const
{
    QMetaObject::activate(const_cast< AlbumViewStyledDelegate *>(this), &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
