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

#ifndef _SILK_SIGPROC_FIX_H_
#define _SILK_SIGPROC_FIX_H_

#ifdef  __cplusplus
extern "C"
{
#endif

//#define SKP_MACRO_COUNT           /* Used to enable WMOPS counting */

#define SILK_MAX_ORDER_LPC            16                        /* max order of the LPC analysis in schur() and k2a()    */

#include "silk_typedef.h"
#include <stdlib.h>                                             /* for abs() */
#include "silk_resampler_structs.h"
#include "silk_macros.h"


/********************************************************************/
/*                    SIGNAL PROCESSING FUNCTIONS                   */
/********************************************************************/

/*!
 * Initialize/reset the resampler state for a given pair of input/output sampling rates 
*/
SKP_int silk_resampler_init( 
	silk_resampler_state_struct	        *S,         /* I/O: Resampler state 			*/
	SKP_int32							Fs_Hz_in,	/* I:	Input sampling rate (Hz)	*/
	SKP_int32							Fs_Hz_out	/* I:	Output sampling rate (Hz)	*/
);

/*!
 * Clear the states of all resampling filters, without resetting sampling rate ratio 
 */
SKP_int silk_resampler_clear( 
	silk_resampler_state_struct	        *S          /* I/O: Resampler state 			*/
);

/*!
 * Resampler: convert from one sampling rate to another
 */
SKP_int silk_resampler( 
	silk_resampler_state_struct	        *S,         /* I/O: Resampler state 			*/
	SKP_int16							out[],	    /* O:	Output signal 				*/
	const SKP_int16						in[],	    /* I:	Input signal				*/
	SKP_int32							inLen	    /* I:	Number of input samples		*/
);

/*!
 Upsample 2x, low quality 
 */
void silk_resampler_up2(
    SKP_int32                           *S,         /* I/O: State vector [ 2 ]                  */
    SKP_int16                           *out,       /* O:   Output signal [ 2 * len ]           */
    const SKP_int16                     *in,        /* I:   Input signal [ len ]                */
    SKP_int32                           len         /* I:   Number of input samples             */
);

/*!
* Downsample 2x, mediocre quality 
*/
void silk_resampler_down2(
    SKP_int32                           *S,         /* I/O: State vector [ 2 ]                  */
    SKP_int16                           *out,       /* O:   Output signal [ len ]               */
    const SKP_int16                     *in,        /* I:   Input signal [ floor(len/2) ]       */
    SKP_int32                           inLen       /* I:   Number of input samples             */
);


/*!
 * Downsample by a factor 2/3, low quality
*/
void silk_resampler_down2_3(
    SKP_int32                           *S,         /* I/O: State vector [ 6 ]                  */
    SKP_int16                           *out,       /* O:   Output signal [ floor(2*inLen/3) ]  */
    const SKP_int16                     *in,        /* I:   Input signal [ inLen ]              */
    SKP_int32                           inLen       /* I:   Number of input samples             */
);

/*!
 * Downsample by a factor 3, low quality
*/
void silk_resampler_down3(
    SKP_int32                           *S,         /* I/O: State vector [ 8 ]                  */
    SKP_int16                           *out,       /* O:   Output signal [ floor(inLen/3) ]    */
    const SKP_int16                     *in,        /* I:   Input signal [ inLen ]              */
    SKP_int32                           inLen       /* I:   Number of input samples             */
);

/*!
 * second order ARMA filter; 
 * slower than biquad() but uses more precise coefficients
 * can handle (slowly) varying coefficients 
 */
void silk_biquad_alt(
    const SKP_int16     *in,           /* I:    input signal                 */
    const SKP_int32     *B_Q28,        /* I:    MA coefficients [3]          */
    const SKP_int32     *A_Q28,        /* I:    AR coefficients [2]          */
    SKP_int32           *S,            /* I/O:  State vector [2]             */
    SKP_int16           *out,          /* O:    output signal                */
    const SKP_int32     len            /* I:    signal length (must be even) */
);

/*! 
 * variable order MA filter. Prediction error filter implementation. Coeficients negated and starting with coef to x[n - 1]
 */
void silk_MA_Prediction(
    const SKP_int16      *in,          /* I:   Input signal                                */
    const SKP_int16      *B,           /* I:   MA prediction coefficients, Q12 [order]     */
    SKP_int32            *S,           /* I/O: State vector [order]                        */
    SKP_int16            *out,         /* O:   Output signal                               */
    const SKP_int32      len,          /* I:   Signal length                               */
    const SKP_int32      order         /* I:   Filter order                                */
);

/*!
 * 16th order AR filter for LPC synthesis, coefficients are in Q12
 */
void silk_LPC_synthesis_order16(
    const SKP_int16      *in,          /* I:   excitation signal                            */
    const SKP_int16      *A_Q12,       /* I:   AR coefficients [16], between -8_Q0 and 8_Q0 */
    const SKP_int32      Gain_Q26,     /* I:   gain                                         */
          SKP_int32      *S,           /* I/O: state vector [16]                            */
          SKP_int16      *out,         /* O:   output signal                                */
    const SKP_int32      len           /* I:   signal length, must be multiple of 16        */
);

/* variable order MA prediction error filter. */
/* Inverse filter of silk_LPC_synthesis_filter */
void silk_LPC_analysis_filter(
    SKP_int16            *out,         /* O:   Output signal                               */
    const SKP_int16      *in,          /* I:   Input signal                                */
    const SKP_int16      *B,           /* I:   MA prediction coefficients, Q12 [order]     */
    const SKP_int32      len,          /* I:   Signal length                               */
    const SKP_int32      Order         /* I:   Filter order                                */
);

/* even order AR filter */
void silk_LPC_synthesis_filter(
    const SKP_int16      *in,          /* I:   excitation signal                               */
    const SKP_int16      *A_Q12,       /* I:   AR coefficients [Order], between -8_Q0 and 8_Q0 */
    const SKP_int32      Gain_Q26,     /* I:   gain                                            */
    SKP_int32            *S,           /* I/O: state vector [Order]                            */
    SKP_int16            *out,         /* O:   output signal                                   */
    const SKP_int32      len,          /* I:   signal length                                   */
    const SKP_int        Order         /* I:   filter order, must be even                      */
);

/* Chirp (bandwidth expand) LP AR filter */
void silk_bwexpander( 
    SKP_int16            *ar,          /* I/O  AR filter to be expanded (without leading 1)    */
    const SKP_int        d,            /* I    Length of ar                                    */
    SKP_int32            chirp_Q16     /* I    Chirp factor (typically in the range 0 to 1)    */
);

/* Chirp (bandwidth expand) LP AR filter */
void silk_bwexpander_32( 
    SKP_int32            *ar,          /* I/O  AR filter to be expanded (without leading 1)    */
    const SKP_int        d,            /* I    Length of ar                                    */
    SKP_int32            chirp_Q16     /* I    Chirp factor in Q16                             */
);

/* Compute inverse of LPC prediction gain, and                           */
/* test if LPC coefficients are stable (all poles within unit circle)    */
SKP_int silk_LPC_inverse_pred_gain(     /* O:  Returns 1 if unstable, otherwise 0          */
    SKP_int32            *invGain_Q30,  /* O:  Inverse prediction gain, Q30 energy domain  */
    const SKP_int16      *A_Q12,        /* I:  Prediction coefficients, Q12 [order]        */
    const SKP_int        order          /* I:  Prediction order                            */
);

SKP_int silk_LPC_inverse_pred_gain_Q24( /* O:   Returns 1 if unstable, otherwise 0      */
    SKP_int32           *invGain_Q30,   /* O:   Inverse prediction gain, Q30 energy domain  */
    const SKP_int32     *A_Q24,         /* I:   Prediction coefficients, Q24 [order]        */
    const SKP_int       order           /* I:   Prediction order                            */
);

/* split signal in two decimated bands using first-order allpass filters */
void silk_ana_filt_bank_1(
    const SKP_int16      *in,           /* I:   Input signal [N]        */
    SKP_int32            *S,            /* I/O: State vector [2]        */
    SKP_int16            *outL,         /* O:   Low band [N/2]          */
    SKP_int16            *outH,         /* O:   High band [N/2]         */
    const SKP_int32      N              /* I:   Number of input samples */
);

/********************************************************************/
/*                        SCALAR FUNCTIONS                            */
/********************************************************************/

/* approximation of 128 * log2() (exact inverse of approx 2^() below) */
/* convert input to a log scale    */
SKP_int32 silk_lin2log(const SKP_int32 inLin);        /* I: input in linear scale        */

/* Approximation of a sigmoid function */
SKP_int silk_sigm_Q15(SKP_int in_Q5);

/* approximation of 2^() (exact inverse of approx log2() above) */
/* convert input to a linear scale    */ 
SKP_int32 silk_log2lin(const SKP_int32 inLog_Q7);    /* I: input on log scale */ 

/* Function that returns the maximum absolut value of the input vector */
SKP_int16 silk_int16_array_maxabs(      /* O   Maximum absolute value, max: 2^15-1   */
    const SKP_int16     *vec,           /* I   Input vector  [len]                   */ 
    const SKP_int32     len             /* I   Length of input vector                */
);

/* Compute number of bits to right shift the sum of squares of a vector    */
/* of int16s to make it fit in an int32                                    */
void silk_sum_sqr_shift(
    SKP_int32           *energy,        /* O   Energy of x, after shifting to the right            */
    SKP_int             *shift,         /* O   Number of bits right shift applied to energy        */
    const SKP_int16     *x,             /* I   Input vector                                        */
    SKP_int             len             /* I   Length of input vector                              */
);

/* Calculates the reflection coefficients from the correlation sequence    */
/* Faster than schur64(), but much less accurate.                          */
/* uses SMLAWB(), requiring armv5E and higher.                             */ 
SKP_int32 silk_schur(                   /* O:    Returns residual energy                   */
    SKP_int16           *rc_Q15,        /* O:    reflection coefficients [order] Q15       */
    const SKP_int32     *c,             /* I:    correlations [order+1]                    */
    const SKP_int32     order           /* I:    prediction order                          */
);;

/* Calculates the reflection coefficients from the correlation sequence    */
/* Slower than schur(), but more accurate.                                 */
/* Uses SMULL(), available on armv4                                        */
SKP_int32 silk_schur64(                 /* O:  returns residual energy                     */
    SKP_int32           rc_Q16[],       /* O:  Reflection coefficients [order] Q16         */
    const SKP_int32     c[],            /* I:  Correlations [order+1]                      */
    SKP_int32           order           /* I:  Prediction order                            */
);

/* Step up function, converts reflection coefficients to prediction coefficients */
void silk_k2a(
    SKP_int32           *A_Q24,         /* O:  Prediction coefficients [order] Q24         */
    const SKP_int16     *rc_Q15,        /* I:  Reflection coefficients [order] Q15         */
    const SKP_int32     order           /* I:  Prediction order                            */
);

/* Step up function, converts reflection coefficients to prediction coefficients */
void silk_k2a_Q16(
    SKP_int32           *A_Q24,         /* O:  Prediction coefficients [order] Q24         */
    const SKP_int32     *rc_Q16,        /* I:  Reflection coefficients [order] Q16         */
    const SKP_int32     order           /* I:  Prediction order                            */
);

/* Apply sine window to signal vector.                                      */
/* Window types:                                                            */
/*    1 -> sine window from 0 to pi/2                                       */
/*    2 -> sine window from pi/2 to pi                                      */
/* every other sample of window is linearly interpolated, for speed         */
void silk_apply_sine_window(
    SKP_int16           px_win[],       /* O  Pointer to windowed signal                  */
    const SKP_int16     px[],           /* I  Pointer to input signal                     */
    const SKP_int       win_type,       /* I  Selects a window type                       */
    const SKP_int       length          /* I  Window length, multiple of 4                */
);

/* Compute autocorrelation */
void silk_autocorr( 
    SKP_int32           *results,       /* O  Result (length correlationCount)            */
    SKP_int             *scale,         /* O  Scaling of the correlation vector           */
    const SKP_int16     *inputData,     /* I  Input data to correlate                     */
    const SKP_int       inputDataSize,  /* I  Length of input                             */
    const SKP_int       correlationCount /* I  Number of correlation taps to compute      */
);

/* Pitch estimator */
#define SILK_PE_MIN_COMPLEX        0
#define SILK_PE_MID_COMPLEX        1
#define SILK_PE_MAX_COMPLEX        2

void silk_decode_pitch(
    SKP_int16       lagIndex,                        /* I                             */
    SKP_int8        contourIndex,                    /* O                             */
    SKP_int         pitch_lags[],                    /* O 4 pitch values              */
    const SKP_int   Fs_kHz,                          /* I sampling frequency (kHz)    */
    const SKP_int   nb_subfr                         /* I number of sub frames        */
);

SKP_int silk_pitch_analysis_core(        /* O    Voicing estimate: 0 voiced, 1 unvoiced                     */
    const SKP_int16  *signal,            /* I    Signal of length PE_FRAME_LENGTH_MS*Fs_kHz                 */
    SKP_int          *pitch_out,         /* O    4 pitch lag values                                         */
    SKP_int16        *lagIndex,          /* O    Lag Index                                                  */
    SKP_int8         *contourIndex,      /* O    Pitch contour Index                                        */
    SKP_int          *LTPCorr_Q15,       /* I/O  Normalized correlation; input: value from previous frame   */
    SKP_int          prevLag,            /* I    Last lag of previous frame; set to zero is unvoiced        */
    const SKP_int32  search_thres1_Q16,  /* I    First stage threshold for lag candidates 0 - 1             */
    const SKP_int    search_thres2_Q15,  /* I    Final threshold for lag candidates 0 - 1                   */
    const SKP_int    Fs_kHz,             /* I    Sample frequency (kHz)                                     */
    const SKP_int    complexity,         /* I    Complexity setting, 0-2, where 2 is highest                */
    const SKP_int    nb_subfr            /* I    number of 5 ms subframes                                   */
);

void silk_LPC_fit(
          SKP_int16    *a_QQ,            /* O    stabilized LPC vector, Q(24-rshift) [L]        */
          SKP_int32    *a_Q24,           /* I    LPC vector [L]                                 */
    const SKP_int      QQ,               /* I    Q domain of output LPC vector                  */
    const SKP_int      L                 /* I    Number of LPC parameters in the input vector   */
);

/* Compute Normalized Line Spectral Frequencies (NLSFs) from whitening filter coefficients      */
/* If not all roots are found, the a_Q16 coefficients are bandwidth expanded until convergence. */
void silk_A2NLSF(
    SKP_int16          *NLSF,            /* O    Normalized Line Spectral Frequencies, Q15 (0 - (2^15-1)), [d] */
    SKP_int32          *a_Q16,           /* I/O  Monic whitening filter coefficients in Q16 [d]                */
    const SKP_int      d                 /* I    Filter order (must be even)                                   */
);

/* compute whitening filter coefficients from normalized line spectral frequencies */
void silk_NLSF2A(
    SKP_int16          *a,               /* o    monic whitening filter coefficients in Q12,  [d]    */
    const SKP_int16    *NLSF,            /* i    normalized line spectral frequencies in Q15, [d]    */
    const SKP_int      d                 /* i    filter order (should be even)                       */
);

void silk_insertion_sort_increasing(
    SKP_int32            *a,            /* I/O   Unsorted / Sorted vector                */
    SKP_int              *idx,          /* O:    Index vector for the sorted elements    */
    const SKP_int        L,             /* I:    Vector length                           */
    const SKP_int        K              /* I:    Number of correctly sorted positions    */
);

void silk_insertion_sort_decreasing_int16(
    SKP_int16            *a,            /* I/O:  Unsorted / Sorted vector                */
    SKP_int              *idx,          /* O:    Index vector for the sorted elements    */
    const SKP_int        L,             /* I:    Vector length                           */
    const SKP_int        K              /* I:    Number of correctly sorted positions    */
);

void silk_insertion_sort_increasing_all_values_int16(
     SKP_int16           *a,            /* I/O:  Unsorted / Sorted vector                */
     const SKP_int       L              /* I:    Vector length                           */
);

/* NLSF stabilizer, for a single input data vector */
void silk_NLSF_stabilize(
          SKP_int16      *NLSF_Q15,      /* I/O:  Unstable/stabilized normalized LSF vector in Q15 [L]                    */
    const SKP_int16      *NDeltaMin_Q15, /* I:    Normalized delta min vector in Q15, NDeltaMin_Q15[L] must be >= 1 [L+1] */
    const SKP_int        L               /* I:    Number of NLSF parameters in the input vector                           */
);

/* Laroia low complexity NLSF weights */
void silk_NLSF_VQ_weights_laroia(
    SKP_int16            *pNLSFW_Q5,     /* O:    Pointer to input vector weights            [D x 1]       */
    const SKP_int16      *pNLSF_Q15,     /* I:    Pointer to input vector                    [D x 1]       */
    const SKP_int        D               /* I:    Input vector dimension (even)                            */
);

/* Compute reflection coefficients from input signal */
void silk_burg_modified(        
    SKP_int32            *res_nrg,           /* O   residual energy                                                 */
    SKP_int              *res_nrgQ,          /* O   residual energy Q value                                         */
    SKP_int32            A_Q16[],            /* O   prediction coefficients (length order)                          */
    const SKP_int16      x[],                /* I   input signal, length: nb_subfr * ( D + subfr_length )           */
    const SKP_int        subfr_length,       /* I   input signal subframe length (including D preceeding samples)   */
    const SKP_int        nb_subfr,           /* I   number of subframes stacked in x                                */
    const SKP_int32      WhiteNoiseFrac_Q32, /* I   fraction added to zero-lag autocorrelation                      */
    const SKP_int        D                   /* I   order                                                           */
);

/* Copy and multiply a vector by a constant */
void silk_scale_copy_vector16( 
    SKP_int16            *data_out, 
    const SKP_int16      *data_in, 
    SKP_int32            gain_Q16,           /* I:   gain in Q16   */
    const SKP_int        dataSize            /* I:   length        */
);

/* Some for the LTP related function requires Q26 to work.*/
void silk_scale_vector32_Q26_lshift_18( 
    SKP_int32            *data1,             /* I/O: Q0/Q18        */
    SKP_int32            gain_Q26,           /* I:   Q26           */
    SKP_int              dataSize            /* I:   length        */
);

/********************************************************************/
/*                        INLINE ARM MATH                             */
/********************************************************************/

/*    return sum(inVec1[i]*inVec2[i])    */
SKP_int32 silk_inner_prod_aligned(
    const SKP_int16 *const  inVec1,     /*    I input vector 1    */
    const SKP_int16 *const  inVec2,     /*    I input vector 2    */
    const SKP_int           len         /*    I vector lengths    */
);

SKP_int32 silk_inner_prod_aligned_scale(
    const SKP_int16 *const  inVec1,     /*    I input vector 1          */
    const SKP_int16 *const  inVec2,     /*    I input vector 2          */
    const SKP_int           scale,      /*    I number of bits to shift */
    const SKP_int           len         /*    I vector lengths          */
);

SKP_int64 silk_inner_prod16_aligned_64(
    const SKP_int16         *inVec1,    /*    I input vector 1    */ 
    const SKP_int16         *inVec2,    /*    I input vector 2    */
    const SKP_int           len         /*    I vector lengths    */
);

/********************************************************************/
/*                                MACROS                            */
/********************************************************************/

/* Rotate a32 right by 'rot' bits. Negative rot values result in rotating
   left. Output is 32bit int.
   Note: contemporary compilers recognize the C expression below and
   compile it into a 'ror' instruction if available. No need for inline ASM! */
SKP_INLINE SKP_int32 silk_ROR32( SKP_int32 a32, SKP_int rot )
{
    SKP_uint32 x = (SKP_uint32) a32;
    SKP_uint32 r = (SKP_uint32) rot;
    SKP_uint32 m = (SKP_uint32) -rot;
    if(rot <= 0)
        return (SKP_int32) ((x << m) | (x >> (32 - m)));
    else
        return (SKP_int32) ((x << (32 - r)) | (x >> r));
}

/* Allocate SKP_int16 alligned to 4-byte memory address */
#if EMBEDDED_ARM
#define SKP_DWORD_ALIGN __attribute__((aligned(4)))
#else
#define SKP_DWORD_ALIGN
#endif

/* Useful Macros that can be adjusted to other platforms */
#define SKP_memcpy(a, b, c)                memcpy((a), (b), (c))    /* Dest, Src, ByteCount */
#define SKP_memset(a, b, c)                memset((a), (b), (c))    /* Dest, value, ByteCount */
#define SKP_memmove(a, b, c)               memmove((a), (b), (c))   /* Dest, Src, ByteCount */
/* fixed point macros */

// (a32 * b32) output have to be 32bit int
#define SKP_MUL(a32, b32)                  ((a32) * (b32))

// (a32 * b32) output have to be 32bit uint
#define SKP_MUL_uint(a32, b32)             SKP_MUL(a32, b32)

// a32 + (b32 * c32) output have to be 32bit int
#define SKP_MLA(a32, b32, c32)             SKP_ADD32((a32),((b32) * (c32)))

// a32 + (b32 * c32) output have to be 32bit uint
#define SKP_MLA_uint(a32, b32, c32)        SKP_MLA(a32, b32, c32)

// ((a32 >> 16)  * (b32 >> 16)) output have to be 32bit int
#define SKP_SMULTT(a32, b32)               (((a32) >> 16) * ((b32) >> 16))

// a32 + ((a32 >> 16)  * (b32 >> 16)) output have to be 32bit int
#define SKP_SMLATT(a32, b32, c32)          SKP_ADD32((a32),((b32) >> 16) * ((c32) >> 16))

#define SKP_SMLALBB(a64, b16, c16)         SKP_ADD64((a64),(SKP_int64)((SKP_int32)(b16) * (SKP_int32)(c16)))

// (a32 * b32)
#define SKP_SMULL(a32, b32)                ((SKP_int64)(a32) * /*(SKP_int64)*/(b32))

// multiply-accumulate macros that allow overflow in the addition (ie, no asserts in debug mode)
#define SKP_MLA_ovflw(a32, b32, c32)       SKP_MLA(a32, b32, c32)
#ifndef SKP_SMLABB_ovflw
#    define SKP_SMLABB_ovflw(a32, b32, c32)    SKP_SMLABB(a32, b32, c32)
#endif
#define SKP_SMLABT_ovflw(a32, b32, c32)    SKP_SMLABT(a32, b32, c32)
#define SKP_SMLATT_ovflw(a32, b32, c32)    SKP_SMLATT(a32, b32, c32)
#define SKP_SMLAWB_ovflw(a32, b32, c32)    SKP_SMLAWB(a32, b32, c32)
#define SKP_SMLAWT_ovflw(a32, b32, c32)    SKP_SMLAWT(a32, b32, c32)

#define SKP_DIV32_16(a32, b16)             ((SKP_int32)((a32) / (b16)))
#define SKP_DIV32(a32, b32)                ((SKP_int32)((a32) / (b32)))

// These macros enables checking for overflow in silk_API_Debug.h
#define SKP_ADD16(a, b)                    ((a) + (b))
#define SKP_ADD32(a, b)                    ((a) + (b))
#define SKP_ADD64(a, b)                    ((a) + (b))

#define SKP_SUB16(a, b)                    ((a) - (b))
#define SKP_SUB32(a, b)                    ((a) - (b))
#define SKP_SUB64(a, b)                    ((a) - (b))

#define SKP_SAT8(a)                        ((a) > SKP_int8_MAX ? SKP_int8_MAX  : \
                                           ((a) < SKP_int8_MIN ? SKP_int8_MIN  : (a)))
