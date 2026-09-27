#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <widget/dao/playlistdao.h>
#include <stdexcept>
#include <iostream>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(QStringLiteral(":memory:"));
    if (!db.open()) return 1;
    QSqlQuery q(db);
    auto exec = [&](const char* sql) { if (!q.exec(QString::fromUtf8(sql))) throw std::runtime_error(sql); };
    auto count = [&](const char* table) { exec((std::string("SELECT COUNT(*) FROM ")+table).c_str()); q.next(); return q.value(0).toInt(); };
    try {
        exec("CREATE TABLE playlist(playlistId INTEGER PRIMARY KEY)");
        exec("CREATE TABLE playlistMusics(playlistId INTEGER, musicId INTEGER)");
        exec("CREATE TABLE playlistAlbumStates(playlistId INTEGER, albumId INTEGER)");
        exec("CREATE TABLE musics(musicId INTEGER PRIMARY KEY)");
        exec("INSERT INTO playlist VALUES(10),(11)");
        exec("INSERT INTO playlistMusics VALUES(10,1),(11,1)");
        exec("INSERT INTO playlistAlbumStates VALUES(10,1),(11,1)");
        exec("INSERT INTO musics VALUES(1)");
        dao::PlaylistDao dao(db);
        dao.removePlaylist(10);
        if (count("playlist") != 1 || count("playlistMusics") != 1 || count("playlistAlbumStates") != 1 || count("musics") != 1)
            throw std::runtime_error("Deletion affected unrelated data");
        exec("CREATE TRIGGER reject_delete BEFORE DELETE ON playlist BEGIN SELECT RAISE(ABORT, 'test rollback'); END");
        bool failed = false;
        try { dao.removePlaylist(11); } catch (...) { failed = true; }
        if (!failed || count("playlist") != 1 || count("playlistMusics") != 1 || count("playlistAlbumStates") != 1)
            throw std::runtime_error("Deletion did not roll back");
        std::cout << "Playlist deletion and rollback passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what(); return 1; }
}
