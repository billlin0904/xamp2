/*******************************************************************************
* Copyright 2008-2019 Intel Corporation.
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
 * fftwf_plan_guru64_r2r - FFTW3 wrapper to Intel(R) MKL.
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"

#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
static const char FFTW3_MKL_1[] =
    "Warning: Consider use of MKL_RODFT00 instead of FFTW_RODFT00 in "
    "planning r2r transforms with FFTW3 interface to Intel(R) MKL "
    "(see Intel(R) MKL Reference Manual).";
static const char FFTW3_MKL_2_I[] =
    "Error: Intel(R) MKL TT initialization failed with status=%d "
    "in FFTW3 interface to Intel(R) MKL.";
static const char FFTW3_MKL_3_I[] =
    "Error: Intel(R) MKL TT commit step failed with status=%d "
    "in FFTW3 interface to Intel(R) MKL.";
static const char FFTW3_MKL_4_I[] =
    "Error: Intel(R) MKL TT backward transform failed with status=%d "
    "in FFTW3 interface to Intel(R) MKL.";
static const char FFTW3_MKL_5_I[] =
    "Error: Intel(R) MKL TT forward transform failed with status=%d "
    "in FFTW3 interface to Intel(R) MKL.";
#endif

static fftwf_plan tt_plan(MKL_INT tt_type, MKL_INT n1, MKL_INT xsz, int ipar15,
                         float *in, float *out, int fwd);

fftwf_plan
fftwf_plan_guru64_r2r(int rank, const fftwf_iodim64 *dims, int howmany_rank,
                     const fftwf_iodim64 *howmany_dims, float *in, float *out,
                     const fftwf_r2r_kind *kind, unsigned flags)
{
    MKL_INT n;

    UNUSED(howmany_dims);
    UNUSED(flags);

    if (dims == NULL || kind == NULL)
        return NULL;

    /* Check unimplemented cases */
    if (rank != 1 || howmany_rank != 0)
        return NULL;

    if (dims[0].is != 1 || dims[0].os != 1)
        return NULL;

    if (dims[0].is != 1 || dims[0].os != 1)
        return NULL;

    n = (MKL_INT) dims[0].n;
    if (n != dims[0].n)
        return NULL;

    if (kind[0] == (fftwf_r2r_kind)MKL_RODFT00)
        return tt_plan(MKL_SINE_TRANSFORM, n + 1, n + 1, 1, in, out, 0);

    switch (kind[0])
    {
    case FFTW_RODFT00:
        /* Warn about MKL_RODFT00 alternative to FFTW_RODFT00 */
#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
        if (fftw3_mkl.verbose)
        {
            printf(FFTW3_MKL_1);
        }
#endif
        return tt_plan(MKL_SINE_TRANSFORM, n + 1, n, 0, in, out, 0);
    case FFTW_REDFT00:
        return tt_plan(MKL_COSINE_TRANSFORM, n - 1, n, 0, in, out, 0);
    case FFTW_REDFT10:
        return tt_plan(MKL_STAGGERED_COSINE_TRANSFORM, n, n, 0, in, out, 0);
    case FFTW_REDFT11:
        return tt_plan(MKL_STAGGERED2_COSINE_TRANSFORM, n, n, 0, in, out, 0);
    case FFTW_REDFT01:
        return tt_plan(MKL_STAGGERED_COSINE_TRANSFORM, n, n, 0, in, out, 1);
    case FFTW_RODFT11:
        return tt_plan(MKL_STAGGERED2_SINE_TRANSFORM, n, n, 0, in, out, 0);
    case FFTW_RODFT01:
        return tt_plan(MKL_STAGGERED_SINE_TRANSFORM, n, n, 0, in, out, 1);
    case FFTW_RODFT10:
        return tt_plan(MKL_STAGGERED_SINE_TRANSFORM, n, n, 0, in, out, 0);
    case FFTW_R2HC:
    case FFTW_HC2R:
    case FFTW_DHT:
    default:
        return NULL;
    }
}

