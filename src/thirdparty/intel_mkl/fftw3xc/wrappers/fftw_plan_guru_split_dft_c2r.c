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
 * fftw_plan_guru_split_dft_c2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

fftw_plan
fftw_plan_guru_split_dft_c2r(int rank, const fftw_iodim *dims,
                             int howmany_rank, const fftw_iodim *howmany_dims,
                             double *ri, double *ii, double *out,
                             unsigned flags)
{
    /* Intel(R) MKL DFTI doesn't support split complex to real */
    UNUSED(rank);
    UNUSED(dims);
    UNUSED(howmany_rank);
    UNUSED(howmany_dims);
    UNUSED(ri);
    UNUSED(ii);
    UNUSED(out);
    UNUSED(flags);
    return NULL;
}