#define SKP_SAT16(a)                       ((a) > SKP_int16_MAX ? SKP_int16_MAX : \
                                           ((a) < SKP_int16_MIN ? SKP_int16_MIN : (a)))
#define SKP_SAT32(a)                       ((a) > SKP_int32_MAX ? SKP_int32_MAX : \
                                           ((a) < SKP_int32_MIN ? SKP_int32_MIN : (a)))

#define SKP_CHECK_FIT8(a)                  (a)
#define SKP_CHECK_FIT16(a)                 (a)
#define SKP_CHECK_FIT32(a)                 (a)

#define SKP_ADD_SAT16(a, b)                (SKP_int16)SKP_SAT16( SKP_ADD32( (SKP_int32)(a), (b) ) )
#define SKP_ADD_SAT64(a, b)                ((((a) + (b)) & 0x8000000000000000LL) == 0 ?                            \
                                           ((((a) & (b)) & 0x8000000000000000LL) != 0 ? SKP_int64_MIN : (a)+(b)) :    \
                                           ((((a) | (b)) & 0x8000000000000000LL) == 0 ? SKP_int64_MAX : (a)+(b)) )

#define SKP_SUB_SAT16(a, b)                (SKP_int16)SKP_SAT16( SKP_SUB32( (SKP_int32)(a), (b) ) )
#define SKP_SUB_SAT64(a, b)                ((((a)-(b)) & 0x8000000000000000LL) == 0 ?                                                    \
                                           (( (a) & ((b)^0x8000000000000000LL) & 0x8000000000000000LL) ? SKP_int64_MIN : (a)-(b)) :    \
                                           ((((a)^0x8000000000000000LL) & (b)  & 0x8000000000000000LL) ? SKP_int64_MAX : (a)-(b)) )

