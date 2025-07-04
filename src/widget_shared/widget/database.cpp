﻿#include <widget/database.h>

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QDir>
#include <QRegularExpressionMatchIterator>
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
        QSqlQuery query("create table if not exists " SCHAMA_MIGRATIONS_TABLE " ("
            "version Text primary key not null, "
            "run_on timestamp not null default current_timestamp)"_str, database);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    void markMigrationRun(QSqlDatabase& database, const QString& name) {
        XAMP_LOG_DEBUG("Marking migration {} as done.", name.toStdString());

        QSqlQuery query(database);
        if (!query.prepare("insert into " SCHAMA_MIGRATIONS_TABLE " (version) values (:name)"_str)) {
            printSqlError(query);
        }
        query.bindValue(":name"_str, name);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    QString currentDatabaseVersion(QSqlDatabase& database) {
        QSqlQuery query(database);
        query.prepare("select version from " SCHAMA_MIGRATIONS_TABLE " order by version desc limit 1"_str);
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
                QFile file(migration_directory + QDir::separator() + entry + QDir::separator() + "up.sql"_str);
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


QString lastExecutedQuery(const QSqlQuery& query) {
    QString sql = query.executedQuery();
    const auto boundValues = query.boundValues();

    static const QRegularExpression placeholderPattern{ "(:\\w+)"_str };
    QRegularExpressionMatchIterator it = placeholderPattern.globalMatch(sql);

    int valueIndex{ 0 };
    while (it.hasNext() && valueIndex < boundValues.size()) {
        const QRegularExpressionMatch match = it.next();
        const QString placeholder = match.captured(1);
        const QString value = boundValues.at(valueIndex).toString();

        if (!value.isEmpty()) {
            const QRegularExpression exactPlaceholderPattern{ QRegularExpression::escape(placeholder) + "\\b"_str };
            sql.replace(exactPlaceholderPattern, value);
        }

        ++valueIndex;
    }

    return sql;
}

QString SqlQuery::lastQuery() const {
    return lastExecutedQuery(*this);
}

QString DatabaseFactory::getDatabaseId() {
    return "xamp_db_"_str + QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16);
}

PooledDatabasePtr getPooledDatabase(int32_t pool_size) {
    return std::make_shared<ObjectPool<Database, DatabaseFactory>>(pool_size);
}

QScopedPointer<Database> makeDatabaseConnection() {
    DatabaseFactory factory;
    return QScopedPointer<Database>(factory.Create());
}

SqlException::SqlException(const SqlQuery& query) 
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, query.lastQuery().toStdString()) {
    XAMP_LOG_DEBUG("SqlException: {}\r\n{}\r\n{}",
        query.lastError().text().toStdString(),
        query.lastQuery().toStdString(),
        GetStackTrace());
}

SqlException::SqlException(QSqlError error)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
        error.text().toStdString()) {
    XAMP_LOG_DEBUG("SqlException: {}{}", error.text().toStdString(), GetStackTrace());
}

const char* SqlException::what() const noexcept {
    return message_.c_str();
}

constexpr auto kGuiDatabaseName = "UI"_str;

XAMP_DECLARE_LOG_NAME(Database);

Database::Database(const QString& name) {
    logger_ = XampLoggerFactory.GetLogger(kDatabaseLoggerName);
    if (QSqlDatabase::contains(name)) {
        db_ = QSqlDatabase::database(name);
    }
    else {
        db_ = QSqlDatabase::addDatabase("QSQLITE"_str, name);
    }
    connection_name_ = name;
}

Database::Database()
	: Database(kGuiDatabaseName) {
}

QSqlDatabase& Database::database() {
    return db_;
}

Database::~Database() {
    Close();
}

void Database::Close() {
    if (db_.isOpen()) {
    	db_.close();
        XAMP_LOG_I(logger_, "Database {} closed.", connection_name_.toStdString());
        logger_.reset();
    }
}

void Database::open() {
    db_.setDatabaseName("xamp.db"_str);

    if (!db_.open()) {
        throw SqlException(db_.lastError());
    }

    XAMP_LOG_I(logger_, "Database {} opened, SQlite version: {}.",
        connection_name_.toStdString(), getVersion().toStdString());

    //if (connection_name_ != kGuiDatabaseName) {
    //    return;
    //}


#define CHECK_EXE(expr) \
	if (!(expr)) {\
		XAMP_LOG_ERROR("Failed to execute database command: {}", #expr);\
    }

    SqlQuery query(db_);
    CHECK_EXE(query.exec("PRAGMA auto_vacuum = FULL"_str))
    CHECK_EXE(query.exec("PRAGMA page_size = 40960"_str))
    CHECK_EXE(query.exec("PRAGMA foreign_keys = ON"_str))
    CHECK_EXE(query.exec("PRAGMA cache_size = 40960"_str))
    CHECK_EXE(query.exec("PRAGMA temp_store = MEMORY"_str))
    CHECK_EXE(query.exec("PRAGMA mmap_size = 40960"_str))    

    //CHECK_EXE(query.exec("PRAGMA journal_mode = WAL"_str))
    //CHECK_EXE(query.exec("PRAGMA busy_timeout = 5000"_str))

    createTableIfNotExist();
}

bool Database::transaction() {
    return db_.transaction();
}

bool Database::commit() {
    return db_.commit();
}

bool Database::rollback() {
    return db_.rollback();
}

QString Database::getVersion() const {
    SqlQuery query(db_);
    query.exec("SELECT sqlite_version() AS version;"_str);
    if (query.next()) {
        return query.value("version"_str).toString();
    }
    throw SqlException(query.lastError());
}

void Database::createTableIfNotExist() {
    runDatabaseMigrations(db_, ":/xamp/migrations/"_str);
}

bool Database::dropAllTable() {
    SqlQuery drop_query(db_);

    const QString drop_query_string = "DROP TABLE IF EXISTS %1"_str;
    auto tableNames = db_.tables();

    tableNames.removeAll("sqlite_sequence"_str);
    for (const auto& table_name : qAsConst(tableNames)) {
        DbIfFailedThrow(drop_query, drop_query_string.arg(table_name));
    }
    return true;
}
