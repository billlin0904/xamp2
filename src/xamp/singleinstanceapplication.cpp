#include <QCryptographicHash>
#include <QSharedMemory>
#include <QDir>

#include <base/logger_impl.h>
#include <base/siphash.h>
#include <base/rng.h>
#include <base/uuid.h>
#include <widget/str_utilts.h>
#include <singleinstanceapplication.h>

static QString getHashAppName() {
    //TODO: use applicationFilePath?
    //QString serverNameSource = QCoreApplication::applicationFilePath();
    QString server_name_source;
    server_name_source += QDir::home().dirName();
    server_name_source += QSysInfo::machineHostName();
    return QString(Q_TEXT(QCryptographicHash::hash(server_name_source.toUtf8(), QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
}

static std::string getRandomMutexName(const std::string &src_name) {
	union w128_t {
        uint8_t  b[16];
        uint32_t w[4];
        uint64_t q[2];
        double   d[2];
    } s{};

    PRNG prng;
    s.w[0] = prng.NextInt32() * 0x9e3779b9;
    s.w[1] = prng.NextInt32() * 0x9e3779b9;

    SipHash siphash(s.w[0], s.w[1]);
    siphash.Update(src_name);
    s.q[1] = siphash.GetHash();

    Uuid uuid(s.b, s.b + 16);
    std::string str = uuid;

    auto parsed = Uuid::FromString(str);
    const auto *p = reinterpret_cast<const w128_t*>(parsed.GetBytes().data());
    SipHash test_hash(p->w[0], p->w[1]);
    test_hash.Update(src_name);
    auto hash = test_hash.GetHash();
    return str;
}

// https://github.com/odzhan/polymutex

SingleInstanceApplication::SingleInstanceApplication()
    : singular_(new QSharedMemory()) {
    getRandomMutexName("Xamp2");
    singular_->setKey(getHashAppName());
}

SingleInstanceApplication::~SingleInstanceApplication() {
    delete singular_;
}

bool SingleInstanceApplication::attach() const {
    if (singular_->attach(QSharedMemory::ReadOnly)) {
        singular_->detach();
        return false;
    }

    if (singular_->create(1))
        return true;

    return false;
}

