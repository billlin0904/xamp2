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
 * sfftw_plan_many_r2r - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"

void
sfftw_plan_many_r2r(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *howmany,
                    REAL4 *in, INTEGER *inembed, INTEGER *istride,
                    INTEGER *idist, REAL4 *out, INTEGER *onembed,
                    INTEGER *ostride, INTEGER *odist, INTEGER *kind,
                    INTEGER *flags)
{
    INTEGER one = 1;
    INTEGER is[MKL_MAXRANK];
    INTEGER os[MKL_MAXRANK];
    INTEGER i;

    if (p == NULL || rank == NULL) return;
    if (istride == NULL || ostride == NULL) return;
    if (inembed == NULL || onembed == NULL) return;

    if (*rank > MKL_MAXRANK)
    {
        *(MKL_INT64 *)p = 0;
        return;
    }

    is[0] = *istride;
    os[0] = *ostride;
    for (i = 1; i < *rank; i++)
    {
        is[i] = inembed[i - 1] * is[i - 1];
        os[i] = onembed[i - 1] * os[i - 1];
    }
    sfftw_plan_guru_r2r(p, rank, n, is, os, &one, howmany, idist, odist, in,
                        out, kind, flags);
}
