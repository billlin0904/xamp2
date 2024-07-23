/****************************************************************************
** Meta object code from reading C++ file 'backgroundservice.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/worker/backgroundservice.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'backgroundservice.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSBackgroundServiceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSBackgroundServiceENDCLASS = QtMocHelpers::stringData(
    "BackgroundService",
    "readFileStart",
    "",
    "readCompleted",
    "readFilePath",
    "file_path",
    "readFileProgress",
    "int32_t",
    "progress",
    "foundFileCount",
    "size_t",
    "file_count",
    "readReplayGain",
    "playlistId",
    "PlayListEntity",
    "entity",
    "ReplayGain",
    "replay_gain",
    "blurImage",
    "image",
    "dominantColor",
    "color",
    "readCdTrackInfo",
    "disc_id",
    "ForwardList<TrackInfo>",
    "track_infos",
    "fetchMbDiscInfoCompleted",
    "MbDiscIdInfo",
    "mb_disc_id_info",
    "fetchDiscCoverCompleted",
    "cover_id",
    "fetchLyricsCompleted",
    "music_id",
    "lyrics",
    "trlyrics",
    "fetchArtistCompleted",
    "artist",
    "translationCompleted",
    "keyword",
    "result"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSBackgroundServiceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,    1 /* Public */,
       3,    0,   99,    2, 0x06,    2 /* Public */,
       4,    1,  100,    2, 0x06,    3 /* Public */,
       6,    1,  103,    2, 0x06,    5 /* Public */,
       9,    1,  106,    2, 0x06,    7 /* Public */,
      12,    3,  109,    2, 0x06,    9 /* Public */,
      18,    1,  116,    2, 0x06,   13 /* Public */,
      20,    1,  119,    2, 0x06,   15 /* Public */,
      22,    2,  122,    2, 0x06,   17 /* Public */,
      26,    1,  127,    2, 0x06,   20 /* Public */,
      29,    2,  130,    2, 0x06,   22 /* Public */,
      31,    3,  135,    2, 0x06,   25 /* Public */,
      35,    2,  142,    2, 0x06,   29 /* Public */,
      37,    2,  147,    2, 0x06,   32 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 14, 0x80000000 | 16,   13,   15,   17,
    QMetaType::Void, QMetaType::QImage,   19,
    QMetaType::Void, QMetaType::QColor,   21,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 24,   23,   25,
    QMetaType::Void, 0x80000000 | 27,   28,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   23,   30,
    QMetaType::Void, 0x80000000 | 7, QMetaType::QString, QMetaType::QString,   32,   33,   34,
    QMetaType::Void, QMetaType::QString, QMetaType::QByteArray,   36,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   38,   39,

       0        // eod
};

Q_CONSTINIT const QMetaObject BackgroundService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSBackgroundServiceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSBackgroundServiceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSBackgroundServiceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BackgroundService, std::true_type>,
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
        // method 'readReplayGain'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ReplayGain &, std::false_type>,
        // method 'blurImage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'dominantColor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>,
        // method 'readCdTrackInfo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ForwardList<TrackInfo> &, std::false_type>,
        // method 'fetchMbDiscInfoCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MbDiscIdInfo &, std::false_type>,
        // method 'fetchDiscCoverCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'fetchLyricsCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'fetchArtistCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'translationCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void BackgroundService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BackgroundService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->readFileStart(); break;
        case 1: _t->readCompleted(); break;
        case 2: _t->readFilePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->readFileProgress((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1]))); break;
        case 4: _t->foundFileCount((*reinterpret_cast< std::add_pointer_t<size_t>>(_a[1]))); break;
        case 5: _t->readReplayGain((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ReplayGain>>(_a[3]))); break;
        case 6: _t->blurImage((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 7: _t->dominantColor((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1]))); break;
        case 8: _t->readCdTrackInfo((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ForwardList<TrackInfo>>>(_a[2]))); break;
        case 9: _t->fetchMbDiscInfoCompleted((*reinterpret_cast< std::add_pointer_t<MbDiscIdInfo>>(_a[1]))); break;
        case 10: _t->fetchDiscCoverCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->fetchLyricsCompleted((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 12: _t->fetchArtistCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[2]))); break;
        case 13: _t->translationCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            case 2:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ReplayGain >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< MbDiscIdInfo >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BackgroundService::*)();
            if (_t _q_method = &BackgroundService::readFileStart; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)();
            if (_t _q_method = &BackgroundService::readCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QString & );
            if (_t _q_method = &BackgroundService::readFilePath; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(int32_t );
            if (_t _q_method = &BackgroundService::readFileProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(size_t );
            if (_t _q_method = &BackgroundService::foundFileCount; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(int32_t , const PlayListEntity & , const ReplayGain & );
            if (_t _q_method = &BackgroundService::readReplayGain; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QImage & );
            if (_t _q_method = &BackgroundService::blurImage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QColor & );
            if (_t _q_method = &BackgroundService::dominantColor; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QString & , const ForwardList<TrackInfo> & );
            if (_t _q_method = &BackgroundService::readCdTrackInfo; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const MbDiscIdInfo & );
            if (_t _q_method = &BackgroundService::fetchMbDiscInfoCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QString & , const QString & );
            if (_t _q_method = &BackgroundService::fetchDiscCoverCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(int32_t , const QString & , const QString & );
            if (_t _q_method = &BackgroundService::fetchLyricsCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QString & , const QByteArray & );
            if (_t _q_method = &BackgroundService::fetchArtistCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (BackgroundService::*)(const QString & , const QString & );
            if (_t _q_method = &BackgroundService::translationCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
    }
}

const QMetaObject *BackgroundService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BackgroundService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSBackgroundServiceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BackgroundService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void BackgroundService::readFileStart()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void BackgroundService::readCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BackgroundService::readFilePath(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void BackgroundService::readFileProgress(int32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void BackgroundService::foundFileCount(size_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void BackgroundService::readReplayGain(int32_t _t1, const PlayListEntity & _t2, const ReplayGain & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void BackgroundService::blurImage(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void BackgroundService::dominantColor(const QColor & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void BackgroundService::readCdTrackInfo(const QString & _t1, const ForwardList<TrackInfo> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void BackgroundService::fetchMbDiscInfoCompleted(const MbDiscIdInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void BackgroundService::fetchDiscCoverCompleted(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void BackgroundService::fetchLyricsCompleted(int32_t _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void BackgroundService::fetchArtistCompleted(const QString & _t1, const QByteArray & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void BackgroundService::translationCompleted(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
QT_WARNING_POP
