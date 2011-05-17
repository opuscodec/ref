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

#ifndef SILK_MAIN_H
#define SILK_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "silk_SigProc_FIX.h"
#include "silk_define.h"
#include "silk_structs.h"
#include "silk_tables.h"
#include "silk_PLC.h"
#include "silk_control.h"
#include "silk_debug.h"
#include "entenc.h"
#include "entdec.h"


/* Uncomment the next line to store intermadiate data to files */
//#define SAVE_ALL_INTERNAL_DATA      1
/* Uncomment the next line to force a fixed internal sampling rate (independent of what bitrate is used */
//#define FORCE_INTERNAL_FS_KHZ       16


/* Convert Left/Right stereo signal to adaptive Mid/Side representation */
void silk_stereo_LR_to_MS( 
    stereo_enc_state    *state,                         /* I/O  State                                       */
    SKP_int16           x1[],                           /* I/O  Left input signal, becomes mid signal       */
    SKP_int16           x2[],                           /* I/O  Right input signal, becomes side signal     */
    SKP_int8            ix[ 2 ][ 4 ],                   /* O    Quantization indices                        */
    SKP_int32           mid_side_rates_bps[],           /* O    Bitrates for mid and side signals           */
    SKP_int32           total_rate_bps,                 /* I    Total bitrate                               */
    SKP_int             prev_speech_act_Q8,             /* I    Speech activity level in previous frame     */
    SKP_int             fs_kHz,                         /* I    Sample rate (kHz)                           */
    SKP_int             frame_length                    /* I    Number of samples                           */
);

/* Convert adaptive Mid/Side representation to Left/Right stereo signal */
void silk_stereo_MS_to_LR( 
    stereo_dec_state    *state,                         /* I/O  State                                       */
    SKP_int16           x1[],                           /* I/O  Left input signal, becomes mid signal       */
    SKP_int16           x2[],                           /* I/O  Right input signal, becomes side signal     */
    const SKP_int32     pred_Q13[],                     /* I    Predictors                                  */
    SKP_int             fs_kHz,                         /* I    Samples rate (kHz)                          */
    SKP_int             frame_length                    /* I    Number of samples                           */
);

/* Find least-squares prediction gain for one signal based on another and quantize it */
SKP_int32 silk_stereo_find_predictor(                   /* O    Returns predictor in Q13                    */
    SKP_int32           *ratio_Q14,                     /* O    Ratio of residual and mid energies          */
    const SKP_int16     x[],                            /* I    Basis signal                                */
    const SKP_int16     y[],                            /* I    Target signal                               */
    SKP_int32           mid_res_amp_Q0[],               /* I/O  Smoothed mid, residual norms                */
    SKP_int             length,                         /* I    Number of samples                           */
    SKP_int             smooth_coef_Q16                 /* I    Smoothing coefficient                       */
);

/* Quantize mid/side predictors */
void silk_stereo_quant_pred(
    stereo_enc_state    *state,                         /* I/O  State                                       */
    SKP_int32           pred_Q13[],                     /* I/O  Predictors (out: quantized)                 */
    SKP_int8            ix[ 2 ][ 4 ]                    /* O    Quantization indices                        */
);

/* Entropy code the mid/side quantization indices */
void silk_stereo_encode_pred(
    ec_enc              *psRangeEnc,                    /* I/O  Compressor data structure                   */
    SKP_int8            ix[ 2 ][ 4 ]                    /* I    Quantization indices                        */
);

/* Decode mid/side predictors */
void silk_stereo_decode_pred(
    ec_dec              *psRangeDec,                    /* I/O  Compressor data structure                   */
    SKP_int             *decode_only_mid,               /* O    Flag that only mid channel has been coded   */
    SKP_int32           pred_Q13[]                      /* O    Predictors                                  */
);

/* Encodes signs of excitation */
void silk_encode_signs(
    ec_enc                      *psRangeEnc,                        /* I/O  Compressor data structure                   */
    const SKP_int8              pulses[],                           /* I    pulse signal                                */
    SKP_int                     length,                             /* I    length of input                             */
    const SKP_int               signalType,                         /* I    Signal type                                 */
    const SKP_int               quantOffsetType,                    /* I    Quantization offset type                    */
    const SKP_int               sum_pulses[ MAX_NB_SHELL_BLOCKS ]   /* I    Sum of absolute pulses per block            */
);

