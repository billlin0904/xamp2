#include <base/platfrom_handle.h>
#include <base/dll.h>
#include <base/exception.h>
#include <base/logger.h>

#ifdef XAMP_OS_WIN

#include <discid/discid.h>
#include <stream/discIdlib.h>
#include <stream/mbdiscid.h>

XAMP_STREAM_NAMESPACE_BEGIN

struct DiscIdDeleter final {
	static DiscId* invalid() {
		return nullptr;
	}

	static void close(DiscId* value) {
		DiscIdDLL.discid_free(value);
	}
};

using DiscIdHandle = UniqueHandle<DiscId*, DiscIdDeleter>;

class DiscIdException : public Exception {
public:
	explicit DiscIdException(const DiscIdHandle &disc) {
		message_ = DiscIdDLL.discid_get_error_msg(disc.get());
		what_ = message_;
	}
};

#define DiscIdIfFailedThrow(result) \
	do {\
        if (!(result)) {\
            throw DiscIdException(handle_);\
        }\
    } while (false)

class MBDiscId::MBDiscIdImpl {
public:
	MBDiscIdImpl() {
		handle_.reset(DiscIdDLL.discid_new());
	}

	std::string getDiscId(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_id(handle_.get());
	}

	std::string getFreeDBId(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_freedb_id(handle_.get());
	}

	std::string getSubmissionUrl(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_submission_url(handle_.get());
	}

	std::string getDiscIdLookupUrl(const std::string& drive) const {
		auto disc_id = getDiscId(drive);
		auto toc = getSubmissionUrl(drive);
		if (!disc_id.empty() && !toc.empty()) {
			toc = toc.substr(toc.find_first_of('&'));
			return "http://www.musicbrainz.org/ws/2/discid/" + disc_id + "?" + "&inc=recordings%20artists" + toc;
		}
		return "";
	}
private:
	DiscIdHandle handle_;
};

MBDiscId::MBDiscId()
	: impl_(makeAlign<MBDiscIdImpl>()) {
}

XAMP_PIMPL_IMPL(MBDiscId)

std::string MBDiscId::getDiscId(const std::string& drive) const {
	return impl_->getDiscId(drive);
}

std::string MBDiscId::getFreeDBId(const std::string& drive) const {
	return impl_->getFreeDBId(drive);
}

std::string MBDiscId::getSubmissionUrl(const std::string& drive) const {
	return impl_->getSubmissionUrl(drive);
}

std::string MBDiscId::getDiscIdLookupUrl(const std::string& drive) const {
	return impl_->getDiscIdLookupUrl(drive);
}

XAMP_STREAM_NAMESPACE_END

#endif
