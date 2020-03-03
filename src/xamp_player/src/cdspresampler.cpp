#include <base/dll.h>
#include <base/logger.h>
#include <player/cdspresampler.h>

namespace xamp::player {

class CdspLib final {
public:
	CdspLib() try
#ifdef _WIN32
		: module_(LoadDll("libsoxr.dll"))
#else
		: module_(LoadDll("libsoxr.dylib"))
#endif
		, soxr_quality_spec(module_, "soxr_quality_spec")
		, soxr_create(module_, "soxr_create")
		, soxr_process(module_, "soxr_process")
		, soxr_delete(module_, "soxr_delete")
		, soxr_io_spec(module_, "soxr_io_spec")
		, soxr_runtime_spec(module_, "soxr_runtime_spec") {
	}
	catch (const Exception & e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	static XAMP_ALWAYS_INLINE CdspLib& Instance() {
		static CdspLib lib;
		return lib;
	}

	XAMP_DISABLE_COPY(CdspLib)

private:
	ModuleHandle module_;

public:	
	XAMP_DEFINE_DLL_API(soxr_quality_spec) soxr_quality_spec;
	XAMP_DEFINE_DLL_API(soxr_create) soxr_create;
	XAMP_DEFINE_DLL_API(soxr_process) soxr_process;
	XAMP_DEFINE_DLL_API(soxr_delete) soxr_delete;
	XAMP_DEFINE_DLL_API(soxr_io_spec) soxr_io_spec;
	XAMP_DEFINE_DLL_API(soxr_runtime_spec) soxr_runtime_spec;
};

class CdspResampler::CdspResamplerImpl {
public:
	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
		return false;
	}
};

CdspResampler::CdspResampler()
	: impl_(MakeAlign<CdspResamplerImpl>()) {
}

XAMP_PIMPL_IMPL(CdspResampler)

}
