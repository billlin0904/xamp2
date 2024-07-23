/****************************************************************************
** Meta object code from reading C++ file 'ytmusicoauth.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/youtubedl/ytmusicoauth.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ytmusicoauth.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSYtMusicOAuthENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSYtMusicOAuthENDCLASS = QtMocHelpers::stringData(
    "YtMusicOAuth",
    "requestGrantCompleted",
    "",
    "acceptAuthorization",
    "url",
    "setupError",
    "requestGrantError"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSYtMusicOAuthENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,
       3,    1,   39,    2, 0x06,    2 /* Public */,
       5,    0,   42,    2, 0x06,    4 /* Public */,
       6,    0,   43,    2, 0x06,    5 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject YtMusicOAuth::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSYtMusicOAuthENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSYtMusicOAuthENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSYtMusicOAuthENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<YtMusicOAuth, std::true_type>,
        // method 'requestGrantCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'acceptAuthorization'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setupError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestGrantError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void YtMusicOAuth::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<YtMusicOAuth *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->requestGrantCompleted(); break;
        case 1: _t->acceptAuthorization((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->setupError(); break;
        case 3: _t->requestGrantError(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (YtMusicOAuth::*)();
            if (_t _q_method = &YtMusicOAuth::requestGrantCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (YtMusicOAuth::*)(const QString & );
            if (_t _q_method = &YtMusicOAuth::acceptAuthorization; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (YtMusicOAuth::*)();
            if (_t _q_method = &YtMusicOAuth::setupError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (YtMusicOAuth::*)();
            if (_t _q_method = &YtMusicOAuth::requestGrantError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *YtMusicOAuth::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *YtMusicOAuth::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSYtMusicOAuthENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int YtMusicOAuth::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void YtMusicOAuth::requestGrantCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void YtMusicOAuth::acceptAuthorization(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void YtMusicOAuth::setupError()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void YtMusicOAuth::requestGrantError()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
