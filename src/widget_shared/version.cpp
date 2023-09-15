#include <version.h>

const Version kApplicationVersionValue{ 0, 0, 1 };
const ConstLatin1String kApplicationName{ "XAMP2" };
const ConstLatin1String kApplicationTitle{ "XAMP" };
const ConstLatin1String kApplicationVersion{ "0.0.1" };
const ConstLatin1String kDefaultCharset{ "UTF-8" };
const ConstLatin1String kDefaultUserAgent{ "xamp/0.0.1" };

bool ParseVersion(const QString& s, Version& version) {
	const auto ver = s.split(qTEXT("."));
	if (ver.length() != 3) {
		return false;
	}

	for (auto i = 0; i < ver.length(); ++i) {
		bool ok = false;
		switch (i) {
		case 0:
			version.major_part = ver[i].toInt(&ok);
			break;
		case 1:
			version.minor_part = ver[i].toInt(&ok);
			break;
		case 2:
			version.revision_part = ver[i].toInt(&ok);
			break;
		default: ;
		}
		if (!ok) {
			return false;
		}
	}
	return true;
}