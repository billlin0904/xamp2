#include <widget/database.h>

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QDir>
#include <QThread>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/assert.h>
#include <widget/util/str_util.h>

namespace {
#define SCHAMA_MIGRATIONS_TABLE "__qt_schema_migrations"

    void printSqlError(const QSqlQuery& query) {
        XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
    }

    void createInternalTable(QSqlDatabase& database) {
        QSqlQuery query(qTEXT("create table if not exists " SCHAMA_MIGRATIONS_TABLE " ("
            "version Text primary key not null, "
            "run_on timestamp not null default current_timestamp)"), database);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    void markMigrationRun(QSqlDatabase& database, const QString& name) {
        XAMP_LOG_DEBUG("Marking migration {} as done.", name.toStdString());

        QSqlQuery query(database);
        if (!query.prepare(qTEXT("insert into " SCHAMA_MIGRATIONS_TABLE " (version) values (:name)"))) {
            printSqlError(query);
        }
        query.bindValue(qTEXT(":name"), name);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    QString currentDatabaseVersion(QSqlDatabase& database) {
        QSqlQuery query(database);
        query.prepare(qTEXT("select version from " SCHAMA_MIGRATIONS_TABLE " order by version desc limit 1"));
        query.exec();

        if (query.next()) {
            return query.value(0).toString();
        }
        else {
            return {};
        }
    }

    void runDatabaseMigrations(QSqlDatabase& database, const QString& migration_directory) {
        createInternalTable(database);

        QDir dir(migration_directory);
        const auto entries = dir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot, QDir::SortFlag::Name);

        const QString currentVersion = currentDatabaseVersion(database);
        for (const auto& entry : entries) {
            QDir subdir(entry);
            if (subdir.dirName() > currentVersion) {
                QFile file(migration_directory + QDir::separator() + entry + QDir::separator() + qTEXT("up.sql"));
                if (!file.open(QFile::ReadOnly)) {
                    XAMP_LOG_DEBUG("Failed to open migration file {}", file.fileName().toStdString());
                }
                XAMP_LOG_DEBUG("Running migration {}", subdir.dirName().toStdString());

                database.transaction();

                // Hackish
                const auto statements = file.readAll().split(';');

                bool migration_successful = true;
                for (const QByteArray& statement : statements) {
                    const auto trimmed_statement = QString::fromUtf8(statement.trimmed());
                    QSqlQuery query(database);

                    if (!trimmed_statement.isEmpty()) {
                        XAMP_LOG_DEBUG("Running {}", trimmed_statement.toStdString());
                        if (!query.prepare(trimmed_statement)) {
                            printSqlError(query);
                            migration_successful = false;
                        }
                        else {
                            const bool success = query.exec();
                            migration_successful &= success;
                            if (!success) {
                                printSqlError(query);
                            }
                        }
                    }
                }

                if (migration_successful) {
                    database.commit();
                    markMigrationRun(database, subdir.dirName());
                }
                else {
                    XAMP_LOG_DEBUG("Migration {} failed, retrying next time.", subdir.dirName().toStdString());
                    XAMP_LOG_DEBUG("Stopping migrations here, as the next migration may depens on this one.");
                    database.rollback();
                    return;
                }
            }
        }
        XAMP_LOG_DEBUG("Migrations finished");
    }
}

QString DatabaseFactory::getDatabaseId() {
    return qTEXT("xamp_db_") + QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16);
}

PooledDatabasePtr getPooledDatabase(int32_t pool_size) {
    return std::make_shared<ObjectPool<Database, DatabaseFactory>>(pool_size);
}

SqlException::SqlException(QSqlError error)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
        error.text().toStdString()) {
    XAMP_LOG_DEBUG("SqlException: {}{}", error.text().toStdString(), GetStackTrace());
}

const char* SqlException::what() const noexcept {
    return message_.c_str();
}

XAMP_DECLARE_LOG_NAME(Database);

Database::Database(const QString& name) {
    logger_ = XampLoggerFactory.GetLogger(kDatabaseLoggerName);
    if (QSqlDatabase::contains(name)) {
        db_ = QSqlDatabase::database(name);
    }
    else {
        db_ = QSqlDatabase::addDatabase(qTEXT("QSQLITE"), name);
    }
    connection_name_ = name;
}

Database::Database()
	: Database(qTEXT("UI")) {
}

QSqlDatabase& Database::database() {
    return db_;
}

Database::~Database() {
    close();
}

void Database::close() {
    if (db_.isOpen()) {
        XAMP_LOG_I(logger_, "Database {} closed.", connection_name_.toStdString());
        db_.close();
    }
}

void Database::open() {    
    db_.setDatabaseName(qTEXT("xamp.db"));

    if (!db_.open()) {
        throw SqlException(db_.lastError());
    }

    XAMP_LOG_I(logger_, "Database {} opened, SQlite version: {}.",
        connection_name_.toStdString(), getVersion().toStdString());

    if (connection_name_ != qTEXT("UI")) {
        return;
    }

    (void)db_.exec(qTEXT("PRAGMA auto_vacuum = FULL"));
    (void)db_.exec(qTEXT("PRAGMA foreign_keys = ON"));
    (void)db_.exec(qTEXT("PRAGMA journal_mode = DELETE"));

    /*(void)db_.exec(qTEXT("PRAGMA synchronous = OFF"));    
    (void)db_.exec(qTEXT("PRAGMA auto_vacuum = FULL"));
    (void)db_.exec(qTEXT("PRAGMA foreign_keys = ON"));
    (void)db_.exec(qTEXT("PRAGMA journal_mode = DELETE"));
    (void)db_.exec(qTEXT("PRAGMA cache_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA temp_store = MEMORY"));
    (void)db_.exec(qTEXT("PRAGMA mmap_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA busy_timeout = 1000"));*/    

    createTableIfNotExist();
}

bool Database::transaction() {
    return db_.transaction();
}

bool Database::commit() {
    return db_.commit();
}

void Database::rollback() {
    db_.rollback();
}

QString Database::getVersion() const {
    SqlQuery query(db_);
    query.exec(qTEXT("SELECT sqlite_version() AS version;"));
    if (query.next()) {
        return query.value(qTEXT("version")).toString();
    }
    throw SqlException(query.lastError());
}

void Database::createTableIfNotExist() {
    runDatabaseMigrations(db_, qTEXT(":/xamp/migrations/"));
}

bool Database::dropAllTable() {
    SqlQuery drop_query(db_);

    const QString drop_query_string = qTEXT("DROP TABLE IF EXISTS %1");
    auto tableNames = db_.tables();

    tableNames.removeAll(qTEXT("sqlite_sequence"));
    for (const auto& table_name : qAsConst(tableNames)) {
        DbIfFailedThrow(drop_query, drop_query_string.arg(table_name));
    }
    return true;
}
