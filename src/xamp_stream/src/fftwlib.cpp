#include <stream/fftwlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

#if (USE_INTEL_MKL_LIB)
#ifndef MKL_FFTW_MALLOC_ALIGNMENT
#define MKL_FFTW_MALLOC_ALIGNMENT (64)
#endif

fftw3_mkl_s fftw3_mkl = {
	0,  /* verbose */
	1,  /* nthreads */
	0.0,        /* timelimit */
	1,  /* Number of user threads variable. Will be depricated in nearest future */
	xamp::stream::FFTWLib::new_plan,
	MKL_FFTW_MALLOC_ALIGNMENT /* default_alignment */
};
#endif

XAMP_STREAM_NAMESPACE_BEGIN

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

#if (USE_INTEL_MKL_LIB)

static void ExecuteIForward(fftw_mkl_plan p) {
	FFTW_LIB.DftiComputeForward(p->desc, p->io[0]);
}

static void ExecuteForward(fftw_mkl_plan p) {
	FFTW_LIB.DftiComputeForward(p->desc, p->io[0], p->io[1]);
}

static void ExecuteIBackward(fftw_mkl_plan p) {
	FFTW_LIB.DftiComputeBackward(p->desc, p->io[0]);
}

static void ExecuteBackward(fftw_mkl_plan p) {
	FFTW_LIB.DftiComputeBackward(p->desc, p->io[0], p->io[1]);
}

MKLLib::MKLLib() try
    : module_(OpenSharedLibrary("mkl_rt.2"))
	, XAMP_LOAD_DLL_API(MKL_malloc)
	, XAMP_LOAD_DLL_API(MKL_free)
	, XAMP_LOAD_DLL_API(DftiErrorClass)
	, XAMP_LOAD_DLL_API(DftiFreeDescriptor)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_md)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_md)
	, XAMP_LOAD_DLL_API(DftiComputeForward)
	, DftiCreateDescriptor_(module_, "DftiCreateDescriptor")
	, XAMP_LOAD_DLL_API(DftiSetValue)
	, XAMP_LOAD_DLL_API(DftiCommitDescriptor)
	, XAMP_LOAD_DLL_API(DftiComputeBackward){
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

FFTWLib::FFTWLib() try
    : module_(OpenSharedLibrary("mkl_rt.2"))
	, XAMP_LOAD_DLL_API(DftiErrorClass)
	, XAMP_LOAD_DLL_API(DftiFreeDescriptor)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_md)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_md)
	, XAMP_LOAD_DLL_API(DftiComputeForward)
	, DftiCreateDescriptor_(module_, "DftiCreateDescriptor")
	, XAMP_LOAD_DLL_API(DftiSetValue)
	, XAMP_LOAD_DLL_API(DftiCommitDescriptor)
	, XAMP_LOAD_DLL_API(DftiComputeBackward) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

void FFTWLib::delete_plan(fftw_mkl_plan p) {
	if (p) {
		if (p->spar)
			FFTW_LIB.fftw_free(p->spar);
		if (p->dpar)
			FFTW_LIB.fftw_free(p->dpar);
		if (p->ipar)
			FFTW_LIB.fftw_free(p->ipar);
		if (p->desc)
			FFTW_LIB.DftiFreeDescriptor(&p->desc);
		FFTW_LIB.fftw_free(p);
	}
}

fftw_mkl_plan FFTWLib::new_plan() {
	fftw_mkl_plan p = static_cast<fftw_mkl_plan>(FFTW_LIB.fftw_malloc(sizeof(*p)));

	if (p) {
		p->desc = nullptr;
		p->io[0] = p->io[1] = p->io[2] = p->io[3] = nullptr;
		p->ipar = nullptr;
		p->dpar = nullptr;
		p->spar = nullptr;
		p->execute = nullptr;
		p->destroy = delete_plan;
		p->mpi_plan = nullptr;
	}
	return p;
}

