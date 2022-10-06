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
 * fftw_plan_many_dft_c2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

fftw_plan
fftw_plan_many_dft_c2r(int rank, const int *n, int howmany, fftw_complex *in,
                       const int *inembed, int istride, int idist, double *out,
                       const int *onembed, int ostride, int odist,
                       unsigned flags)
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

    /* compute strides, different for inplace and out-of-place */
    if (rank > 0)
    {
        dims64[rank - 1].is = istride;
        dims64[rank - 1].os = ostride;
    }

    for (i = rank - 1; i > 0; --i)
    {
        int inplace = (void*)in == (void*)out;
        /* compute instrides based on inembed input */
        dims64[i - 1].is = dims64[i].is *
            (inembed ? inembed[i] : i == rank-1 ? n[i]/2+1 : n[i]);
        /* compute outstrides based on onembed input */
        dims64[i - 1].os = dims64[i].os *
            (onembed ? onembed[i]
                     : (i == rank-1 && inplace) ? 2*(n[i]/2+1) : n[i]);
    }

    howmany64.n = howmany;
    howmany64.is = idist;
    howmany64.os = odist;

    return fftw_plan_guru64_dft_c2r(rank, dims64, 1, &howmany64, in, out,
                                    flags);
}
