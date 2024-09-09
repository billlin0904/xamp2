#include <base/platfrom_handle.h>
#include <base/dll.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/singleton.h>

#ifdef XAMP_OS_WIN

#include <discid/discid.h>
#include <stream/discIdlib.h>
#include <stream/mbdiscid.h>

XAMP_STREAM_NAMESPACE_BEGIN

#define DiscIdDLL SharedSingleton<DiscIdLib>::GetInstance()

struct DiscIdDeleter final {
	static DiscId* invalid() noexcept {
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

	std::string GetDiscId(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_id(handle_.get());
	}

	std::string GetFreeDBId(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_freedb_id(handle_.get());
	}

	std::string GetSubmissionUrl(const std::string& drive) const {
		DiscIdIfFailedThrow(DiscIdDLL.discid_read(handle_.get(), drive.c_str()));
		return DiscIdDLL.discid_get_submission_url(handle_.get());
	}

	std::string GetDiscIdLookupUrl(const std::string& drive) const {
		auto disc_id = GetDiscId(drive);
		auto toc = GetSubmissionUrl(drive);
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
	: impl_(MakeAlign<MBDiscIdImpl>()) {
}

XAMP_PIMPL_IMPL(MBDiscId)

std::string MBDiscId::GetDiscId(const std::string& drive) const {
	return impl_->GetDiscId(drive);
}

std::string MBDiscId::GetFreeDBId(const std::string& drive) const {
	return impl_->GetFreeDBId(drive);
}

std::string MBDiscId::GetSubmissionUrl(const std::string& drive) const {
	return impl_->GetSubmissionUrl(drive);
}

std::string MBDiscId::GetDiscIdLookupUrl(const std::string& drive) const {
	return impl_->GetDiscIdLookupUrl(drive);
}

XAMP_STREAM_NAMESPACE_END

#endif
