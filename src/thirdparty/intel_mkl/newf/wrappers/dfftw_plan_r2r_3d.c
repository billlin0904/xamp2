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
 * dfftw_plan_r2r_3d - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"

void
dfftw_plan_r2r_3d(PLAN *p, INTEGER *nx, INTEGER *ny, INTEGER *nz, REAL8 *in,
                  REAL8 *out, INTEGER *kindx, INTEGER *kindy, INTEGER *kindz,
                  INTEGER *flags)
{
    if (p == NULL) return;
    *p = 0;
    if(nx != NULL && ny != NULL && nz != NULL && kindx != NULL && kindy != NULL && kindz != NULL) {
        INTEGER three = 3;
        INTEGER n[3] = { *nx, *ny, *nz };
        INTEGER knd[3] = { *kindx, *kindy, *kindz };

        dfftw_plan_r2r(p, &three, n, in, out, knd, flags);
    }
}