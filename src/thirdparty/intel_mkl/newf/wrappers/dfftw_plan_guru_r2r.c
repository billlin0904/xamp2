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
 * dfftw_plan_guru_r2r - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"

void
dfftw_plan_guru_r2r(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, REAL8 *in,
                    REAL8 *out, INTEGER *kind, INTEGER *flags)
{
    fftw_iodim64 dims64[MKL_MAXRANK];
    fftw_iodim64 howmany_dims64[MKL_ONE];
    fftw_r2r_kind knd[MKL_MAXRANK];
    int i;

    if (p == NULL || rank == NULL || n == NULL || kind == NULL || flags == NULL)
        return;
    if (is == NULL || os == NULL) return;
    if (howmany_rank == NULL) return;
    if (*howmany_rank > 0 && (howmany_n == NULL || howmany_is == NULL || howmany_os == NULL)) return;

    *(MKL_INT64 *)p = 0;
    if (*rank > MKL_MAXRANK || *howmany_rank > MKL_ONE) return;

    for (i = 0; i < *rank; ++i)
    {
        dims64[i].n = n[*rank - i - 1];
        dims64[i].is = is[*rank - i - 1];
        dims64[i].os = os[*rank - i - 1];
        knd[i] = (fftw_r2r_kind) kind[*rank - i - 1];
    }
    for (i = 0; i < *howmany_rank; ++i)
    {
        howmany_dims64[i].n = howmany_n[*rank - i - 1];
        howmany_dims64[i].is = howmany_is[*rank - i - 1];
        howmany_dims64[i].os = howmany_os[*rank - i - 1];
    }
    *(fftw_plan *)p =
        fftw_plan_guru64_r2r(*rank, dims64, *howmany_rank, howmany_dims64, in,
                             out, knd, *flags);
}