fftw_plan FFTWLib::fftw_plan_guru64_dft_c2r(int rank,
	const fftw_iodim64* dims,
	int howmany_rank,
	const fftw_iodim64* howmany_dims,
	fftw_complex* in,
	double* out,
	unsigned flags) {
	fftw_mkl_plan mkl_plan;
	MKL_LONG periods[MKL_MAXRANK];
	MKL_LONG istrides[MKL_MAXRANK + 1];
	MKL_LONG ostrides[MKL_MAXRANK + 1];
	MKL_LONG s = 0;                    /* status */
	int i;

	UNUSED(flags);

	if (rank > MKL_MAXRANK || howmany_rank > MKL_ONE)
		return nullptr;

	if (dims == nullptr || (howmany_rank > 0 && howmany_dims == nullptr))
		return nullptr;

	mkl_plan = fftw3_mkl.new_plan();
	if (!mkl_plan)
		return nullptr;

	istrides[0] = 0;
	ostrides[0] = 0;

	for (i = 0; i < rank; ++i)
	{
		periods[i] = static_cast<long>(dims[i].n);
		istrides[i + 1] = static_cast<long>(dims[i].is);
		ostrides[i + 1] = static_cast<long>(dims[i].os);

		/* check if MKL_LONG is sufficient to hold dims */
		if (periods[i] != dims[i].n)
			goto broken;
		if (istrides[i + 1] != dims[i].is)
			goto broken;
		if (ostrides[i + 1] != dims[i].os)
			goto broken;
	}
	if (rank == 1)
		s = DftiCreateDescriptor_(&mkl_plan->desc, DFTI_DOUBLE, DFTI_REAL,
			static_cast<long>(rank), periods[0]);
	else
		s = DftiCreateDescriptor_(&mkl_plan->desc, DFTI_DOUBLE, DFTI_REAL,
			static_cast<long>(rank), periods);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_CONJUGATE_EVEN_STORAGE,
		DFTI_COMPLEX_COMPLEX);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_STRIDES, istrides);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_STRIDES, ostrides);
	if (BAD(s))
		goto broken;

	if (static_cast<void*>(in) == static_cast<void*>(out))
	{
		mkl_plan->io[0] = in;
		mkl_plan->execute = ExecuteIBackward;
	}
	else
	{
		mkl_plan->io[0] = in;
		mkl_plan->io[1] = out;
		mkl_plan->execute = ExecuteBackward;
		s = DftiSetValue(mkl_plan->desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
		if (BAD(s))
			goto broken;
	}

	if (howmany_rank == 1)
	{
		MKL_LONG howmany = static_cast<long>(howmany_dims[0].n);
		MKL_LONG idistance = static_cast<long>(howmany_dims[0].is);
		MKL_LONG odistance = static_cast<long>(howmany_dims[0].os);

		/* check if MKL_LONG is sufficient to hold dims */
		if (howmany != howmany_dims[0].n)
			goto broken;
		if (idistance != howmany_dims[0].is)
			goto broken;
		if (odistance != howmany_dims[0].os)
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_NUMBER_OF_TRANSFORMS, howmany);
		if (BAD(s))
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_DISTANCE, idistance);
		if (BAD(s))
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_DISTANCE, odistance);
		if (BAD(s))
			goto broken;
	}

	if (fftw3_mkl.nthreads >= 0)
	{
		s = DftiSetValue(mkl_plan->desc, DFTI_THREAD_LIMIT,
			static_cast<long>(fftw3_mkl.nthreads));
		if (BAD(s))
			goto broken;
	}

	s = DftiCommitDescriptor(mkl_plan->desc);
	if (BAD(s))
		goto broken;

	return reinterpret_cast<fftw_plan>(mkl_plan);

broken:
	/* possibly report the reason before returning NULL */
	mkl_plan->destroy(mkl_plan);
	return nullptr;
}