/* Saturation for positive input values */ 
#define SKP_POS_SAT32(a)                   ((a) > SKP_int32_MAX ? SKP_int32_MAX : (a))

/* Add with saturation for positive input values */ 
#define SKP_ADD_POS_SAT8(a, b)             ((((a)+(b)) & 0x80)                 ? SKP_int8_MAX  : ((a)+(b)))
#define SKP_ADD_POS_SAT16(a, b)            ((((a)+(b)) & 0x8000)               ? SKP_int16_MAX : ((a)+(b)))
#define SKP_ADD_POS_SAT32(a, b)            ((((a)+(b)) & 0x80000000)           ? SKP_int32_MAX : ((a)+(b)))
#define SKP_ADD_POS_SAT64(a, b)            ((((a)+(b)) & 0x8000000000000000LL) ? SKP_int64_MAX : ((a)+(b)))

#define SKP_LSHIFT8(a, shift)              ((a)<<(shift))                // shift >= 0, shift < 8
#define SKP_LSHIFT16(a, shift)             ((a)<<(shift))                // shift >= 0, shift < 16
#define SKP_LSHIFT32(a, shift)             ((a)<<(shift))                // shift >= 0, shift < 32
#define SKP_LSHIFT64(a, shift)             ((a)<<(shift))                // shift >= 0, shift < 64
#define SKP_LSHIFT(a, shift)               SKP_LSHIFT32(a, shift)        // shift >= 0, shift < 32

