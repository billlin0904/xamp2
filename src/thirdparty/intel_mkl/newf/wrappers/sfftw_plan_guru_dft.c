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
 * sfftw_plan_guru_dft - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT void
sfftw_plan_guru_dft_omp_offload(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, CFI_cdesc_t *in,
                    CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags, void *interopObj)
{
    sfftw_plan_guru_dft_omp_offload_impl(p, rank, n, is, os, howmany_rank, howmany_n,
                                         howmany_is, howmany_os,
                                         (COMPLEX8 *)in->base_addr,
                                         (COMPLEX8 *)out->base_addr, sign,
                                         flags, interopObj);
}

DLL_EXPORT void
sfftw_plan_guru_dft_cpu(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, CFI_cdesc_t *in,
                    CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags)
{
    sfftw_plan_guru_dft(p, rank, n, is, os, howmany_rank, howmany_n,
                        howmany_is, howmany_os, (COMPLEX8 *)in->base_addr,
                        (COMPLEX8 *)out->base_addr, sign, flags);
}

void
sfftw_plan_guru_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, COMPLEX8 *in,
                    COMPLEX8 *out, INTEGER *sign, INTEGER *flags, void *interopObj)
#else
void
sfftw_plan_guru_dft(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, COMPLEX8 *in,
                    COMPLEX8 *out, INTEGER *sign, INTEGER *flags)
#endif
{
    fftwf_iodim64 dims64[MKL_MAXRANK];
    fftwf_iodim64 howmany_dims64[MKL_ONE];
    INTEGER i;

    if (p == NULL || rank == NULL || n == NULL || sign == NULL || flags == NULL)
        return;
    if (is == NULL || os == NULL) return;
    if (howmany_rank == NULL) return;
    if (*howmany_rank > 0 && (howmany_n == NULL || howmany_is == NULL || howmany_os == NULL)) return;

    *(MKL_INT64 *)p = 0;
    if (*rank > MKL_MAXRANK || *howmany_rank > MKL_ONE) return;

    for (i = 0; i < *rank; i++)
    {
        dims64[i].n = n[*rank - i - 1];
        dims64[i].is = is[*rank - i - 1];
        dims64[i].os = os[*rank - i - 1];
    }
    for (i = 0; i < *howmany_rank; i++)
    {
        howmany_dims64[i].n = howmany_n[i];
        howmany_dims64[i].is = howmany_is[i];
        howmany_dims64[i].os = howmany_os[i];
    }

#ifdef DFT_ENABLE_OFFLOAD
#ifdef MKL_ILP64
    *(fftwf_plan *)p =
        fftwf_plan_guru64_dft_omp_offload_impl_ilp64(*rank, dims64, *howmany_rank,
                               howmany_dims64, in, out, *sign, *flags, interopObj);
#else
    *(fftwf_plan *)p =
        fftwf_plan_guru64_dft_omp_offload_impl_lp64(*rank, dims64, *howmany_rank,
                               howmany_dims64, in, out, *sign, *flags, interopObj);
#endif
#else
    *(fftwf_plan *)p =
        fftwf_plan_guru64_dft(*rank, dims64, *howmany_rank, howmany_dims64, in,
                             out, *sign, *flags);
#endif
}
