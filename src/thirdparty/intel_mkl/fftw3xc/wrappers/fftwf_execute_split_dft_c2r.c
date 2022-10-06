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
 * fftwf_execute_split_dft_c2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

void
fftwf_execute_split_dft_c2r(const fftwf_plan plan, float *ri, float *ii,
                           float *out)
{
    /* Intel(R) MKL DFTI doesn't support split complex to real */
    UNUSED(plan);
    UNUSED(ri);
    UNUSED(ii);
    UNUSED(out);
    return;
}
