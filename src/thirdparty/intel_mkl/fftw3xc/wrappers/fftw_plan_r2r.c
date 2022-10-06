/*******************************************************************************
* Copyright 2005-2019 Intel Corporation.
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
 * fftw_plan_r2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

fftw_plan
fftw_plan_r2r(int rank, const int *n, double *in, double *out,
              const fftw_r2r_kind * kind, unsigned flags)
{
    fftw_iodim64 dims64[MKL_MAXRANK];
    int i;

    if (rank > MKL_MAXRANK)
        return NULL;

    if (n == NULL)
        return NULL;

    for (i = 0; i < rank; ++i)
        dims64[i].n = n[i];

    if (rank > 0)
    {
        dims64[rank - 1].is = 1;
        dims64[rank - 1].os = 1;
    }

    for (i = rank - 1; i > 0; --i)
    {
        dims64[i - 1].is = dims64[i].is * dims64[i].n;
        dims64[i - 1].os = dims64[i].os * dims64[i].n;
    }

    return fftw_plan_guru64_r2r(rank, dims64, 0, NULL, in, out, kind, flags);
}