#define SKP_RSHIFT8(a, shift)              ((a)>>(shift))                // shift >= 0, shift < 8
#define SKP_RSHIFT16(a, shift)             ((a)>>(shift))                // shift >= 0, shift < 16
#define SKP_RSHIFT32(a, shift)             ((a)>>(shift))                // shift >= 0, shift < 32
#define SKP_RSHIFT64(a, shift)             ((a)>>(shift))                // shift >= 0, shift < 64
#define SKP_RSHIFT(a, shift)               SKP_RSHIFT32(a, shift)        // shift >= 0, shift < 32

/* saturates before shifting */
#define SKP_LSHIFT_SAT16(a, shift)         (SKP_LSHIFT16( SKP_LIMIT( (a), SKP_RSHIFT16( SKP_int16_MIN, (shift) ),    \
                                                                          SKP_RSHIFT16( SKP_int16_MAX, (shift) ) ), (shift) ))
#define SKP_LSHIFT_SAT32(a, shift)         (SKP_LSHIFT32( SKP_LIMIT( (a), SKP_RSHIFT32( SKP_int32_MIN, (shift) ),    \
                                                                          SKP_RSHIFT32( SKP_int32_MAX, (shift) ) ), (shift) ))

#define SKP_LSHIFT_ovflw(a, shift)        ((a)<<(shift))        // shift >= 0, allowed to overflow
#define SKP_LSHIFT_uint(a, shift)         ((a)<<(shift))        // shift >= 0
#define SKP_RSHIFT_uint(a, shift)         ((a)>>(shift))        // shift >= 0

