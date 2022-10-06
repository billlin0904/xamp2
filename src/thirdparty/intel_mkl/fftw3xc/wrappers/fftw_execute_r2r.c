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
 * fftw_execute_r2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

void
fftw_execute_r2r(const fftw_plan plan, double *in, double *out)
{
    fftw_mkl_plan mkl_plan = (fftw_mkl_plan)plan;
    struct fftw_mkl_plan_s tmp_plan_s;

    if (!(mkl_plan && mkl_plan->execute)) return;

    tmp_plan_s = mkl_plan[0];
    tmp_plan_s.io[0] = in;
    tmp_plan_s.io[1] = out;

    tmp_plan_s.execute(&tmp_plan_s);
}
