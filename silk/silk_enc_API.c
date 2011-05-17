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


#include "silk_define.h"
#include "silk_API.h"
#include "silk_control.h"
#include "silk_typedef.h"
#include "silk_structs.h"
#include "silk_tuning_parameters.h"
#ifdef FIXED_POINT
#include "silk_main_FIX.h"
#else
#include "silk_main_FLP.h"
#endif

/****************************************/
/* Encoder functions                    */
/****************************************/

SKP_int silk_Get_Encoder_Size( SKP_int32 *encSizeBytes )
{
    SKP_int ret = SILK_NO_ERROR;
    
    *encSizeBytes = sizeof( silk_encoder );
    
    return ret;
}

/*************************/
/* Init or Reset encoder */
/*************************/
SKP_int silk_InitEncoder(
    void                            *encState,          /* I/O: State                                           */
    silk_EncControlStruct   *encStatus          /* O:   Control structure                               */
)
{
    silk_encoder *psEnc;
    SKP_int n, ret = SILK_NO_ERROR;

    psEnc = (silk_encoder *)encState;
    
    /* Reset encoder */
    SKP_memset( psEnc, 0, sizeof( silk_encoder ) );
    for( n = 0; n < ENCODER_NUM_CHANNELS; n++ ) {
        if( ret += silk_init_encoder( &psEnc->state_Fxx[ n ] ) ) {
            SKP_assert( 0 );
        }
    }

    psEnc->nChannelsAPI = 1;
    psEnc->nChannelsInternal = 1;

    /* Read control structure */
    if( ret += silk_QueryEncoder( encState, encStatus ) ) {
        SKP_assert( 0 );
    }

    return ret;
}

/***************************************/
/* Read control structure from encoder */
/***************************************/
SKP_int silk_QueryEncoder(
    const void *encState,                       /* I:   State Vector                                    */
    silk_EncControlStruct *encStatus    /* O:   Control Structure                               */
)
{
    SKP_int ret = SILK_NO_ERROR;
    silk_encoder_state_Fxx *state_Fxx;
    silk_encoder *psEnc = (silk_encoder *)encState;

    state_Fxx = psEnc->state_Fxx;

    encStatus->nChannelsAPI              = psEnc->nChannelsAPI;
    encStatus->nChannelsInternal         = psEnc->nChannelsInternal;
    encStatus->API_sampleRate            = state_Fxx[ 0 ].sCmn.API_fs_Hz;
    encStatus->maxInternalSampleRate     = state_Fxx[ 0 ].sCmn.maxInternal_fs_Hz;
    encStatus->minInternalSampleRate     = state_Fxx[ 0 ].sCmn.minInternal_fs_Hz;
    encStatus->desiredInternalSampleRate = state_Fxx[ 0 ].sCmn.desiredInternal_fs_Hz;
    encStatus->payloadSize_ms            = state_Fxx[ 0 ].sCmn.PacketSize_ms;
    encStatus->bitRate                   = state_Fxx[ 0 ].sCmn.TargetRate_bps;
    encStatus->packetLossPercentage      = state_Fxx[ 0 ].sCmn.PacketLoss_perc;
    encStatus->complexity                = state_Fxx[ 0 ].sCmn.Complexity;
    encStatus->useInBandFEC              = state_Fxx[ 0 ].sCmn.useInBandFEC;
    encStatus->useDTX                    = state_Fxx[ 0 ].sCmn.useDTX;
    encStatus->useCBR                    = state_Fxx[ 0 ].sCmn.useCBR;
    encStatus->internalSampleRate        = SKP_SMULBB( state_Fxx[ 0 ].sCmn.fs_kHz, 1000 );
    encStatus->allowBandwidthSwitch      = state_Fxx[ 0 ].sCmn.allow_bandwidth_switch;
    encStatus->inWBmodeWithoutVariableLP = state_Fxx[ 0 ].sCmn.fs_kHz == 16 && state_Fxx[ 0 ].sCmn.sLP.mode == 0;

    return ret;
}

