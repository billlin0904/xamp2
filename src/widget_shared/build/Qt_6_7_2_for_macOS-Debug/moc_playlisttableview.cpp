/****************************************************************************
** Meta object code from reading C++ file 'playlisttableview.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget/playlisttableview.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playlisttableview.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSPlaylistTableViewENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSPlaylistTableViewENDCLASS = QtMocHelpers::stringData(
    "PlaylistTableView",
    "updatePlayingState",
    "",
    "PlayListEntity",
    "entity",
    "PlayingState",
    "playing_state",
    "playMusic",
    "int32_t",
    "playlist_id",
    "item",
    "is_plays",
    "is_doubleclicked",
    "encodeFlacFile",
    "encodeAacFile",
    "EncodingProfile",
    "profile",
    "encodeWavFile",
    "downloadFile",
    "readReplayGain",
    "QList<PlayListEntity>",
    "entities",
    "editTags",
    "updateAlbumCover",
    "cover_id",
    "addPlaylistItemFinished",
    "extractFile",
    "file_path",
    "addToPlaylist",
    "source_playlist_id",
    "std::vector<std::string>",
    "video_ids",
    "removePlaylistItems",
    "likeSong",
    "like",
    "addPlaylist",
    "onPlayIndex",
    "QModelIndex",
    "index",
    "is_play",
    "onProcessDatabase",
    "onProcessTrackInfo",
    "total_album",
    "total_tracks",
    "onThemeColorChanged",
    "backgroundColor",
    "color",
    "onReloadEntity",
    "onUpdateReplayGain",
    "playlistId",
    "ReplayGain",
    "replay_gain",
    "onRetranslateUi"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSPlaylistTableViewENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      24,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      15,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  158,    2, 0x06,    1 /* Public */,
       7,    4,  163,    2, 0x06,    4 /* Public */,
      13,    1,  172,    2, 0x06,    9 /* Public */,
      14,    2,  175,    2, 0x06,   11 /* Public */,
      17,    1,  180,    2, 0x06,   14 /* Public */,
      18,    1,  183,    2, 0x06,   16 /* Public */,
      19,    2,  186,    2, 0x06,   18 /* Public */,
      22,    2,  191,    2, 0x06,   21 /* Public */,
      23,    1,  196,    2, 0x06,   24 /* Public */,
      25,    0,  199,    2, 0x06,   26 /* Public */,
      26,    2,  200,    2, 0x06,   27 /* Public */,
      28,    3,  205,    2, 0x06,   30 /* Public */,
      32,    2,  212,    2, 0x06,   34 /* Public */,
      33,    2,  217,    2, 0x06,   37 /* Public */,
      35,    2,  222,    2, 0x06,   40 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      36,    3,  227,    2, 0x0a,   43 /* Public */,
      36,    2,  234,    2, 0x2a,   47 /* Public | MethodCloned */,
      36,    1,  239,    2, 0x2a,   50 /* Public | MethodCloned */,
      40,    2,  242,    2, 0x0a,   52 /* Public */,
      41,    2,  247,    2, 0x0a,   55 /* Public */,
      44,    2,  252,    2, 0x0a,   58 /* Public */,
      47,    1,  257,    2, 0x0a,   61 /* Public */,
      48,    3,  260,    2, 0x0a,   63 /* Public */,
      52,    0,  267,    2, 0x0a,   67 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 3, QMetaType::Bool, QMetaType::Bool,    9,   10,   11,   12,
    QMetaType::Void, 0x80000000 | 3,   10,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 15,   10,   16,
    QMetaType::Void, 0x80000000 | 3,   10,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 20,    9,   21,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 20,    9,   21,
    QMetaType::Void, QMetaType::QString,   24,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 8,   27,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, 0x80000000 | 30,   29,    9,   31,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 30,    9,   31,
    QMetaType::Void, QMetaType::Bool, 0x80000000 | 3,   34,    4,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 20,    9,   21,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 37, QMetaType::Bool, QMetaType::Bool,   38,   39,   12,
    QMetaType::Void, 0x80000000 | 37, QMetaType::Bool,   38,   39,
    QMetaType::Void, 0x80000000 | 37,   38,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 20,    9,   21,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 8,   42,   43,
    QMetaType::Void, QMetaType::QColor, QMetaType::QColor,   45,   46,
    QMetaType::Void, 0x80000000 | 3,   10,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 3, 0x80000000 | 50,   49,    4,   51,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlaylistTableView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTableView::staticMetaObject>(),
    qt_meta_stringdata_CLASSPlaylistTableViewENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSPlaylistTableViewENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSPlaylistTableViewENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaylistTableView, std::true_type>,
        // method 'updatePlayingState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        QtPrivate::TypeAndForceComplete<PlayingState, std::false_type>,
        // method 'playMusic'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'encodeFlacFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        // method 'encodeAacFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const EncodingProfile &, std::false_type>,
        // method 'encodeWavFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        // method 'downloadFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        // method 'readReplayGain'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<PlayListEntity> &, std::false_type>,
        // method 'editTags'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<PlayListEntity> &, std::false_type>,
        // method 'updateAlbumCover'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addPlaylistItemFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'extractFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'addToPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::vector<std::string> &, std::false_type>,
        // method 'removePlaylistItems'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const std::vector<std::string> &, std::false_type>,
        // method 'likeSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        // method 'addPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<PlayListEntity> &, std::false_type>,
        // method 'onPlayIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onPlayIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onPlayIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        // method 'onProcessDatabase'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<PlayListEntity> &, std::false_type>,
        // method 'onProcessTrackInfo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        // method 'onThemeColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        QtPrivate::TypeAndForceComplete<QColor, std::false_type>,
        // method 'onReloadEntity'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        // method 'onUpdateReplayGain'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayListEntity &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ReplayGain &, std::false_type>,
        // method 'onRetranslateUi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PlaylistTableView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PlaylistTableView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updatePlayingState((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayingState>>(_a[2]))); break;
        case 1: _t->playMusic((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 2: _t->encodeFlacFile((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1]))); break;
        case 3: _t->encodeAacFile((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<EncodingProfile>>(_a[2]))); break;
        case 4: _t->encodeWavFile((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1]))); break;
        case 5: _t->downloadFile((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1]))); break;
        case 6: _t->readReplayGain((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<PlayListEntity>>>(_a[2]))); break;
        case 7: _t->editTags((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<PlayListEntity>>>(_a[2]))); break;
        case 8: _t->updateAlbumCover((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->addPlaylistItemFinished(); break;
        case 10: _t->extractFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        case 11: _t->addToPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<std::vector<std::string>>>(_a[3]))); break;
        case 12: _t->removePlaylistItems((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<std::vector<std::string>>>(_a[2]))); break;
        case 13: _t->likeSong((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[2]))); break;
        case 14: _t->addPlaylist((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<PlayListEntity>>>(_a[2]))); break;
        case 15: _t->onPlayIndex((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 16: _t->onPlayIndex((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 17: _t->onPlayIndex((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 18: _t->onProcessDatabase((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<PlayListEntity>>>(_a[2]))); break;
        case 19: _t->onProcessTrackInfo((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[2]))); break;
        case 20: _t->onThemeColorChanged((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QColor>>(_a[2]))); break;
        case 21: _t->onReloadEntity((*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[1]))); break;
        case 22: _t->onUpdateReplayGain((*reinterpret_cast< std::add_pointer_t<int32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayListEntity>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ReplayGain>>(_a[3]))); break;
        case 23: _t->onRetranslateUi(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<PlayListEntity> >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<PlayListEntity> >(); break;
            }
            break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<PlayListEntity> >(); break;
            }
            break;
        case 18:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<PlayListEntity> >(); break;
            }
            break;
        case 21:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        case 22:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayListEntity >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PlaylistTableView::*)(const PlayListEntity & , PlayingState );
            if (_t _q_method = &PlaylistTableView::updatePlayingState; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(int32_t , const PlayListEntity & , bool , bool );
            if (_t _q_method = &PlaylistTableView::playMusic; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const PlayListEntity & );
            if (_t _q_method = &PlaylistTableView::encodeFlacFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const PlayListEntity & , const EncodingProfile & );
            if (_t _q_method = &PlaylistTableView::encodeAacFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const PlayListEntity & );
            if (_t _q_method = &PlaylistTableView::encodeWavFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const PlayListEntity & );
            if (_t _q_method = &PlaylistTableView::downloadFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(int32_t , const QList<PlayListEntity> & );
            if (_t _q_method = &PlaylistTableView::readReplayGain; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(int32_t , const QList<PlayListEntity> & );
            if (_t _q_method = &PlaylistTableView::editTags; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const QString & );
            if (_t _q_method = &PlaylistTableView::updateAlbumCover; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)();
            if (_t _q_method = &PlaylistTableView::addPlaylistItemFinished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const QString & , int32_t );
            if (_t _q_method = &PlaylistTableView::extractFile; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const QString & , const QString & , const std::vector<std::string> & );
            if (_t _q_method = &PlaylistTableView::addToPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(const QString & , const std::vector<std::string> & );
            if (_t _q_method = &PlaylistTableView::removePlaylistItems; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(bool , const PlayListEntity & );
            if (_t _q_method = &PlaylistTableView::likeSong; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (PlaylistTableView::*)(int32_t , const QList<PlayListEntity> & );
            if (_t _q_method = &PlaylistTableView::addPlaylist; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
    }
}

const QMetaObject *PlaylistTableView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaylistTableView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSPlaylistTableViewENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QTableView::qt_metacast(_clname);
}

int PlaylistTableView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTableView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    }
    return _id;
}

// SIGNAL 0
void PlaylistTableView::updatePlayingState(const PlayListEntity & _t1, PlayingState _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PlaylistTableView::playMusic(int32_t _t1, const PlayListEntity & _t2, bool _t3, bool _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PlaylistTableView::encodeFlacFile(const PlayListEntity & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PlaylistTableView::encodeAacFile(const PlayListEntity & _t1, const EncodingProfile & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void PlaylistTableView::encodeWavFile(const PlayListEntity & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PlaylistTableView::downloadFile(const PlayListEntity & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void PlaylistTableView::readReplayGain(int32_t _t1, const QList<PlayListEntity> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void PlaylistTableView::editTags(int32_t _t1, const QList<PlayListEntity> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void PlaylistTableView::updateAlbumCover(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void PlaylistTableView::addPlaylistItemFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void PlaylistTableView::extractFile(const QString & _t1, int32_t _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void PlaylistTableView::addToPlaylist(const QString & _t1, const QString & _t2, const std::vector<std::string> & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void PlaylistTableView::removePlaylistItems(const QString & _t1, const std::vector<std::string> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void PlaylistTableView::likeSong(bool _t1, const PlayListEntity & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void PlaylistTableView::addPlaylist(int32_t _t1, const QList<PlayListEntity> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}
QT_WARNING_POP
