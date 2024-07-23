/****************************************************************************
** Meta object code from reading C++ file 'filesystemservice.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/worker/filesystemservice.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'filesystemservice.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSFileSystemServiceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSFileSystemServiceENDCLASS = QtMocHelpers::stringData(
    "FileSystemService",
    "insertDatabase",
    "",
    "ForwardList<TrackInfo>",
    "result",
    "int32_t",
    "playlist_id",
    "readFileStart",
    "readCompleted",
    "readFilePath",
    "file_path",
    "readFileProgress",
    "progress",
    "foundFileCount",
    "size_t",
    "file_count",
    "remainingTimeEstimation",
    "total_work",
    "completed_work",
    "secs",
    "onExtractFile",
    "onSetWatchDirectory",
    "dir",
    "cancelRequested"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSFileSystemServiceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   74,    2, 0x06,    1 /* Public */,
       7,    0,   79,    2, 0x06,    4 /* Public */,
       8,    0,   80,    2, 0x06,    5 /* Public */,
       9,    1,   81,    2, 0x06,    6 /* Public */,
      11,    1,   84,    2, 0x06,    8 /* Public */,
      13,    1,   87,    2, 0x06,   10 /* Public */,
      16,    3,   90,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      20,    2,   97,    2, 0x0a,   16 /* Public */,
      21,    1,  102,    2, 0x0a,   19 /* Public */,
      23,    0,  105,    2, 0x0a,   21 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, 0x80000000 | 5,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14, 0x80000000 | 5,   17,   18,   19,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 5,   10,    6,
    QMetaType::Void, QMetaType::QString,   22,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject FileSystemService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSFileSystemServiceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSFileSystemServiceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSFileSystemServiceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FileSystemService, std::true_type>,
        // method 'insertDatabase'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ForwardList<TrackInfo> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'readFileStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'readCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'readFilePath'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'readFileProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'foundFileCount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<size_t, std::false_type>,
        // method 'remainingTimeEstimation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<size_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<size_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onExtractFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onSetWatchDirectory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'cancelRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void FileSystemService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FileSystemService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->insertDatabase((*reinterpret_cast< std::add_pointer_t<ForwardList<TrackInfo>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        case 1: _t->readFileStart(); break;
        case 2: _t->readCompleted(); break;
        case 3: _t->readFilePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->readFileProgress((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 5: _t->foundFileCount((*reinterpret_cast< std::add_pointer_t<size_t>>(_a[1]))); break;
        case 6: _t->remainingTimeEstimation((*reinterpret_cast< std::add_pointer_t<size_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<size_t>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[3]))); break;
        case 7: _t->onExtractFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        case 8: _t->onSetWatchDirectory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->cancelRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FileSystemService::*)(const ForwardList<TrackInfo> & , int32_t );
            if (_t _q_method = &FileSystemService::insertDatabase; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)();
            if (_t _q_method = &FileSystemService::readFileStart; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)();
            if (_t _q_method = &FileSystemService::readCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)(const QString & );
            if (_t _q_method = &FileSystemService::readFilePath; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)(int32_t );
            if (_t _q_method = &FileSystemService::readFileProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)(size_t );
            if (_t _q_method = &FileSystemService::foundFileCount; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (FileSystemService::*)(size_t , size_t , int32_t );
            if (_t _q_method = &FileSystemService::remainingTimeEstimation; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject *FileSystemService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FileSystemService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSFileSystemServiceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FileSystemService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void FileSystemService::insertDatabase(const ForwardList<TrackInfo> & _t1, int32_t _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FileSystemService::readFileStart()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void FileSystemService::readCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void FileSystemService::readFilePath(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void FileSystemService::readFileProgress(int32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void FileSystemService::foundFileCount(size_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void FileSystemService::remainingTimeEstimation(size_t _t1, size_t _t2, int32_t _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