static void
tt_fftwf_in_to_out(float *in, float *out, MKL_INT n)
{
    MKL_INT i;

    if (in != out)
    {
        for (i = 0; i < n; i++)
            out[i] = in[i];
    }
}

static void
tt_execute_b_norm(fftw_mkl_plan p)
{
    float *spar = p->spar;
    MKL_INT *ipar = p->ipar;
    float *in = p->io[0];
    float *out = p->io[1];
    MKL_INT i, n, ir;

    n = ipar[0];
    if (n > 0)
    {
        spar[n / 2 + 2] = 0.0;
        for (i = 1; i < n; i++)
            spar[n / 2 + 2 + i] = in[i - 1];
        s_backward_trig_transform(&spar[n / 2 + 2], &p->desc, ipar, spar, &ir);
        for (i = 1; i < ipar[0]; i++)
            out[i - 1] = spar[n / 2 + 2 + i];

#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
        if (ir && fftw3_mkl.verbose)
        {
            printf(FFTW3_MKL_4_I, (int)ir);
        }
#endif
    }
}

static void
tt_execute_b(fftw_mkl_plan p)
{
    float *in = p->io[0];
    float *out = p->io[1];
    MKL_INT ir;

    if (p->ipar[5] == MKL_SINE_TRANSFORM && p->ipar[15] == 0)
    {
        tt_execute_b_norm(p);
    }
    else
    {
        if (in != out)
        {
            tt_fftwf_in_to_out(in, out, p->ipar[14]);
        }
        s_backward_trig_transform(out, &p->desc, p->ipar, p->spar, &ir);
#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
        if (ir && fftw3_mkl.verbose)
        {
            printf(FFTW3_MKL_4_I, (int)ir);
        }
#endif
    }
}

static void
tt_execute_f(fftw_mkl_plan p)
{
    float *in = p->io[0];
    float *out = p->io[1];
    MKL_INT ir;

    if (in != out)
    {
        tt_fftwf_in_to_out(in, out, p->ipar[14]);
    }
    s_forward_trig_transform(out, &p->desc, p->ipar, p->spar, &ir);
#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
    if (ir && fftw3_mkl.verbose)
    {
        printf(FFTW3_MKL_5_I, (int)ir);
    }
#endif
}

static fftwf_plan
tt_plan(MKL_INT tt_type, MKL_INT n1, MKL_INT xsz, int ipar15, float *in,
        float *out, int fwd)
{
    fftw_mkl_plan plan;
    MKL_INT ir;

    plan = fftw3_mkl.new_plan();
    if (!plan)
        return NULL;

    plan->spar = (float *)fftwf_malloc((5 * n1 / 2 + 2) * sizeof(float));
    plan->ipar = (MKL_INT *) fftwf_malloc(128 * sizeof(MKL_INT));
    if (!plan->spar || !plan->ipar)
        goto broken;
    plan->ipar[1] = fftw3_mkl.verbose ? 1 : 0;
    plan->ipar[2] = fftw3_mkl.verbose ? 1 : 0;
    plan->ipar[9] = fftw3_mkl.nthreads;
    s_init_trig_transform(&n1, &tt_type, plan->ipar, plan->spar, &ir);
    if (ir)
    {
#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
        if (fftw3_mkl.verbose)
        {

            printf(FFTW3_MKL_2_I, (int)ir);
        }
#endif
        goto broken;
    }
    plan->ipar[10] = 1; /* compatibility with FFTW */
    plan->ipar[14] = xsz;
    plan->ipar[15] = ipar15;
    s_commit_trig_transform(out, &plan->desc, plan->ipar, plan->spar, &ir);
    if (ir)
    {
#if defined(FFTW_VERBOSE_PRINT_R2R_MESSAGES)
        if (fftw3_mkl.verbose)
        {
            printf(FFTW3_MKL_3_I, (int)ir);
        }
#endif
        goto broken;
    }

    plan->io[0] = in;
    plan->io[1] = out;
    plan->execute = fwd ? tt_execute_f : tt_execute_b;

    return (fftwf_plan)plan;

  broken:
    plan->destroy(plan);
    return NULL;
}
