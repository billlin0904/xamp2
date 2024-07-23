/****************************************************************************
** Meta object code from reading C++ file 'aboutpage.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/aboutpage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aboutpage.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSAboutPageENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSAboutPageENDCLASS = QtMocHelpers::stringData(
    "AboutPage",
    "CheckForUpdate",
    "",
    "RestartApp",
    "OnCreditsOrLicenseChecked",
    "checked",
    "onThemeChangedFinished",
    "ThemeColor",
    "theme_color",
    "OnUpdateNewVersion",
    "QVersionNumber",
    "version"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSAboutPageENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    0,   45,    2, 0x06,    2 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   46,    2, 0x0a,    3 /* Public */,
       6,    1,   49,    2, 0x0a,    5 /* Public */,
       9,    1,   52,    2, 0x0a,    7 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 10,   11,

       0        // eod
};

Q_CONSTINIT const QMetaObject AboutPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_CLASSAboutPageENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSAboutPageENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSAboutPageENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AboutPage, std::true_type>,
        // method 'CheckForUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'RestartApp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'OnCreditsOrLicenseChecked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onThemeChangedFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeColor, std::false_type>,
        // method 'OnUpdateNewVersion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVersionNumber &, std::false_type>
    >,
    nullptr
} };

void AboutPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AboutPage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->CheckForUpdate(); break;
        case 1: _t->RestartApp(); break;
        case 2: _t->OnCreditsOrLicenseChecked((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->onThemeChangedFinished((*reinterpret_cast< std::add_pointer_t<ThemeColor>>(_a[1]))); break;
        case 4: _t->OnUpdateNewVersion((*reinterpret_cast< std::add_pointer_t<QVersionNumber>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QVersionNumber >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AboutPage::*)();
            if (_t _q_method = &AboutPage::CheckForUpdate; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AboutPage::*)();
            if (_t _q_method = &AboutPage::RestartApp; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *AboutPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AboutPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSAboutPageENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int AboutPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void AboutPage::CheckForUpdate()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AboutPage::RestartApp()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