/* Decodes signs of excitation */
void silk_decode_signs(
    ec_dec                      *psRangeDec,                        /* I/O  Compressor data structure                   */
    SKP_int                     pulses[],                           /* I/O  pulse signal                                */
    SKP_int                     length,                             /* I    length of input                             */
    const SKP_int               signalType,                         /* I    Signal type                                 */
    const SKP_int               quantOffsetType,                    /* I    Quantization offset type                    */
    const SKP_int               sum_pulses[ MAX_NB_SHELL_BLOCKS ]   /* I    Sum of absolute pulses per block            */
);

/* Check encoder control struct */
SKP_int check_control_input( 
    silk_EncControlStruct        *encControl     /* I:   Control structure                               */
);

/* Control internal sampling rate */
SKP_int silk_control_audio_bandwidth(
    silk_encoder_state          *psEncC             /* I/O  Pointer to Silk encoder state               */
);

/* Control SNR of redidual quantizer */
SKP_int silk_control_SNR(
    silk_encoder_state          *psEncC,            /* I/O  Pointer to Silk encoder state               */
    SKP_int32                   TargetRate_bps      /* I    Target max bitrate (bps)                    */
);

/***************/
/* Shell coder */
/***************/

/* Encode quantization indices of excitation */
void silk_encode_pulses(
    ec_enc                      *psRangeEnc,        /* I/O  compressor data structure                   */
    const SKP_int               signalType,         /* I    Signal type                                 */
    const SKP_int               quantOffsetType,    /* I    quantOffsetType                             */
    SKP_int8                    pulses[],           /* I    quantization indices                        */
    const SKP_int               frame_length        /* I    Frame length                                */
);

/* Shell encoder, operates on one shell code frame of 16 pulses */
void silk_shell_encoder(
    ec_enc                      *psRangeEnc,        /* I/O  compressor data structure                   */
    const SKP_int               *pulses0            /* I    data: nonnegative pulse amplitudes          */
);

/* Shell decoder, operates on one shell code frame of 16 pulses */
void silk_shell_decoder(
    SKP_int                     *pulses0,           /* O    data: nonnegative pulse amplitudes          */
    ec_dec                      *psRangeDec,        /* I/O  Compressor data structure                   */
    const SKP_int               pulses4             /* I    number of pulses per pulse-subframe         */
);

/* Gain scalar quantization with hysteresis, uniform on log scale */
void silk_gains_quant(
    SKP_int8                        ind[ MAX_NB_SUBFR ],        /* O    gain indices                            */
    SKP_int32                       gain_Q16[ MAX_NB_SUBFR ],   /* I/O  gains (quantized out)                   */
    SKP_int8                        *prev_ind,                  /* I/O  last index in previous frame            */
    const SKP_int                   conditional,                /* I    first gain is delta coded if 1          */
    const SKP_int                   nb_subfr                    /* I    number of subframes                     */
);

/* Gains scalar dequantization, uniform on log scale */
void silk_gains_dequant(
    SKP_int32                       gain_Q16[ MAX_NB_SUBFR ],   /* O    quantized gains                         */
    const SKP_int8                  ind[ MAX_NB_SUBFR ],        /* I    gain indices                            */
    SKP_int8                        *prev_ind,                  /* I/O  last index in previous frame            */
    const SKP_int                   conditional,                /* I    first gain is delta coded if 1          */
    const SKP_int                   nb_subfr                    /* I    number of subframes                     */
);

/* Convert NLSF parameters to stable AR prediction filter coefficients */
void silk_NLSF2A_stable(
    SKP_int16                       pAR_Q12[ MAX_LPC_ORDER ],   /* O    Stabilized AR coefs [LPC_order]     */ 
    const SKP_int16                 pNLSF[ MAX_LPC_ORDER ],     /* I    NLSF vector         [LPC_order]     */
    const SKP_int                   LPC_order                   /* I    LPC/LSF order                       */
);

/* Interpolate two vectors */
void silk_interpolate(
    SKP_int16                       xi[ MAX_LPC_ORDER ],    /* O    interpolated vector                     */
    const SKP_int16                 x0[ MAX_LPC_ORDER ],    /* I    first vector                            */
    const SKP_int16                 x1[ MAX_LPC_ORDER ],    /* I    second vector                           */
    const SKP_int                   ifact_Q2,               /* I    interp. factor, weight on 2nd vector    */
    const SKP_int                   d                       /* I    number of parameters                    */
);

