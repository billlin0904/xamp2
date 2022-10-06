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
 * fftwf_execute - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT void
fftwf_execute_omp_offload(const fftwf_plan plan, void *interopObj)
{
    fftw_mkl_plan mkl_plan = (fftw_mkl_plan)plan;

    if (!(mkl_plan && mkl_plan->execute_offload)) return;

    mkl_plan->execute_offload(mkl_plan, interopObj);
}
#else
void
fftwf_execute(const fftwf_plan plan)
{
    fftw_mkl_plan mkl_plan = (fftw_mkl_plan)plan;

    if (!(mkl_plan && mkl_plan->execute)) return;

    mkl_plan->execute(mkl_plan);
}
#endif