fftw_plan FFTWLib::fftw_plan_guru64_dft_r2c(int rank,
	const fftw_iodim64* dims,
	int howmany_rank,
	const fftw_iodim64* howmany_dims,
	double* in,
	fftw_complex* out,
	unsigned flags) {
	fftw_mkl_plan mkl_plan;
	MKL_LONG periods[MKL_MAXRANK];
	MKL_LONG istrides[MKL_MAXRANK + 1];
	MKL_LONG ostrides[MKL_MAXRANK + 1];
	MKL_LONG s = 0;                    /* status */
	int i;

	UNUSED(flags);

	if (rank > MKL_MAXRANK || howmany_rank > MKL_ONE)
		return nullptr;

	if (dims == nullptr || (howmany_rank > 0 && howmany_dims == nullptr))
		return nullptr;

	mkl_plan = fftw3_mkl.new_plan();
	if (!mkl_plan)
		return nullptr;

	istrides[0] = 0;
	ostrides[0] = 0;

	for (i = 0; i < rank; ++i)
	{
		periods[i] = static_cast<long>(dims[i].n);
		istrides[i + 1] = static_cast<long>(dims[i].is);
		ostrides[i + 1] = static_cast<long>(dims[i].os);

		/* check if MKL_LONG is sufficient to hold dims */
		if (periods[i] != dims[i].n)
			goto broken;
		if (istrides[i + 1] != dims[i].is)
			goto broken;
		if (ostrides[i + 1] != dims[i].os)
			goto broken;
	}
	if (rank == 1)
		s = DftiCreateDescriptor_(&mkl_plan->desc, DFTI_DOUBLE, DFTI_REAL,
			static_cast<long>(rank), periods[0]);
	else
		s = DftiCreateDescriptor_(&mkl_plan->desc, DFTI_DOUBLE, DFTI_REAL,
			static_cast<long>(rank), periods);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_CONJUGATE_EVEN_STORAGE,
		DFTI_COMPLEX_COMPLEX);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_STRIDES, istrides);
	if (BAD(s))
		goto broken;

	s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_STRIDES, ostrides);
	if (BAD(s))
		goto broken;

	if (static_cast<void*>(in) == static_cast<void*>(out))
	{
		mkl_plan->io[0] = in;
		mkl_plan->execute = ExecuteIForward;
	}
	else
	{
		mkl_plan->io[0] = in;
		mkl_plan->io[1] = out;
		mkl_plan->execute = ExecuteForward;
		s = DftiSetValue(mkl_plan->desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
		if (BAD(s))
			goto broken;
	}

	if (howmany_rank == 1)
	{
		MKL_LONG howmany = static_cast<long>(howmany_dims[0].n);
		MKL_LONG idistance = static_cast<long>(howmany_dims[0].is);
		MKL_LONG odistance = static_cast<long>(howmany_dims[0].os);

		/* check if MKL_LONG is sufficient to hold dims */
		if (howmany != howmany_dims[0].n)
			goto broken;
		if (idistance != howmany_dims[0].is)
			goto broken;
		if (odistance != howmany_dims[0].os)
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_NUMBER_OF_TRANSFORMS, howmany);
		if (BAD(s))
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_DISTANCE, idistance);
		if (BAD(s))
			goto broken;

		s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_DISTANCE, odistance);
		if (BAD(s))
			goto broken;
	}

	if (fftw3_mkl.nthreads >= 0)
	{
		s = DftiSetValue(mkl_plan->desc, DFTI_THREAD_LIMIT,
			static_cast<long>(fftw3_mkl.nthreads));
		if (BAD(s))
			goto broken;
	}

	s = DftiCommitDescriptor(mkl_plan->desc);
	if (BAD(s))
		goto broken;

	return reinterpret_cast<fftw_plan>(mkl_plan);

broken:
	/* possibly report the reason before returning NULL */
	mkl_plan->destroy(mkl_plan);
	return nullptr;
}

void FFTWLib::fftw_destroy_plan(fftw_plan plan) {
	if (plan && reinterpret_cast<fftw_mkl_plan>(plan)->destroy) {
		reinterpret_cast<fftw_mkl_plan>(plan)->destroy(reinterpret_cast<fftw_mkl_plan>(plan));
	}
}

void* FFTWLib::fftw_malloc(size_t n) {
	return MKL_LIB.MKL_malloc(n, fftw3_mkl.default_alignment);
}

void FFTWLib::fftw_free(void* p) {
	MKL_LIB.MKL_free(p);
}

void FFTWLib::fftw_execute(const fftw_plan plan) {
	auto mkl_plan = reinterpret_cast<fftw_mkl_plan>(plan);

	if (!(mkl_plan && mkl_plan->execute)) return;

	mkl_plan->execute(mkl_plan);
}

fftw_plan FFTWLib::fftw_plan_dft_r2c_1d(int n, double* in, fftw_complex* out, unsigned flags) {
	return fftw_plan_dft_r2c(1, &n, in, out, flags);
}

fftw_plan FFTWLib::fftw_plan_dft_c2r_1d(int n, fftw_complex* in, double* out, unsigned flags) {
	return fftw_plan_dft_c2r(1, &n, in, out, flags);
}