/* LTP tap quantizer */
void silk_quant_LTP_gains(
    SKP_int16           B_Q14[ MAX_NB_SUBFR * LTP_ORDER ],              /* I/O  (un)quantized LTP gains     */
    SKP_int8            cbk_index[ MAX_NB_SUBFR ],                      /* O    Codebook Index              */
    SKP_int8            *periodicity_index,                             /* O    Periodicity Index           */
    const SKP_int32     W_Q18[ MAX_NB_SUBFR*LTP_ORDER*LTP_ORDER ],      /* I    Error Weights in Q18        */
    SKP_int             mu_Q9,                                          /* I    Mu value (R/D tradeoff)     */
    SKP_int             lowComplexity,                                  /* I    Flag for low complexity     */
    const SKP_int       nb_subfr                                        /* I    number of subframes         */
);

/* Entropy constrained matrix-weighted VQ, for a single input data vector */
void silk_VQ_WMat_EC(
    SKP_int8                        *ind,               /* O    index of best codebook vector               */
    SKP_int32                       *rate_dist_Q14,     /* O    best weighted quantization error + mu * rate*/
    const SKP_int16                 *in_Q14,            /* I    input vector to be quantized                */
    const SKP_int32                 *W_Q18,             /* I    weighting matrix                            */
    const SKP_int8                  *cb_Q7,             /* I    codebook                                    */
    const SKP_uint8                 *cl_Q5,             /* I    code length for each codebook vector        */
    const SKP_int                   mu_Q9,              /* I    tradeoff between weighted error and rate    */
    SKP_int                         L                   /* I    number of vectors in codebook               */
);

/***********************************/
/* Noise shaping quantization (NSQ)*/
/***********************************/
void silk_NSQ(
    const silk_encoder_state        *psEncC,                                    /* I/O  Encoder State                       */
    silk_nsq_state                  *NSQ,                                       /* I/O  NSQ state                           */
    SideInfoIndices                 *psIndices,                                 /* I/O  Quantization Indices                */
    const SKP_int16                 x[],                                        /* I    prefiltered input signal            */
    SKP_int8                        pulses[],                                   /* O    quantized qulse signal              */
    const SKP_int16                 PredCoef_Q12[ 2 * MAX_LPC_ORDER ],          /* I    Short term prediction coefficients  */
    const SKP_int16                 LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],    /* I    Long term prediction coefficients   */
    const SKP_int16                 AR2_Q13[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ], /* I                                     */
    const SKP_int                   HarmShapeGain_Q14[ MAX_NB_SUBFR ],          /* I                                        */
    const SKP_int                   Tilt_Q14[ MAX_NB_SUBFR ],                   /* I    Spectral tilt                       */
    const SKP_int32                 LF_shp_Q14[ MAX_NB_SUBFR ],                 /* I                                        */
    const SKP_int32                 Gains_Q16[ MAX_NB_SUBFR ],                  /* I                                        */
    const SKP_int                   pitchL[ MAX_NB_SUBFR ],                     /* I                                        */
    const SKP_int                   Lambda_Q10,                                 /* I                                        */
    const SKP_int                   LTP_scale_Q14                               /* I    LTP state scaling                   */
);

/* Noise shaping using delayed decision */
void silk_NSQ_del_dec(
    const silk_encoder_state        *psEncC,                                    /* I/O  Encoder State                       */
    silk_nsq_state                  *NSQ,                                       /* I/O  NSQ state                           */
    SideInfoIndices                 *psIndices,                                 /* I/O  Quantization Indices                */
    const SKP_int16                 x[],                                        /* I    Prefiltered input signal            */
    SKP_int8                        pulses[],                                   /* O    Quantized pulse signal              */
    const SKP_int16                 PredCoef_Q12[ 2 * MAX_LPC_ORDER ],          /* I    Prediction coefs                    */
    const SKP_int16                 LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],    /* I    LT prediction coefs                 */
    const SKP_int16                 AR2_Q13[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ], /* I                                     */
    const SKP_int                   HarmShapeGain_Q14[ MAX_NB_SUBFR ],          /* I                                        */
    const SKP_int                   Tilt_Q14[ MAX_NB_SUBFR ],                   /* I    Spectral tilt                       */
    const SKP_int32                 LF_shp_Q14[ MAX_NB_SUBFR ],                 /* I                                        */
    const SKP_int32                 Gains_Q16[ MAX_NB_SUBFR ],                  /* I                                        */
    const SKP_int                   pitchL[ MAX_NB_SUBFR ],                     /* I                                        */
    const SKP_int                   Lambda_Q10,                                 /* I                                        */
    const SKP_int                   LTP_scale_Q14                               /* I    LTP state scaling                   */
);

/************/
/* Silk VAD */
/************/
/* Initialize the Silk VAD */
SKP_int silk_VAD_Init(                              /* O    Return value, 0 if success                  */ 
    silk_VAD_state              *psSilk_VAD         /* I/O  Pointer to Silk VAD state                   */ 
); 

