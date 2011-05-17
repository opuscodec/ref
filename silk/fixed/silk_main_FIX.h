/***********************************************************************
Copyright (c) 2006-2011, Skype Limited. All rights reserved. 
Redistribution and use in source and binary forms, with or without 
modification, (subject to the limitations in the disclaimer below) 
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution.
- Neither the name of Skype Limited, nor the names of specific 
contributors, may be used to endorse or promote products derived from 
this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED 
BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
CONTRIBUTORS ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#ifndef SILK_MAIN_FIX_H
#define SILK_MAIN_FIX_H

#include "silk_SigProc_FIX.h"
#include "silk_structs_FIX.h"
#include "silk_control.h"
#include "silk_main.h"
#include "silk_PLC.h"
#include "silk_debug.h"
#include "entenc.h"

#ifndef FORCE_CPP_BUILD
#ifdef __cplusplus
extern "C"
{
#endif
#endif

#define silk_encoder_state_Fxx      silk_encoder_state_FIX
#define silk_encode_frame_Fxx       silk_encode_frame_FIX

/*********************/
/* Encoder Functions */
/*********************/

/* High-pass filter with cutoff frequency adaptation based on pitch lag statistics */
void silk_HP_variable_cutoff(
    silk_encoder_state_Fxx          state_Fxx[],    /* I/O  Encoder states                              */
    const SKP_int                   nChannels       /* I    Number of channels                          */
);

/* Encoder main function */
SKP_int silk_encode_frame_FIX( 
    silk_encoder_state_FIX          *psEnc,             /* I/O  Pointer to Silk FIX encoder state       */
    SKP_int32                       *pnBytesOut,        /*   O  Pointer to number of payload bytes;     */
    ec_enc                          *psRangeEnc         /* I/O  compressor data structure               */
);

/* Low Bitrate Redundancy (LBRR) encoding. Reuse all parameters but encode with lower bitrate           */
void silk_LBRR_encode_FIX(
    silk_encoder_state_FIX          *psEnc,             /* I/O  Pointer to Silk FIX encoder state           */
    silk_encoder_control_FIX        *psEncCtrl,         /* I/O  Pointer to Silk FIX encoder control struct  */
    const SKP_int16                 xfw[]               /* I    Input signal                                */
);

/* Initializes the Silk encoder state */
SKP_int silk_init_encoder(
    silk_encoder_state_FIX          *psEnc              /* I/O  Pointer to Silk FIX encoder state           */
);

/* Control the Silk encoder */
SKP_int silk_control_encoder( 
    silk_encoder_state_FIX          *psEnc,             /* I/O  Pointer to Silk encoder state           */
    silk_EncControlStruct           *encControl,        /* I:   Control structure                       */
    const SKP_int32                 TargetRate_bps,     /* I    Target max bitrate (bps)                */
    const SKP_int                   allow_bw_switch,    /* I    Flag to allow switching audio bandwidth */
    const SKP_int                   channelNb           /* I    Channel number                          */
);

/****************/
/* Prefiltering */
/****************/
void silk_prefilter_FIX(
    silk_encoder_state_FIX              *psEnc,         /* I/O  Encoder state                               */
    const silk_encoder_control_FIX      *psEncCtrl,     /* I    Encoder control                             */
    SKP_int16                           xw[],           /* O    Weighted signal                             */
    const SKP_int16                     x[]             /* I    Speech signal                               */
);

/**************************/
/* Noise shaping analysis */
/**************************/
/* Compute noise shaping coefficients and initial gain values */
void silk_noise_shape_analysis_FIX(
    silk_encoder_state_FIX          *psEnc,         /* I/O  Encoder state FIX                           */
    silk_encoder_control_FIX        *psEncCtrl,     /* I/O  Encoder control FIX                         */
    const SKP_int16                 *pitch_res,     /* I    LPC residual from pitch analysis            */
    const SKP_int16                 *x              /* I    Input signal [ frame_length + la_shape ]    */
);

