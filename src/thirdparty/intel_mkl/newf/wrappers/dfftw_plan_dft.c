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
 * dfftw_plan_dft - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT void
dfftw_plan_dft_omp_offload(PLAN *p, INTEGER *rank, INTEGER *n, CFI_cdesc_t *in,
              CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags, void *interopObj)
{
    dfftw_plan_dft_omp_offload_impl(p, rank, n, (COMPLEX16 *)in->base_addr,
                          (COMPLEX16 *)out->base_addr, sign, flags, interopObj);
}

DLL_EXPORT void
dfftw_plan_dft_cpu(PLAN *p, INTEGER *rank, INTEGER *n, CFI_cdesc_t *in,
                   CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags)
{
    dfftw_plan_dft(p, rank, n, (COMPLEX16 *)in->base_addr,
                   (COMPLEX16 *)out->base_addr, sign, flags);
}

void
dfftw_plan_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, COMPLEX16 *in,
               COMPLEX16 *out, INTEGER *sign, INTEGER *flags, void *interopObj)
#else
void
dfftw_plan_dft(PLAN *p, INTEGER *rank, INTEGER *n, COMPLEX16 *in,
               COMPLEX16 *out, INTEGER *sign, INTEGER *flags)
#endif
{
    fftw_iodim64 dims64[MKL_MAXRANK];
    int i;

    if (p == NULL || rank == NULL || n == NULL || sign == NULL || flags == NULL)
        return;

    *(MKL_INT64 *)p = 0;
    if (*rank > MKL_MAXRANK) return;

    /* Reverse dimensions for column-major layout */
    for (i = 0; i < *rank; i++)
    {
        int j = *rank - i - 1;
        dims64[j].n = n[i];
        if (i == 0)
        {
            dims64[j].is = 1;
            dims64[j].os = 1;
        }
        else
        {
            dims64[j].is = dims64[j + 1].is * dims64[j + 1].n;
            dims64[j].os = dims64[j + 1].os * dims64[j + 1].n;
        }
    }
#ifdef DFT_ENABLE_OFFLOAD

#ifdef MKL_ILP64
      *(fftw_plan *)p = fftw_plan_guru64_dft_omp_offload_impl_ilp64(*rank, dims64, 0, NULL,
                                                   in, out, *sign, *flags, interopObj);
#else
      *(fftw_plan *)p = fftw_plan_guru64_dft_omp_offload_impl_lp64(*rank, dims64, 0, NULL,
                                                   in, out, *sign, *flags, interopObj);
#endif
#else

    *(fftw_plan *)p =
        fftw_plan_guru64_dft(*rank, dims64, 0, NULL, in, out, *sign, *flags);
#endif
}