/* Silk VAD noise level estimation */
void silk_VAD_GetNoiseLevels(
    const SKP_int32             pX[ VAD_N_BANDS ],  /* I    subband energies                            */
    silk_VAD_state              *psSilk_VAD         /* I/O  Pointer to Silk VAD state                   */ 
);

/* Get speech activity level in Q8 */
SKP_int silk_VAD_GetSA_Q8(                          /* O    Return value, 0 if success                  */
    silk_encoder_state          *psEncC,            /* I/O  Encoder state                               */
    const SKP_int16             pIn[]               /* I    PCM input                                   */
);

/* Low-pass filter with variable cutoff frequency based on  */
/* piece-wise linear interpolation between elliptic filters */
/* Start by setting transition_frame_no = 1;                */
void silk_LP_variable_cutoff(
    silk_LP_state               *psLP,              /* I/O  LP filter state                             */
    SKP_int16                   *signal,            /* I/O  Low-pass filtered output signal             */
    const SKP_int               frame_length        /* I    Frame length                                */
);

/******************/
/* NLSF Quantizer */
/******************/
/* Limit, stabilize, convert and quantize NLSFs */ 
void silk_process_NLSFs(
    silk_encoder_state              *psEncC,                                /* I/O  Encoder state                               */
    SKP_int16                       PredCoef_Q12[ 2 ][ MAX_LPC_ORDER ],     /* O    Prediction coefficients                     */
    SKP_int16                       pNLSF_Q15[         MAX_LPC_ORDER ],     /* I/O  Normalized LSFs (quant out) (0 - (2^15-1))  */
    const SKP_int16                 prev_NLSFq_Q15[    MAX_LPC_ORDER ]      /* I    Previous Normalized LSFs (0 - (2^15-1))     */
);

SKP_int32 silk_NLSF_encode(                                 /* O    Returns RD value in Q25                 */
          SKP_int8                  *NLSFIndices,           /* I    Codebook path vector [ LPC_ORDER + 1 ]  */
          SKP_int16                 *pNLSF_Q15,             /* I/O  Quantized NLSF vector [ LPC_ORDER ]     */
    const silk_NLSF_CB_struct       *psNLSF_CB,             /* I    Codebook object                         */
    const SKP_int16                 *pW_Q5,                 /* I    NLSF weight vector [ LPC_ORDER ]        */
    const SKP_int                   NLSF_mu_Q20,            /* I    Rate weight for the RD optimization     */
    const SKP_int                   nSurvivors,             /* I    Max survivors after first stage         */
    const SKP_int                   signalType              /* I    Signal type: 0/1/2                      */
);

/* Compute quantization errors for an LPC_order element input vector for a VQ codebook */
void silk_NLSF_VQ(
    SKP_int32                   err_Q26[],              /* O    Quantization errors [K]                     */
    const SKP_int16             in_Q15[],               /* I    Input vectors to be quantized [LPC_order]   */
    const SKP_uint8             pCB_Q8[],               /* I    Codebook vectors [K*LPC_order]              */
    const SKP_int               K,                      /* I    Number of codebook vectors                  */
    const SKP_int               LPC_order               /* I    Number of LPCs                              */
);

/* Delayed-decision quantizer for NLSF residuals */
SKP_int32 silk_NLSF_del_dec_quant(                      /* O    Returns RD value in Q25                     */
    SKP_int8                    indices[],              /* O    Quantization indices [ order ]              */
    const SKP_int16             x_Q10[],                /* I    Input [ order ]                             */
    const SKP_int16             w_Q5[],                 /* I    Weights [ order ]                           */
    const SKP_uint8             pred_coef_Q8[],         /* I    Backward predictor coefs [ order ]          */
    const SKP_int16             ec_ix[],                /* I    Indices to entropy coding tables [ order ]  */
    const SKP_uint8             ec_rates_Q5[],          /* I    Rates []                                    */
    const SKP_int               quant_step_size_Q16,    /* I    Quantization step size                      */
    const SKP_int16             inv_quant_step_size_Q6, /* I    Inverse quantization step size              */
    const SKP_int32             mu_Q20,                 /* I    R/D tradeoff                                */
    const SKP_int16             order                   /* I    Number of input values                      */
);

/* Unpack predictor values and indices for entropy coding tables */
void silk_NLSF_unpack(
          SKP_int16                 ec_ix[],                /* O    Indices to entropy tales [ LPC_ORDER ]  */
          SKP_uint8                 pred_Q8[],              /* O    LSF predictor [ LPC_ORDER ]             */
    const silk_NLSF_CB_struct       *psNLSF_CB,             /* I    Codebook object                         */
    const SKP_int                   CB1_index               /* I    Index of vector in first LSF codebook   */
);

