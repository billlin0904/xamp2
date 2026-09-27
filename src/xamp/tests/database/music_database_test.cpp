#include <widget/database.h>
#include <widget/dao/musicdao.h>
#include <base/logger.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

using namespace xamp::base;
static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static void execute(QSqlDatabase& db, const QString& sql) {
    QSqlQuery query(db);
    if (!query.exec(sql)) throw std::runtime_error(query.lastError().text().toStdString());
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    XampLoggerFactory.startup();
    const auto original = QDir::currentPath();
    try {
        // Fresh database, only archiveEntryName missing, both missing, and current schema.
        for (int scenario = 0; scenario < 4; ++scenario) {
            QTemporaryDir directory;
            require(directory.isValid() && QDir::setCurrent(directory.path()), "temporary database directory");
            if (scenario) {
                {
                    auto seed = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("seed"));
                    seed.setDatabaseName(directory.filePath(QStringLiteral("xamp.db")));
                    require(seed.open(), "open legacy database");
                    QFile file(QStringLiteral(":/xamp/migrations/2024-01-12-093210_init/up.sql"));
                    require(file.open(QIODevice::ReadOnly), "schema resource");
                    auto schema = QString::fromUtf8(file.readAll()).replace("\r\n", "\n").split(';').first();
                    if (scenario < 3) schema.remove("    archiveEntryName TEXT,\n");
                    if (scenario == 2) schema.remove("    isZipFile integer,\n");
                    execute(seed, schema);
                    execute(seed, QStringLiteral("CREATE TABLE __qt_schema_migrations(version TEXT PRIMARY KEY, run_on TIMESTAMP)"));
                    execute(seed, QStringLiteral("INSERT INTO __qt_schema_migrations(version) VALUES('2026-06-15-120000_playlist_album_states')"));
                    execute(seed, QStringLiteral("INSERT INTO musics(musicId,path,parentPath,title) VALUES(99,'existing.flac','','keep me')"));
                    if (scenario == 3) execute(seed, QStringLiteral("UPDATE musics SET isZipFile=1,archiveEntryName='disc/song.flac' WHERE musicId=99"));
                    seed.close();
                }
                QSqlDatabase::removeDatabase(QStringLiteral("seed"));
            }
            int id = -1;
            for (int reopen = 0; reopen < 2; ++reopen) {
                const auto connection = QStringLiteral("regression_%1_%2").arg(scenario).arg(reopen);
                {
                    Database database(connection);
                    database.open();
                    auto& db = database.database();
                    require(db.record(QStringLiteral("musics")).indexOf(QStringLiteral("archiveEntryName")) >= 0, "archive column migration");
                    require(db.record(QStringLiteral("musics")).indexOf(QStringLiteral("isZipFile")) >= 0, "zip column migration");
                    if (scenario) {
                        QSqlQuery existing(db);
                        require(existing.exec("SELECT title,isZipFile,archiveEntryName FROM musics WHERE musicId=99") && existing.next(), "existing track retained");
                        require(existing.value(0).toString() == "keep me", "existing metadata retained");
                        if (scenario == 3) require(existing.value(1).toInt() == 1 && existing.value(2).toString() == "disc/song.flac", "archive metadata retained");
                    }
                    TrackInfo track;
                    track.file_path = Path(L"G:\\音樂\\Final Fantasy Music Collection (FLAC)\\2-01 植松伸夫 - 天来～Divinity I～.flac");
                    track.title = L"天来～Divinity I～";
                    track.duration = 174; track.track = 1; track.sample_rate = 44100;
                    track.bit_rate = 909; track.file_size = 19943580;
                    dao::MusicDao music(db);
                    if (!reopen) {
                        id = music.addOrUpdateMusic(track);
                        music.updateMusicReplayGain(id, -3.5, 0.9, -2.5, 0.8);
                    }
                    track.title = L"updated 天来";
                    require(music.addOrUpdateMusic(track) == id, "rescan retains music id");
                    QSqlQuery check(db);
                    require(check.exec(QStringLiteral("SELECT title,isZipFile,archiveEntryName,albumReplayGain,trackReplayGain FROM musics WHERE musicId=%1").arg(id)) && check.next(), "inserted music");
                    require(check.value(0).toString() == QString::fromStdWString(track.title), "Unicode title");
                    require(check.value(1).toInt() == 0 && check.value(2).isNull(), "normal file archive fields");
                    require(check.value(3).toDouble() == -3.5 && check.value(4).toDouble() == -2.5, "replaygain retained");
                }
                QSqlDatabase::removeDatabase(connection);
            }
            QDir::setCurrent(original);
        }
        std::cout << "PASS: fresh/legacy/current schema, repeated opens, Unicode FLAC insert/rescan, existing tracks and archive metadata retained\n";
    } catch (const std::exception& e) {
        QDir::setCurrent(original);
        std::cerr << e.what() << '\n'; return 1;
    }
    return 0;
}
