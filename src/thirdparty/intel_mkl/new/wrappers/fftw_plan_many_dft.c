/*******************************************************************************
* Copyright 2005-2022 Intel Corporation.
*
* This software and the related documents are Intel copyrighted  materials,  and
* your use of  them is  governed by the  express license  under which  they were
* provided to you (License).  Unless the License provides otherwise, you may not
* use, modify, copy, publish, distribute,  disclose or transmit this software or
* the related documents without Intel's prior written permission.
*
* This software and the related documents  are provided as  is,  with no express
* or implied  warranties,  other  than those  that are  expressly stated  in the
* License.
*******************************************************************************/

/*
 *
 * fftw_plan_many_dft - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT fftw_plan
fftw_plan_many_dft_omp_offload(int rank, const int *n, int howmany, fftw_complex *in,
                               const int *inembed, int istride, int idist,
                               fftw_complex *out, const int *onembed, int ostride,
                               int odist, int sign, unsigned flags, void* interopObj)
#else
fftw_plan
fftw_plan_many_dft(int rank, const int *n, int howmany, fftw_complex *in,
                   const int *inembed, int istride, int idist,
                   fftw_complex *out, const int *onembed, int ostride,
                   int odist, int sign, unsigned flags)
#endif
{
    fftw_iodim64 dims64[MKL_MAXRANK];
    fftw_iodim64 howmany64;
    int i;

    if (rank > MKL_MAXRANK)
        return NULL;

    if (n == NULL)
        return NULL;

    for (i = 0; i < rank; ++i)
    {
        dims64[i].n = n[i];
    }
    if (rank > 0)
    {
        dims64[rank - 1].is = istride;
        dims64[rank - 1].os = ostride;
    }
    if (!inembed)
        inembed = n;
    if (!onembed)
        onembed = n;
    for (i = rank - 1; i > 0; --i)
    {
        dims64[i - 1].is = dims64[i].is * inembed[i];
        dims64[i - 1].os = dims64[i].os * onembed[i];
    }

    howmany64.n = howmany;
    howmany64.is = idist;
    howmany64.os = odist;

#ifdef DFT_ENABLE_OFFLOAD
    if(mkl_dfti_is_ilp64 == 0)
      return fftw_plan_guru64_dft_omp_offload_impl_lp64(rank, dims64, 1, &howmany64,
                                                   in, out, sign, flags, interopObj);
    else
      return fftw_plan_guru64_dft_omp_offload_impl_ilp64(rank, dims64, 1, &howmany64,
                                                   in, out, sign, flags, interopObj);

#else
    return fftw_plan_guru64_dft(rank, dims64, 1, &howmany64, in, out, sign,
                                flags);
#endif
}