#define SKP_ADD_LSHIFT(a, b, shift)       ((a) + SKP_LSHIFT((b), (shift)))            // shift >= 0
#define SKP_ADD_LSHIFT32(a, b, shift)     SKP_ADD32((a), SKP_LSHIFT32((b), (shift)))    // shift >= 0
#define SKP_ADD_LSHIFT_uint(a, b, shift)  ((a) + SKP_LSHIFT_uint((b), (shift)))        // shift >= 0
#define SKP_ADD_RSHIFT(a, b, shift)       ((a) + SKP_RSHIFT((b), (shift)))            // shift >= 0
#define SKP_ADD_RSHIFT32(a, b, shift)     SKP_ADD32((a), SKP_RSHIFT32((b), (shift)))    // shift >= 0
#define SKP_ADD_RSHIFT_uint(a, b, shift)  ((a) + SKP_RSHIFT_uint((b), (shift)))        // shift >= 0
#define SKP_SUB_LSHIFT32(a, b, shift)     SKP_SUB32((a), SKP_LSHIFT32((b), (shift)))    // shift >= 0
#define SKP_SUB_RSHIFT32(a, b, shift)     SKP_SUB32((a), SKP_RSHIFT32((b), (shift)))    // shift >= 0

/* Requires that shift > 0 */
#define SKP_RSHIFT_ROUND(a, shift)        ((shift) == 1 ? ((a) >> 1) + ((a) & 1) : (((a) >> ((shift) - 1)) + 1) >> 1)
#define SKP_RSHIFT_ROUND64(a, shift)      ((shift) == 1 ? ((a) >> 1) + ((a) & 1) : (((a) >> ((shift) - 1)) + 1) >> 1)

