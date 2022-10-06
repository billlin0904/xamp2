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
 * sfftw_plan_r2r - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"

void
sfftw_plan_r2r(PLAN *p, INTEGER *rank, INTEGER *n, REAL4 *in, REAL4 *out,
               INTEGER *kind, INTEGER *flags)
{
    INTEGER zero = 0;
    INTEGER is[MKL_MAXRANK];
    INTEGER os[MKL_MAXRANK];
    int i;

    if (p == NULL || rank == NULL || n == NULL)
        return;

    if (*rank > MKL_MAXRANK)
    {
        *(MKL_INT64 *)p = 0;
        return;
    }

    for (i = 0; i < *rank; i++)
    {
        if (i == 0)
        {
            is[i] = 1;
            os[i] = 1;
        }
        else
        {
            is[i] = is[i - 1] * n[i - 1];
            os[i] = os[i - 1] * n[i - 1];
        }
    }

    sfftw_plan_guru_r2r(p, rank, n, is, os, &zero, NULL, NULL, NULL, in, out,
                        kind, flags);
}