/* Autocorrelations for a warped frequency axis */
void silk_warped_autocorrelation_FIX(
          SKP_int32                 *corr,              /* O    Result [order + 1]                      */
          SKP_int                   *scale,             /* O    Scaling of the correlation vector       */
    const SKP_int16                 *input,             /* I    Input data to correlate                 */
    const SKP_int                   warping_Q16,        /* I    Warping coefficient                     */
    const SKP_int                   length,             /* I    Length of input                         */
    const SKP_int                   order               /* I    Correlation order (even)                */
);

/* Calculation of LTP state scaling */
void silk_LTP_scale_ctrl_FIX(
    silk_encoder_state_FIX          *psEnc,         /* I/O  encoder state                               */
    silk_encoder_control_FIX        *psEncCtrl      /* I/O  encoder control                             */
);

/**********************************************/
/* Prediction Analysis                        */
/**********************************************/
/* Find pitch lags */
void silk_find_pitch_lags_FIX(
    silk_encoder_state_FIX          *psEnc,         /* I/O  encoder state                               */
    silk_encoder_control_FIX        *psEncCtrl,     /* I/O  encoder control                             */
    SKP_int16                       res[],          /* O    residual                                    */
    const SKP_int16                 x[]             /* I    Speech signal                               */
);

/* Find LPC and LTP coefficients */
void silk_find_pred_coefs_FIX(
    silk_encoder_state_FIX          *psEnc,         /* I/O  encoder state                               */
    silk_encoder_control_FIX        *psEncCtrl,     /* I/O  encoder control                             */
    const SKP_int16                 res_pitch[],    /* I    Residual from pitch analysis                */
    const SKP_int16                 x[]             /* I    Speech signal                               */
);

/* LPC analysis */
void silk_find_LPC_FIX(
    SKP_int16                       NLSF_Q15[],             /* O    NLSFs                                                           */
    SKP_int8                        *interpIndex,           /* O    NLSF interpolation index, only used for NLSF interpolation      */
    const SKP_int16                 prev_NLSFq_Q15[],       /* I    previous NLSFs, only used for NLSF interpolation                */
    const SKP_int                   useInterpNLSFs,         /* I    Flag                                                            */
    const SKP_int                   firstFrameAfterReset,   /* I    Flag                                                            */
    const SKP_int                   LPC_order,              /* I    LPC order                                                       */
    const SKP_int16                 x[],                    /* I    Input signal                                                    */
    const SKP_int                   subfr_length,           /* I    Input signal subframe length including preceeding samples       */
    const SKP_int                   nb_subfr                /* I:   Number of subframes                                             */
);

/* LTP analysis */
void silk_find_LTP_FIX(
    SKP_int16           b_Q14[ MAX_NB_SUBFR * LTP_ORDER ],              /* O    LTP coefs                                                   */
    SKP_int32           WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ],   /* O    Weight for LTP quantization                                 */
    SKP_int             *LTPredCodGain_Q7,                              /* O    LTP coding gain                                             */
    const SKP_int16     r_lpc[],                                        /* I    residual signal after LPC signal + state for first 10 ms    */
    const SKP_int       lag[ MAX_NB_SUBFR ],                            /* I    LTP lags                                                    */
    const SKP_int32     Wght_Q15[ MAX_NB_SUBFR ],                       /* I    weights                                                     */
    const SKP_int       subfr_length,                                   /* I    subframe length                                             */
    const SKP_int       nb_subfr,                                       /* I    number of subframes                                         */
    const SKP_int       mem_offset,                                     /* I    number of samples in LTP memory                             */
    SKP_int             corr_rshifts[ MAX_NB_SUBFR ]                    /* O    right shifts applied to correlations                        */
);

void silk_LTP_analysis_filter_FIX(
    SKP_int16           *LTP_res,                               /* O:   LTP residual signal of length MAX_NB_SUBFR * ( pre_length + subfr_length )  */
    const SKP_int16     *x,                                     /* I:   Pointer to input signal with at least max( pitchL ) preceeding samples      */
    const SKP_int16     LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],/* I:   LTP_ORDER LTP coefficients for each MAX_NB_SUBFR subframe                   */
    const SKP_int       pitchL[ MAX_NB_SUBFR ],                 /* I:   Pitch lag, one for each subframe                                            */
    const SKP_int32     invGains_Q16[ MAX_NB_SUBFR ],           /* I:   Inverse quantization gains, one for each subframe                           */
    const SKP_int       subfr_length,                           /* I:   Length of each subframe                                                     */
    const SKP_int       nb_subfr,                               /* I:   Number of subframes                                                         */
    const SKP_int       pre_length                              /* I:   Length of the preceeding samples starting at &x[0] for each subframe        */
);