/* Number of rightshift required to fit the multiplication */
#define SKP_NSHIFT_MUL_32_32(a, b)        ( -(31- (32-silk_CLZ32(SKP_abs(a)) + (32-silk_CLZ32(SKP_abs(b))))) )
#define SKP_NSHIFT_MUL_16_16(a, b)        ( -(15- (16-silk_CLZ16(SKP_abs(a)) + (16-silk_CLZ16(SKP_abs(b))))) )


#define SKP_min(a, b)                     (((a) < (b)) ? (a) : (b)) 
#define SKP_max(a, b)                     (((a) > (b)) ? (a) : (b))

/* Macro to convert floating-point constants to fixed-point */
#define SILK_FIX_CONST( C, Q )           ((SKP_int32)((C) * ((SKP_int64)1 << (Q)) + 0.5))

/* SKP_min() versions with typecast in the function call */
SKP_INLINE SKP_int SKP_min_int(SKP_int a, SKP_int b)
{
    return (((a) < (b)) ? (a) : (b));
}
SKP_INLINE SKP_int16 SKP_min_16(SKP_int16 a, SKP_int16 b)
{
    return (((a) < (b)) ? (a) : (b));
}
SKP_INLINE SKP_int32 SKP_min_32(SKP_int32 a, SKP_int32 b)
{
    return (((a) < (b)) ? (a) : (b));
}
SKP_INLINE SKP_int64 SKP_min_64(SKP_int64 a, SKP_int64 b)
{
    return (((a) < (b)) ? (a) : (b));
}

