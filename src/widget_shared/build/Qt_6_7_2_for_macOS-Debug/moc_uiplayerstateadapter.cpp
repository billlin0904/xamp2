/****************************************************************************
** Meta object code from reading C++ file 'uiplayerstateadapter.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/uiplayerstateadapter.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'uiplayerstateadapter.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSUIPlayerStateAdapterENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSUIPlayerStateAdapterENDCLASS = QtMocHelpers::stringData(
    "UIPlayerStateAdapter",
    "sampleTimeChanged",
    "",
    "stream_time",
    "stateChanged",
    "PlayerState",
    "play_state",
    "playbackError",
    "message",
    "deviceChanged",
    "DeviceState",
    "state",
    "volumeChanged",
    "vol",
    "fftResultChanged",
    "ComplexValarray",
    "result"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUIPlayerStateAdapterENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    1,   53,    2, 0x06,    3 /* Public */,
       7,    1,   56,    2, 0x06,    5 /* Public */,
       9,    2,   59,    2, 0x06,    7 /* Public */,
      12,    1,   64,    2, 0x06,   10 /* Public */,
      14,    1,   67,    2, 0x06,   12 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 10, QMetaType::QString,   11,    8,
    QMetaType::Void, QMetaType::Float,   13,
    QMetaType::Void, 0x80000000 | 15,   16,

       0        // eod
};

Q_CONSTINIT const QMetaObject UIPlayerStateAdapter::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUIPlayerStateAdapterENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUIPlayerStateAdapterENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUIPlayerStateAdapterENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UIPlayerStateAdapter, std::true_type>,
        // method 'sampleTimeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'stateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<PlayerState, std::false_type>,
        // method 'playbackError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'deviceChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DeviceState, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'volumeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        // method 'fftResultChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ComplexValarray const &, std::false_type>
    >,
    nullptr
} };

void UIPlayerStateAdapter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UIPlayerStateAdapter *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sampleTimeChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 1: _t->stateChanged((*reinterpret_cast< std::add_pointer_t<PlayerState>>(_a[1]))); break;
        case 2: _t->playbackError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->deviceChanged((*reinterpret_cast< std::add_pointer_t<DeviceState>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->volumeChanged((*reinterpret_cast< std::add_pointer_t<float>>(_a[1]))); break;
        case 5: _t->fftResultChanged((*reinterpret_cast< std::add_pointer_t<ComplexValarray>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UIPlayerStateAdapter::*)(double );
            if (_t _q_method = &UIPlayerStateAdapter::sampleTimeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UIPlayerStateAdapter::*)(PlayerState );
            if (_t _q_method = &UIPlayerStateAdapter::stateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UIPlayerStateAdapter::*)(const QString & );
            if (_t _q_method = &UIPlayerStateAdapter::playbackError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UIPlayerStateAdapter::*)(DeviceState , const QString & );
            if (_t _q_method = &UIPlayerStateAdapter::deviceChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UIPlayerStateAdapter::*)(float );
            if (_t _q_method = &UIPlayerStateAdapter::volumeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UIPlayerStateAdapter::*)(ComplexValarray const & );
            if (_t _q_method = &UIPlayerStateAdapter::fftResultChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *UIPlayerStateAdapter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UIPlayerStateAdapter::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSUIPlayerStateAdapterENDCLASS.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IPlaybackStateAdapter"))
        return static_cast< IPlaybackStateAdapter*>(this);
    return QObject::qt_metacast(_clname);
}

int UIPlayerStateAdapter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void UIPlayerStateAdapter::sampleTimeChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void UIPlayerStateAdapter::stateChanged(PlayerState _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void UIPlayerStateAdapter::playbackError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void UIPlayerStateAdapter::deviceChanged(DeviceState _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void UIPlayerStateAdapter::volumeChanged(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void UIPlayerStateAdapter::fftResultChanged(ComplexValarray const & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
