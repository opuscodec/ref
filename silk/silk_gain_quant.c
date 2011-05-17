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

#include "silk_main.h"

#define OFFSET                  ( ( MIN_QGAIN_DB * 128 ) / 6 + 16 * 128 )
#define SCALE_Q16               ( ( 65536 * ( N_LEVELS_QGAIN - 1 ) ) / ( ( ( MAX_QGAIN_DB - MIN_QGAIN_DB ) * 128 ) / 6 ) )
#define INV_SCALE_Q16           ( ( 65536 * ( ( ( MAX_QGAIN_DB - MIN_QGAIN_DB ) * 128 ) / 6 ) ) / ( N_LEVELS_QGAIN - 1 ) )

/* Gain scalar quantization with hysteresis, uniform on log scale */
void silk_gains_quant(
    SKP_int8                        ind[ MAX_NB_SUBFR ],        /* O    gain indices                            */
    SKP_int32                       gain_Q16[ MAX_NB_SUBFR ],   /* I/O  gains (quantized out)                   */
    SKP_int8                        *prev_ind,                  /* I/O  last index in previous frame            */
    const SKP_int                   conditional,                /* I    first gain is delta coded if 1          */
    const SKP_int                   nb_subfr                    /* I    number of subframes                     */
)
{
    SKP_int k, double_step_size_threshold;

    for( k = 0; k < nb_subfr; k++ ) {
        /* Add half of previous quantization error, convert to log scale, scale, floor() */
        ind[ k ] = SKP_SMULWB( SCALE_Q16, silk_lin2log( gain_Q16[ k ] ) - OFFSET );

        /* Round towards previous quantized gain (hysteresis) */
        if( ind[ k ] < *prev_ind ) {
            ind[ k ]++;
        }
        ind[ k ] = SKP_max_int( ind[ k ], 0 );

        /* Compute delta indices and limit */
        if( k == 0 && conditional == 0 ) {
            /* Full index */
            ind[ k ] = SKP_LIMIT_int( ind[ k ], *prev_ind + MIN_DELTA_GAIN_QUANT, N_LEVELS_QGAIN - 1 );
            *prev_ind = ind[ k ];
        } else {
            /* Delta index */
            ind[ k ] = ind[ k ] - *prev_ind;

            /* Double the quantization step size for large gain increases, so that the max gain level can be reached */
            double_step_size_threshold = 2 * MAX_DELTA_GAIN_QUANT - N_LEVELS_QGAIN + *prev_ind;
            if( ind[ k ] > double_step_size_threshold ) {
                ind[ k ] = double_step_size_threshold + SKP_RSHIFT( ind[ k ] - double_step_size_threshold + 1, 1 );
            }

            ind[ k ] = SKP_LIMIT_int( ind[ k ], MIN_DELTA_GAIN_QUANT, MAX_DELTA_GAIN_QUANT );

            /* Accumulate deltas */
            if( ind[ k ] > double_step_size_threshold ) {
                *prev_ind += SKP_LSHIFT( ind[ k ], 1 ) - double_step_size_threshold;
            } else {
                *prev_ind += ind[ k ];
            }

            /* Shift to make non-negative */
            ind[ k ] -= MIN_DELTA_GAIN_QUANT;
        }

        /* Convert to linear scale and scale */
        gain_Q16[ k ] = silk_log2lin( SKP_min_32( SKP_SMULWB( INV_SCALE_Q16, *prev_ind ) + OFFSET, 3967 ) ); /* 3967 = 31 in Q7 */
    }
}

/* Gains scalar dequantization, uniform on log scale */
void silk_gains_dequant(
    SKP_int32                       gain_Q16[ MAX_NB_SUBFR ],   /* O    quantized gains                         */
    const SKP_int8                  ind[ MAX_NB_SUBFR ],        /* I    gain indices                            */
    SKP_int8                        *prev_ind,                  /* I/O  last index in previous frame            */
    const SKP_int                   conditional,                /* I    first gain is delta coded if 1          */
    const SKP_int                   nb_subfr                    /* I    number of subframes                     */
)
{
    SKP_int   k, ind_tmp, double_step_size_threshold;

    for( k = 0; k < nb_subfr; k++ ) {
        if( k == 0 && conditional == 0 ) {
            *prev_ind = ind[ k ];
        } else {
            /* Delta index */
            ind_tmp = ind[ k ] + MIN_DELTA_GAIN_QUANT;

            /* Accumulate deltas */
            double_step_size_threshold = 2 * MAX_DELTA_GAIN_QUANT - N_LEVELS_QGAIN + *prev_ind;
            if( ind_tmp > double_step_size_threshold ) {
                *prev_ind += SKP_LSHIFT( ind_tmp, 1 ) - double_step_size_threshold;
            } else {
                *prev_ind += ind_tmp;
            }
        }

        /* Convert to linear scale and scale */
        gain_Q16[ k ] = silk_log2lin( SKP_min_32( SKP_SMULWB( INV_SCALE_Q16, *prev_ind ) + OFFSET, 3967 ) ); /* 3967 = 31 in Q7 */
    }
}
