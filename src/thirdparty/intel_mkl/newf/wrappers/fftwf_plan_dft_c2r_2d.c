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
 * fftwf_plan_dft_c2r_2d - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

fftwf_plan
fftwf_plan_dft_c2r_2d(int nx, int ny, fftwf_complex *in, float *out,
                     unsigned flags)
{
    int n[2] = { nx, ny };
    return fftwf_plan_dft_c2r(2, n, in, out, flags);
}