/***********************/
/* NLSF vector decoder */
/***********************/
void silk_NLSF_decode(
          SKP_int16                 *pNLSF_Q15,             /* O    Quantized NLSF vector [ LPC_ORDER ]     */
          SKP_int8                  *NLSFIndices,           /* I    Codebook path vector [ LPC_ORDER + 1 ]  */
    const silk_NLSF_CB_struct       *psNLSF_CB              /* I    Codebook object                         */
);

/****************************************************/
/* Decoder Functions                                */
/****************************************************/
SKP_int silk_create_decoder(
    silk_decoder_state              **ppsDec            /* I/O  Decoder state pointer pointer               */
);

SKP_int silk_free_decoder(
    silk_decoder_state              *psDec              /* I/O  Decoder state pointer                       */
);

SKP_int silk_init_decoder(
    silk_decoder_state              *psDec              /* I/O  Decoder state pointer                       */
);

/* Set decoder sampling rate */
void silk_decoder_set_fs(
    silk_decoder_state              *psDec,             /* I/O  Decoder state pointer                       */
    SKP_int                         fs_kHz              /* I    Sampling frequency (kHz)                    */
);

/****************/
/* Decode frame */
/****************/
SKP_int silk_decode_frame(
    silk_decoder_state          *psDec,             /* I/O  Pointer to Silk decoder state               */
    ec_dec                      *psRangeDec,        /* I/O  Compressor data structure                   */
    SKP_int16                   pOut[],             /* O    Pointer to output speech frame              */
    SKP_int32                   *pN,                /* O    Pointer to size of output frame             */
    SKP_int                     lostFlag            /* I    0: no loss, 1 loss, 2 decode fec            */
);

/* Decode LBRR side info and excitation */
void silk_LBRR_extract(
    silk_decoder_state          *psDec,             /* I/O  State                                       */
    ec_dec                      *psRangeDec         /* I/O  Compressor data structure                   */
);

/* Decode indices from payload v4 Bitstream */
void silk_decode_indices(
    silk_decoder_state          *psDec,             /* I/O  State                                       */
    ec_dec                      *psRangeDec,        /* I/O  Compressor data structure                   */
    SKP_int                     FrameIndex,         /* I    Frame number                                */
    SKP_int                     decode_LBRR         /* I    Flag indicating LBRR data is being decoded  */
);

/* Decode parameters from payload */
void silk_decode_parameters(
    silk_decoder_state      *psDec,                             /* I/O  State                                    */
    silk_decoder_control    *psDecCtrl                          /* I/O  Decoder control                          */
);

/* Core decoder. Performs inverse NSQ operation LTP + LPC */
void silk_decode_core(
    silk_decoder_state          *psDec,                             /* I/O  Decoder state               */
    silk_decoder_control        *psDecCtrl,                         /* I    Decoder control             */
    SKP_int16                   xq[],                               /* O    Decoded speech              */
    const SKP_int               pulses[ MAX_FRAME_LENGTH ]          /* I    Pulse signal                */
);

/* Decode quantization indices of excitation (Shell coding) */
void silk_decode_pulses(
    ec_dec                          *psRangeDec,        /* I/O  Compressor data structure                   */
    SKP_int                         pulses[],           /* O    Excitation signal                           */
    const SKP_int                   signalType,         /* I    Sigtype                                     */
    const SKP_int                   quantOffsetType,    /* I    quantOffsetType                             */
    const SKP_int                   frame_length        /* I    Frame length                                */
);

/******************/
/* CNG */
/******************/

/* Reset CNG */
void silk_CNG_Reset(
    silk_decoder_state          *psDec              /* I/O  Decoder state                               */
);

/* Updates CNG estimate, and applies the CNG when packet was lost */
void silk_CNG(
    silk_decoder_state          *psDec,             /* I/O  Decoder state                               */
    silk_decoder_control        *psDecCtrl,         /* I/O  Decoder control                             */
    SKP_int16                   signal[],           /* I/O  Signal                                      */
    SKP_int                     length              /* I    Length of residual                          */
);

/* Encoding of various parameters */
void silk_encode_indices(
    silk_encoder_state          *psEncC,            /* I/O  Encoder state                               */
    ec_enc                      *psRangeEnc,        /* I/O  Compressor data structure                   */
    SKP_int                     FrameIndex,         /* I    Frame number                                */
    SKP_int                     encode_LBRR         /* I    Flag indicating LBRR data is being encoded  */
);

#ifdef __cplusplus
}
#endif

#endif