/**************************/
/* Encode frame with Silk */
/**************************/
SKP_int silk_Encode( 
    void                                *encState,      /* I/O: State                                           */
    silk_EncControlStruct       *encControl,    /* I:   Control structure                               */
    const SKP_int16                     *samplesIn,     /* I:   Speech sample input vector                      */
    SKP_int                             nSamplesIn,     /* I:   Number of samples in input vector               */
    ec_enc                              *psRangeEnc,    /* I/O  Compressor data structure                       */
    SKP_int32                           *nBytesOut,     /* I/O: Number of bytes in payload (input: Max bytes)   */
    const SKP_int                       prefillFlag     /* I:   Flag to indicate prefilling buffers; no coding  */
)
{
    SKP_int   n, i, nBits, flags, tmp_payloadSize_ms = 0, tmp_complexity = 0, ret = 0;
    SKP_int   nSamplesToBuffer, nBlocksOf10ms, nSamplesFromInput = 0;
    SKP_int   speech_act_thr_for_switch_Q8;
    SKP_int32 TargetRate_bps, MStargetRates_bps[ 2 ], channelRate_bps, LBRR_symbol;
    silk_encoder *psEnc = ( silk_encoder * )encState;
    SKP_int   MS_predictorIx[ 2 ] = { 0 };
    SKP_int16 buf[ MAX_FRAME_LENGTH_MS * MAX_API_FS_KHZ ];

    /* Check values in encoder control structure */
    if( ( ret = check_control_input( encControl ) != 0 ) ) {
        SKP_assert( 0 );
        return ret;
    }

    if( encControl->nChannelsInternal > psEnc->nChannelsInternal ) {
        /* Mono -> Stereo transition: init state of second channel and stereo state */
        ret += silk_init_encoder( &psEnc->state_Fxx[ 1 ] );
        SKP_memset( psEnc->sStereo.pred_prev_Q13, 0, sizeof( psEnc->sStereo.pred_prev_Q13 ) );
        SKP_memset( psEnc->sStereo.sSide, 0, sizeof( psEnc->sStereo.sSide ) );
        SKP_memset( psEnc->sStereo.mid_side_amp_Q0, 0, sizeof( psEnc->sStereo.mid_side_amp_Q0 ) );
        psEnc->sStereo.width_prev_Q14 = 0;
        psEnc->sStereo.smth_width_Q14 = SILK_FIX_CONST( 1, 14 );
        if( psEnc->nChannelsAPI == 2 ) {
            SKP_memcpy( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state, &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, sizeof( silk_resampler_state_struct ) );
            SKP_memcpy( &psEnc->state_Fxx[ 1 ].sCmn.In_HP_State,     &psEnc->state_Fxx[ 0 ].sCmn.In_HP_State,     sizeof( psEnc->state_Fxx[ 1 ].sCmn.In_HP_State ) );
        }
    }
    psEnc->nChannelsAPI = encControl->nChannelsAPI;
    psEnc->nChannelsInternal = encControl->nChannelsInternal;

    nBlocksOf10ms = SKP_DIV32( 100 * nSamplesIn, encControl->API_sampleRate );
    if( prefillFlag ) {
        /* Only accept input length of 10 ms */
        if( nBlocksOf10ms != 1 ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            SKP_assert( 0 );
            return ret;
        }
        /* Reset Encoder */
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            if( ret = silk_init_encoder( &psEnc->state_Fxx[ n ] ) ) {
                SKP_assert( 0 );
            }
        }
        tmp_payloadSize_ms = encControl->payloadSize_ms;
        encControl->payloadSize_ms = 10;
        tmp_complexity = encControl->complexity;
        encControl->complexity = 0;
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
            psEnc->state_Fxx[ n ].sCmn.prefillFlag = 1;
        }
    } else {
        /* Only accept input lengths that are a multiple of 10 ms */
        if( nBlocksOf10ms * encControl->API_sampleRate != 100 * nSamplesIn || nSamplesIn < 0 ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            SKP_assert( 0 );
            return ret;
        }
        /* Make sure no more than one packet can be produced */
        if( 1000 * (SKP_int32)nSamplesIn > encControl->payloadSize_ms * encControl->API_sampleRate ) {
            ret = SILK_ENC_INPUT_INVALID_NO_OF_SAMPLES;
            SKP_assert( 0 );
            return ret;
        }
    }

    TargetRate_bps = SKP_RSHIFT32( encControl->bitRate, encControl->nChannelsInternal - 1 );
    for( n = 0; n < encControl->nChannelsInternal; n++ ) {
        if( ( ret = silk_control_encoder( &psEnc->state_Fxx[ n ], encControl, TargetRate_bps, psEnc->allowBandwidthSwitch, n ) ) != 0 ) {
            SKP_assert( 0 );
            return ret;
        }
    }
    SKP_assert( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 0 ].sCmn.fs_kHz == psEnc->state_Fxx[ 1 ].sCmn.fs_kHz );

    /* Input buffering/resampling and encoding */
    while( 1 ) {
        nSamplesToBuffer  = psEnc->state_Fxx[ 0 ].sCmn.frame_length - psEnc->state_Fxx[ 0 ].sCmn.inputBufIx;
        nSamplesToBuffer  = SKP_min( nSamplesToBuffer, 10 * nBlocksOf10ms * psEnc->state_Fxx[ 0 ].sCmn.fs_kHz );
        nSamplesFromInput = SKP_DIV32_16( nSamplesToBuffer * psEnc->state_Fxx[ 0 ].sCmn.API_fs_Hz, psEnc->state_Fxx[ 0 ].sCmn.fs_kHz * 1000 );
        /* Resample and write to buffer */
        if( encControl->nChannelsAPI == 2 && encControl->nChannelsInternal == 2 ) {
            for( n = 0; n < nSamplesFromInput; n++ ) {
                buf[ n ] = samplesIn[ 2 * n ];
            }
            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, 
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx ], buf, nSamplesFromInput );
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;

            nSamplesToBuffer  = psEnc->state_Fxx[ 1 ].sCmn.frame_length - psEnc->state_Fxx[ 1 ].sCmn.inputBufIx;
            nSamplesToBuffer  = SKP_min( nSamplesToBuffer, 10 * nBlocksOf10ms * psEnc->state_Fxx[ 1 ].sCmn.fs_kHz );
            for( n = 0; n < nSamplesFromInput; n++ ) {
                buf[ n ] = samplesIn[ 2 * n + 1 ];
            }
            ret += silk_resampler( &psEnc->state_Fxx[ 1 ].sCmn.resampler_state, 
                &psEnc->state_Fxx[ 1 ].sCmn.inputBuf[ psEnc->state_Fxx[ 1 ].sCmn.inputBufIx ], buf, nSamplesFromInput );
            psEnc->state_Fxx[ 1 ].sCmn.inputBufIx += nSamplesToBuffer;
        } else if( encControl->nChannelsAPI == 2 && encControl->nChannelsInternal == 1 ) {
            /* Combine left and right channels before resampling */
            for( n = 0; n < nSamplesFromInput; n++ ) {
                buf[ n ] = (SKP_int16)SKP_RSHIFT_ROUND( samplesIn[ 2 * n ] + samplesIn[ 2 * n + 1 ],  1 );
            }
            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, 
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx ], buf, nSamplesFromInput );
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;
        } else {
            SKP_assert( encControl->nChannelsAPI == 1 && encControl->nChannelsInternal == 1 );
            ret += silk_resampler( &psEnc->state_Fxx[ 0 ].sCmn.resampler_state, 
                &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.inputBufIx ], samplesIn, nSamplesFromInput );
            psEnc->state_Fxx[ 0 ].sCmn.inputBufIx += nSamplesToBuffer;
        }
        samplesIn  += nSamplesFromInput * encControl->nChannelsAPI;
        nSamplesIn -= nSamplesFromInput;

        /* Default */
        psEnc->allowBandwidthSwitch = 0;

        /* Silk encoder */
        if( psEnc->state_Fxx[ 0 ].sCmn.inputBufIx >= psEnc->state_Fxx[ 0 ].sCmn.frame_length ) {
            /* Enough data in input buffer, so encode */
            SKP_assert( psEnc->state_Fxx[ 0 ].sCmn.inputBufIx == psEnc->state_Fxx[ 0 ].sCmn.frame_length );
            SKP_assert( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 1 ].sCmn.inputBufIx == psEnc->state_Fxx[ 1 ].sCmn.frame_length );

            /* Deal with LBRR data */
            if( psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded == 0 && !prefillFlag ) {
                /* Create space at start of payload for VAD and FEC flags */
                SKP_uint8 iCDF[ 2 ] = { 0, 0 };
                iCDF[ 0 ] = 256 - SKP_RSHIFT( 256, ( psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket + 1 ) * encControl->nChannelsInternal );
                ec_enc_icdf( psRangeEnc, 0, iCDF, 8 );

                /* Encode any LBRR data from previous packet */
                /* Encode LBRR flags */
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                    LBRR_symbol = 0;
                    for( i = 0; i < psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket; i++ ) {
                        LBRR_symbol |= SKP_LSHIFT( psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i ], i );
                    }
                    psEnc->state_Fxx[ n ].sCmn.LBRR_flag = LBRR_symbol > 0 ? 1 : 0;
                    if( LBRR_symbol && psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket > 1 ) {
                        ec_enc_icdf( psRangeEnc, LBRR_symbol - 1, silk_LBRR_flags_iCDF_ptr[ psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket - 2 ], 8 );
                    }
                }

                /* Code LBRR indices and excitation signals */
                for( i = 0; i < psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket; i++ ) {
                    for( n = 0; n < encControl->nChannelsInternal; n++ ) {                
                        if( psEnc->state_Fxx[ n ].sCmn.LBRR_flags[ i ] ) {
                            if( encControl->nChannelsInternal == 2 && n == 0 ) {
                                silk_stereo_encode_pred( psRangeEnc, psEnc->sStereo.ix[ i ] );
                            }
                            silk_encode_indices( &psEnc->state_Fxx[ n ].sCmn, psRangeEnc, i, 1 );
                            silk_encode_pulses( psRangeEnc, psEnc->state_Fxx[ n ].sCmn.indices_LBRR[i].signalType, psEnc->state_Fxx[ n ].sCmn.indices_LBRR[i].quantOffsetType, 
                                psEnc->state_Fxx[ n ].sCmn.pulses_LBRR[ i ], psEnc->state_Fxx[ n ].sCmn.frame_length );
                        }
                    }
                }

                /* Reset LBRR flags */
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {                
                    SKP_memset( psEnc->state_Fxx[ n ].sCmn.LBRR_flags, 0, sizeof( psEnc->state_Fxx[ n ].sCmn.LBRR_flags ) );
                }
            }

            /* High-pass filter */
            silk_HP_variable_cutoff( psEnc->state_Fxx, psEnc->nChannelsInternal );

            /* Total target bits for packet */
            nBits = SKP_DIV32_16( SKP_MUL( encControl->bitRate, encControl->payloadSize_ms ), 1000 );
            /* Subtract bits already used */
            nBits -= ec_tell( psRangeEnc );
            /* Divide by number of uncoded frames left in packet */
            nBits = SKP_DIV32_16( nBits, psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket - psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded );
            /* Convert to bits/second */
            if( encControl->payloadSize_ms == 10 ) {
                TargetRate_bps = SKP_SMULBB( nBits, 100 );
            } else {
                TargetRate_bps = SKP_SMULBB( nBits, 50 );
            }
            /* Subtract fraction of bits in excess of target in previous packets */
            TargetRate_bps -= SKP_DIV32_16( SKP_MUL( psEnc->nBitsExceeded, 1000 ), BITRESERVOIR_DECAY_TIME_MS );
            /* Don't exceed input bitrate */
            TargetRate_bps = SKP_min( TargetRate_bps, encControl->bitRate );

            /* Convert Left/Right to Mid/Side */
            if( encControl->nChannelsInternal == 2 ) {
                silk_stereo_LR_to_MS( &psEnc->sStereo, psEnc->state_Fxx[ 0 ].sCmn.inputBuf, psEnc->state_Fxx[ 1 ].sCmn.inputBuf, 
                    psEnc->sStereo.ix[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ], MStargetRates_bps, TargetRate_bps, 
                    psEnc->state_Fxx[ 0 ].sCmn.speech_activity_Q8, psEnc->state_Fxx[ 0 ].sCmn.fs_kHz, psEnc->state_Fxx[ 0 ].sCmn.frame_length );
                silk_stereo_encode_pred( psRangeEnc, psEnc->sStereo.ix[ psEnc->state_Fxx[ 0 ].sCmn.nFramesEncoded ] );
            } else {
                /* Buffering */
                SKP_memcpy( &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ -2 ], psEnc->sStereo.sMid, 2 * sizeof( SKP_int16 ) );
                SKP_memcpy( psEnc->sStereo.sMid, &psEnc->state_Fxx[ 0 ].sCmn.inputBuf[ psEnc->state_Fxx[ 0 ].sCmn.frame_length - 2 ], 2 * sizeof( SKP_int16 ) );
            }

            /* Encode */
            for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                /* For stereo coding, allocate 60% of the bitrate to mid and 40% to side */
                if( encControl->nChannelsInternal == 1 ) {
                    channelRate_bps = TargetRate_bps;
                } else {
                    channelRate_bps = MStargetRates_bps[ n ];
                }

                if( channelRate_bps > 0 ) {
                    silk_control_SNR( &psEnc->state_Fxx[ n ].sCmn, channelRate_bps );

                    if( ( ret = silk_encode_frame_Fxx( &psEnc->state_Fxx[ n ], nBytesOut, psRangeEnc ) ) != 0 ) {
                        SKP_assert( 0 );
                    }
                }

                psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
                psEnc->state_Fxx[ n ].sCmn.inputBufIx = 0;
            }

            /* Insert VAD and FEC flags at beginning of bitstream */
            if( *nBytesOut > 0 ) {
                flags = 0;
                for( n = 0; n < encControl->nChannelsInternal; n++ ) {
                    for( i = 0; i < psEnc->state_Fxx[ n ].sCmn.nFramesPerPacket; i++ ) {
                        flags  = SKP_LSHIFT( flags, 1 );
                        flags |= psEnc->state_Fxx[ n ].sCmn.VAD_flags[ i ];
                    }
                    flags  = SKP_LSHIFT( flags, 1 );
                    flags |= psEnc->state_Fxx[ n ].sCmn.LBRR_flag;
                }
                ec_enc_patch_initial_bits( psRangeEnc, flags, ( psEnc->state_Fxx[ 0 ].sCmn.nFramesPerPacket + 1 ) * encControl->nChannelsInternal );

                /* Return zero bytes if all channels DTXed */
                if( psEnc->state_Fxx[ 0 ].sCmn.inDTX && ( encControl->nChannelsInternal == 1 || psEnc->state_Fxx[ 1 ].sCmn.inDTX ) ) {
                    *nBytesOut = 0;
                }

                psEnc->nBitsExceeded += *nBytesOut * 8;
                psEnc->nBitsExceeded -= SKP_DIV32_16( SKP_MUL( encControl->bitRate, encControl->payloadSize_ms ), 1000 );
                psEnc->nBitsExceeded  = SKP_LIMIT( psEnc->nBitsExceeded, 0, 10000 );

                /* Update flag indicating if bandwidth switching is allowed */
                speech_act_thr_for_switch_Q8 = SKP_SMLAWB( SILK_FIX_CONST( SPEECH_ACTIVITY_DTX_THRES, 8 ), 
                    SILK_FIX_CONST( ( 1 - SPEECH_ACTIVITY_DTX_THRES ) / MAX_BANDWIDTH_SWITCH_DELAY_MS, 16 + 8 ), psEnc->timeSinceSwitchAllowed_ms );
                if( psEnc->state_Fxx[ 0 ].sCmn.speech_activity_Q8 < speech_act_thr_for_switch_Q8 ) {
                    psEnc->allowBandwidthSwitch = 1;
                    psEnc->timeSinceSwitchAllowed_ms = 0;
                } else {
                    psEnc->allowBandwidthSwitch = 0;
                    psEnc->timeSinceSwitchAllowed_ms += encControl->payloadSize_ms;
                }
            }

            if( nSamplesIn == 0 ) {
                break;
            }
        } else {
            break;
        }
    }

    encControl->allowBandwidthSwitch = psEnc->allowBandwidthSwitch;
    encControl->inWBmodeWithoutVariableLP = psEnc->state_Fxx[ 0 ].sCmn.fs_kHz == 16 && psEnc->state_Fxx[ 0 ].sCmn.sLP.mode == 0;
    encControl->internalSampleRate = SKP_SMULBB( psEnc->state_Fxx[ 0 ].sCmn.fs_kHz, 1000 );
    if( prefillFlag ) {
        encControl->payloadSize_ms = tmp_payloadSize_ms;
        encControl->complexity = tmp_complexity;
        for( n = 0; n < encControl->nChannelsInternal; n++ ) {
            psEnc->state_Fxx[ n ].sCmn.controlled_since_last_payload = 0;
            psEnc->state_Fxx[ n ].sCmn.prefillFlag = 0;
        }
    }

    return ret;
}

