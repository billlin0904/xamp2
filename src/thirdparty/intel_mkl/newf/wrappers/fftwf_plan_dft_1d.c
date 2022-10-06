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
 * fftwf_plan_dft_1d - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT fftwf_plan
fftwf_plan_dft_1d_omp_offload(int n, fftwf_complex *in, fftwf_complex *out, int sign,
                              unsigned flags, void *interopObj)
{
    return fftwf_plan_dft_omp_offload_impl(1, &n, in, out, sign, flags, interopObj);
}
#else
fftwf_plan
fftwf_plan_dft_1d(int n, fftwf_complex *in, fftwf_complex *out, int sign,
                 unsigned flags)
{
    return fftwf_plan_dft(1, &n, in, out, sign, flags);
}
#endif