/* Calculates residual energies of input subframes where all subframes have LPC_order   */
/* of preceeding samples                                                                */
void silk_residual_energy_FIX(
          SKP_int32 nrgs[ MAX_NB_SUBFR ],           /* O    Residual energy per subframe    */
          SKP_int   nrgsQ[ MAX_NB_SUBFR ],          /* O    Q value per subframe            */
    const SKP_int16 x[],                            /* I    Input signal                    */
    const SKP_int16 a_Q12[ 2 ][ MAX_LPC_ORDER ],    /* I    AR coefs for each frame half    */
    const SKP_int32 gains[ MAX_NB_SUBFR ],          /* I    Quantization gains              */
    const SKP_int   subfr_length,                   /* I    Subframe length                 */
    const SKP_int   nb_subfr,                       /* I    Number of subframes             */
    const SKP_int   LPC_order                       /* I    LPC order                       */
);

/* Residual energy: nrg = wxx - 2 * wXx * c + c' * wXX * c */
SKP_int32 silk_residual_energy16_covar_FIX(
    const SKP_int16                 *c,                 /* I    Prediction vector                           */
    const SKP_int32                 *wXX,               /* I    Correlation matrix                          */
    const SKP_int32                 *wXx,               /* I    Correlation vector                          */
    SKP_int32                       wxx,                /* I    Signal energy                               */
    SKP_int                         D,                  /* I    Dimension                                   */
    SKP_int                         cQ                  /* I    Q value for c vector 0 - 15                 */
);

/* Processing of gains */
void silk_process_gains_FIX(
    silk_encoder_state_FIX          *psEnc,         /* I/O  Encoder state                               */
    silk_encoder_control_FIX        *psEncCtrl      /* I/O  Encoder control                             */
);

/******************/
/* Linear Algebra */
/******************/
/* Calculates correlation matrix X'*X */
void silk_corrMatrix_FIX(
    const SKP_int16                 *x,         /* I    x vector [L + order - 1] used to form data matrix X */
    const SKP_int                   L,          /* I    Length of vectors                                   */
    const SKP_int                   order,      /* I    Max lag for correlation                             */
    const SKP_int                   head_room,  /* I    Desired headroom                                    */
    SKP_int32                       *XX,        /* O    Pointer to X'*X correlation matrix [ order x order ]*/
    SKP_int                         *rshifts    /* I/O  Right shifts of correlations                        */
);

/* Calculates correlation vector X'*t */
void silk_corrVector_FIX(
    const SKP_int16                 *x,         /* I    x vector [L + order - 1] used to form data matrix X */
    const SKP_int16                 *t,         /* I    Target vector [L]                                   */
    const SKP_int                   L,          /* I    Length of vectors                                   */
    const SKP_int                   order,      /* I    Max lag for correlation                             */
    SKP_int32                       *Xt,        /* O    Pointer to X'*t correlation vector [order]          */
    const SKP_int                   rshifts     /* I    Right shifts of correlations                        */
);

/* Add noise to matrix diagonal */
void silk_regularize_correlations_FIX(
    SKP_int32                       *XX,                /* I/O  Correlation matrices                        */
    SKP_int32                       *xx,                /* I/O  Correlation values                          */
    SKP_int32                       noise,              /* I    Noise to add                                */
    SKP_int                         D                   /* I    Dimension of XX                             */
);

/* Solves Ax = b, assuming A is symmetric */
void silk_solve_LDL_FIX(
    SKP_int32                       *A,                 /* I    Pointer to symetric square matrix A         */
    SKP_int                         M,                  /* I    Size of matrix                              */
    const SKP_int32                 *b,                 /* I    Pointer to b vector                         */
    SKP_int32                       *x_Q16              /* O    Pointer to x solution vector                */
);

#ifndef FORCE_CPP_BUILD
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FORCE_CPP_BUILD */
#endif /* SILK_MAIN_FIX_H */