/* SKP_min() versions with typecast in the function call */
SKP_INLINE SKP_int SKP_max_int(SKP_int a, SKP_int b)
{
    return (((a) > (b)) ? (a) : (b));
}
SKP_INLINE SKP_int16 SKP_max_16(SKP_int16 a, SKP_int16 b)
{
    return (((a) > (b)) ? (a) : (b));
}
SKP_INLINE SKP_int32 SKP_max_32(SKP_int32 a, SKP_int32 b)
{
    return (((a) > (b)) ? (a) : (b));
}
SKP_INLINE SKP_int64 SKP_max_64(SKP_int64 a, SKP_int64 b)
{
    return (((a) > (b)) ? (a) : (b));
}

#define SKP_LIMIT( a, limit1, limit2)    ((limit1) > (limit2) ? ((a) > (limit1) ? (limit1) : ((a) < (limit2) ? (limit2) : (a))) \
                                                             : ((a) > (limit2) ? (limit2) : ((a) < (limit1) ? (limit1) : (a))))

#define SKP_LIMIT_int SKP_LIMIT
#define SKP_LIMIT_16 SKP_LIMIT
#define SKP_LIMIT_32 SKP_LIMIT

//#define SKP_non_neg(a)                 ((a) & ((-(a)) >> (8 * sizeof(a) - 1)))   /* doesn't seem faster than SKP_max(0, a);

