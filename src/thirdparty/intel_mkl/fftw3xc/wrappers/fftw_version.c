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
 * fftw_version etc - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

const char fftw_version[] = "FFTW 3.3.4 wrappers to Intel(R) MKL";
const char fftw_cc[] = "";
const char fftw_codelet_optim[] = "";

/* Supplementary definitions for the wrappers */

static void
delete_plan(fftw_mkl_plan p)
{
    if (p)
    {
        if (p->spar)
            fftw_free(p->spar);
        if (p->dpar)
            fftw_free(p->dpar);
        if (p->ipar)
            fftw_free(p->ipar);
        if (p->desc)
            DftiFreeDescriptor(&p->desc);
        fftw_free(p);
    }
}

static fftw_mkl_plan
new_plan(void)
{
    fftw_mkl_plan p = (fftw_mkl_plan)fftw_malloc(sizeof(*p));

    if (p)
    {
        p->desc = NULL;
        p->io[0] = p->io[1] = p->io[2] = p->io[3] = 0;
        p->ipar = NULL;
        p->dpar = NULL;
        p->spar = NULL;
        p->execute = NULL;
        p->destroy = delete_plan;
        p->mpi_plan = NULL;
    }
    return p;
}

#ifndef MKL_FFTW_MALLOC_ALIGNMENT
#define MKL_FFTW_MALLOC_ALIGNMENT (64)
#endif

fftw3_mkl_s fftw3_mkl = {
    0,  /* verbose */
    1,  /* nthreads */
    0.0,        /* timelimit */
    1,  /* Number of user threads variable. Will be depricated in nearest future */
    new_plan,
    MKL_FFTW_MALLOC_ALIGNMENT /* default_alignment */
};