fftw_plan FFTWLib::fftw_plan_dft_c2r(int rank, const int* n, fftw_complex* in, double* out, unsigned flags) {
	fftw_iodim64 dims64[MKL_MAXRANK];
	int i, inplace;

	if (rank > MKL_MAXRANK)
		return nullptr;

	if (n == nullptr)
		return nullptr;

	for (i = 0; i < rank; ++i)
		dims64[i].n = n[i];

	/* compute strides, different for inplace and out-of-place */
	inplace = (static_cast<void*>(in) == static_cast<void*>(out));
	if (rank > 0)
	{
		dims64[rank - 1].is = 1;
		dims64[rank - 1].os = 1;
	}
	if (rank > 1)
	{
		dims64[rank - 2].is = dims64[rank - 1].n / 2 + 1;
		dims64[rank - 2].os =
			(inplace ? dims64[rank - 2].is * 2 : dims64[rank - 1].n);
	}
	for (i = rank - 2; i > 0; --i)
	{
		dims64[i - 1].is = dims64[i].is * dims64[i].n;
		dims64[i - 1].os = dims64[i].os * dims64[i].n;
	}

	return fftw_plan_guru64_dft_c2r(rank, dims64, 0, nullptr, in, out, flags);
}

fftw_plan FFTWLib::fftw_plan_dft_r2c(int rank, const int* n, double* in, fftw_complex* out, unsigned flags) {
	fftw_iodim64 dims64[MKL_MAXRANK];
	int i, inplace;

	if (rank > MKL_MAXRANK)
		return nullptr;

	if (n == nullptr)
		return nullptr;

	for (i = 0; i < rank; ++i)
	{
		dims64[i].n = n[i];
	}

	/* compute strides, different for inplace and out-of-place */
	inplace = (static_cast<void*>(in) == static_cast<void*>(out));
	if (rank > 0)
	{
		dims64[rank - 1].is = 1;
		dims64[rank - 1].os = 1;
	}
	if (rank > 1)
	{
		dims64[rank - 2].os = dims64[rank - 1].n / 2 + 1;
		dims64[rank - 2].is =
			(inplace ? dims64[rank - 2].os * 2 : dims64[rank - 1].n);
	}
	for (i = rank - 2; i > 0; --i)
	{
		dims64[i - 1].is = dims64[i].is * dims64[i].n;
		dims64[i - 1].os = dims64[i].os * dims64[i].n;
	}

	return fftw_plan_guru64_dft_r2c(rank, dims64, 0, nullptr, in, out, flags);
}
#else
FFTWLib::FFTWLib() try
    : module_(OpenSharedLibrary("fftw3-3"))
	, XAMP_LOAD_DLL_API(fftw_destroy_plan)
	, XAMP_LOAD_DLL_API(fftw_malloc)
	, XAMP_LOAD_DLL_API(fftw_free)
	, XAMP_LOAD_DLL_API(fftw_execute)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_r2c_1d)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_c2r_1d) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}
#endif

FFTWFLib::FFTWFLib() try
    : module_(OpenSharedLibrary("fftw3f-3"))
	, XAMP_LOAD_DLL_API(fftwf_destroy_plan)
	, XAMP_LOAD_DLL_API(fftwf_malloc)
	, XAMP_LOAD_DLL_API(fftwf_free)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_with_nthreads)
	, XAMP_LOAD_DLL_API(fftwf_init_threads)
	, XAMP_LOAD_DLL_API(fftwf_cleanup_threads) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

FFTWComplexArray MakeFFTWComplexArray(size_t size) {
	return FFTWComplexArray(static_cast<fftw_complex*>(FFTW_LIB.fftw_malloc(sizeof(fftw_complex) * size)));
}

FFTWDoubleArray MakeFFTWDoubleArray(size_t size) {
	return FFTWDoubleArray(static_cast<double*>(FFTW_LIB.fftw_malloc(sizeof(double) * size)));
}

FFTWPlan MakeFFTW(uint32_t fft_size, uint32_t times, FFTWDoubleArray& fft_in, FFTWComplexArray& fft_out) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_r2c_1d(fft_size / times, fft_in.get(), fft_out.get(), FFTW_ESTIMATE));
}

FFTWPlan MakeIFFTW(uint32_t fft_size, uint32_t times, FFTWComplexArray& ifft_in, FFTWDoubleArray& ifft_out) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_c2r_1d(fft_size / times, ifft_in.get(), ifft_out.get(), FFTW_ESTIMATE));
}

FFTWPtr MakeFFTWBuffer(size_t size) {
	return FFTWPtr(static_cast<double*>(FFTW_LIB.fftw_malloc(sizeof(double) * size)));
}

FFTWFPtr MakeFFTWFBuffer(size_t size) {
	return FFTWFPtr(static_cast<float*>(FFTWF_LIB.fftwf_malloc(sizeof(float) * size)));
}

#endif

XAMP_STREAM_NAMESPACE_END