#define SKP_abs(a)                       (((a) >  0)  ? (a) : -(a))            // Be careful, SKP_abs returns wrong when input equals to SKP_intXX_MIN
#define SKP_abs_int(a)                   (((a) ^ ((a) >> (8 * sizeof(a) - 1))) - ((a) >> (8 * sizeof(a) - 1)))
#define SKP_abs_int32(a)                 (((a) ^ ((a) >> 31)) - ((a) >> 31))
#define SKP_abs_int64(a)                 (((a) >  0)  ? (a) : -(a))    

#define SKP_sign(a)                      ((a) > 0 ? 1 : ( (a) < 0 ? -1 : 0 ))

#define SKP_sqrt(a)                      (sqrt(a)) 

/* PSEUDO-RANDOM GENERATOR                                                          */
/* Make sure to store the result as the seed for the next call (also in between     */
/* frames), otherwise result won't be random at all. When only using some of the    */
/* bits, take the most significant bits by right-shifting.                          */
#define SKP_RAND(seed)                   (SKP_MLA_ovflw(907633515, (seed), 196314165))

// Add some multiplication functions that can be easily mapped to ARM.

//    SKP_SMMUL: Signed top word multiply. 
//        ARMv6        2 instruction cycles. 
//        ARMv3M+        3 instruction cycles. use SMULL and ignore LSB registers.(except xM) 
//#define SKP_SMMUL(a32, b32)            (SKP_int32)SKP_RSHIFT(SKP_SMLAL(SKP_SMULWB((a32), (b32)), (a32), SKP_RSHIFT_ROUND((b32), 16)), 16)
// the following seems faster on x86
#define SKP_SMMUL(a32, b32)              (SKP_int32)SKP_RSHIFT64(SKP_SMULL((a32), (b32)), 32)

#include "silk_Inlines.h"
#include "silk_MacroCount.h"
#include "silk_MacroDebug.h"

#ifdef  __cplusplus
}
#endif

#endif
