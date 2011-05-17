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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "silk_API.h"
#include "silk_main.h"

/************************/
/* Decoder Super Struct */
/************************/
typedef struct {
    silk_decoder_state          channel_state[ DECODER_NUM_CHANNELS ];
    stereo_dec_state                sStereo;
    SKP_int                         nChannelsAPI;
    SKP_int                         nChannelsInternal;
} silk_decoder;

/*********************/
/* Decoder functions */
/*********************/

SKP_int silk_Get_Decoder_Size( SKP_int32 *decSizeBytes ) 
{
    SKP_int ret = SILK_NO_ERROR;

    *decSizeBytes = sizeof( silk_decoder );

    return ret;
}

/* Reset decoder state */
SKP_int silk_InitDecoder(
    void* decState                                      /* I/O: State                                          */
)
{
    SKP_int n, ret = SILK_NO_ERROR;
    silk_decoder_state *channel_state = ((silk_decoder *)decState)->channel_state;

    for( n = 0; n < DECODER_NUM_CHANNELS; n++ ) {
        ret  = silk_init_decoder( &channel_state[ n ] );
    }

    return ret;
}

/* Decode a frame */
SKP_int silk_Decode(
    void*                               decState,       /* I/O: State                                           */
    silk_DecControlStruct*      decControl,     /* I/O: Control Structure                               */
    SKP_int                             lostFlag,       /* I:   0: no loss, 1 loss, 2 decode FEC                */
    SKP_int                             newPacketFlag,  /* I:   Indicates first decoder call for this packet    */
    ec_dec                              *psRangeDec,    /* I/O  Compressor data structure                       */
    SKP_int16                           *samplesOut,    /* O:   Decoded output speech vector                    */
    SKP_int32                           *nSamplesOut    /* O:   Number of samples decoded                       */
)
{
    SKP_int   i, n, prev_fs_kHz, decode_only_middle = 0, ret = SILK_NO_ERROR;
    SKP_int32 nSamplesOutDec, LBRR_symbol;
    SKP_int16 samplesOut1_tmp[ 2 ][ MAX_FS_KHZ * MAX_FRAME_LENGTH_MS + 2 ];
    SKP_int16 samplesOut2_tmp[ MAX_API_FS_KHZ * MAX_FRAME_LENGTH_MS ];
    SKP_int   MS_pred_Q13[ 2 ] = { 0 };
    SKP_int16 *resample_out_ptr;
    silk_decoder *psDec = ( silk_decoder * )decState;
    silk_decoder_state *channel_state = psDec->channel_state;

    /**********************************/
    /* Test if first frame in payload */
    /**********************************/
    if( newPacketFlag ) {
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            channel_state[ n ].nFramesDecoded = 0;  /* Used to count frames in packet */
        }
    }

    /* Save previous sample frequency */
    prev_fs_kHz = channel_state[ 0 ].fs_kHz;

    /* If Mono -> Stereo transition in bitstream: init state of second channel */
    if( decControl->nChannelsInternal > psDec->nChannelsInternal ) {
        ret += silk_init_decoder( &channel_state[ 1 ] );
        if( psDec->nChannelsAPI == 2 ) {
            SKP_memcpy( &channel_state[ 1 ].resampler_state, &channel_state[ 0 ].resampler_state, sizeof( silk_resampler_state_struct ) );
        }
    }

    for( n = 0; n < decControl->nChannelsInternal; n++ ) {
        if( channel_state[ n ].nFramesDecoded == 0 ) {
            SKP_int fs_kHz_dec;
            if( decControl->payloadSize_ms == 10 ) {
                channel_state[ n ].nFramesPerPacket = 1;
                channel_state[ n ].nb_subfr = 2;
            } else if( decControl->payloadSize_ms == 20 ) {
                channel_state[ n ].nFramesPerPacket = 1;
                channel_state[ n ].nb_subfr = 4;
            } else if( decControl->payloadSize_ms == 40 ) {
                channel_state[ n ].nFramesPerPacket = 2;
                channel_state[ n ].nb_subfr = 4;
            } else if( decControl->payloadSize_ms == 60 ) {
                channel_state[ n ].nFramesPerPacket = 3;
                channel_state[ n ].nb_subfr = 4;
            } else {
                SKP_assert( 0 );
                return SILK_DEC_INVALID_FRAME_SIZE;
            } 
            fs_kHz_dec = ( decControl->internalSampleRate >> 10 ) + 1;
            if( fs_kHz_dec != 8 && fs_kHz_dec != 12 && fs_kHz_dec != 16 ) {
                SKP_assert( 0 );
                return SILK_DEC_INVALID_SAMPLING_FREQUENCY;
            }
            silk_decoder_set_fs( &channel_state[ n ], fs_kHz_dec );
        }
    }

    /* Initialize resampler when switching internal or external sampling frequency */
    if( prev_fs_kHz != channel_state[ 0 ].fs_kHz || channel_state[ 0 ].prev_API_sampleRate != decControl->API_sampleRate ) {
        ret = silk_resampler_init( &channel_state[ 0 ].resampler_state, SKP_SMULBB( channel_state[ 0 ].fs_kHz, 1000 ), decControl->API_sampleRate );
        if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 ) {
            SKP_memcpy( &channel_state[ 1 ].resampler_state, &channel_state[ 0 ].resampler_state, sizeof( silk_resampler_state_struct ) );
        }
    }
    channel_state[ 0 ].prev_API_sampleRate = decControl->API_sampleRate;
    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 && ( psDec->nChannelsAPI == 1 || psDec->nChannelsInternal == 1 ) ) {
        SKP_memset( psDec->sStereo.pred_prev_Q13, 0, sizeof( psDec->sStereo.pred_prev_Q13 ) );
        SKP_memset( psDec->sStereo.sSide, 0, sizeof( psDec->sStereo.sSide ) );
    }
    psDec->nChannelsAPI      = decControl->nChannelsAPI;
    psDec->nChannelsInternal = decControl->nChannelsInternal;

    if( decControl->API_sampleRate > MAX_API_FS_KHZ * 1000 || decControl->API_sampleRate < 8000 ) {
        ret = SILK_DEC_INVALID_SAMPLING_FREQUENCY;
        return( ret );
    }

    if( lostFlag != FLAG_PACKET_LOST && channel_state[ 0 ].nFramesDecoded == 0 ) {
        /* First decoder call for this payload */
        /* Decode VAD flags and LBRR flag */
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            for( i = 0; i < channel_state[ n ].nFramesPerPacket; i++ ) {
                channel_state[ n ].VAD_flags[ i ] = ec_dec_bit_logp(psRangeDec, 1);
            }
            channel_state[ n ].LBRR_flag = ec_dec_bit_logp(psRangeDec, 1);
        }        
        /* Decode LBRR flags */
        for( n = 0; n < decControl->nChannelsInternal; n++ ) {
            SKP_memset( channel_state[ n ].LBRR_flags, 0, sizeof( channel_state[ n ].LBRR_flags ) );
            if( channel_state[ n ].LBRR_flag ) {
                if( channel_state[ n ].nFramesPerPacket == 1 ) {
                    channel_state[ n ].LBRR_flags[ 0 ] = 1;
                } else {
                    LBRR_symbol = ec_dec_icdf( psRangeDec, silk_LBRR_flags_iCDF_ptr[ channel_state[ n ].nFramesPerPacket - 2 ], 8 ) + 1;
                    for( i = 0; i < channel_state[ n ].nFramesPerPacket; i++ ) {
                        channel_state[ n ].LBRR_flags[ i ] = SKP_RSHIFT( LBRR_symbol, i ) & 1;
                    }
                }
            }
        }

        if( lostFlag == FLAG_DECODE_NORMAL ) {
            /* Regular decoding: skip all LBRR data */
            for( i = 0; i < channel_state[ 0 ].nFramesPerPacket; i++ ) {
                for( n = 0; n < decControl->nChannelsInternal; n++ ) {
                    if( channel_state[ n ].LBRR_flags[ i ] ) {
                        SKP_int pulses[ MAX_FRAME_LENGTH ];
                        if( decControl->nChannelsInternal == 2 && n == 0 ) {
                            silk_stereo_decode_pred( psRangeDec, &decode_only_middle, MS_pred_Q13 );
                        }
                        silk_decode_indices( &channel_state[ n ], psRangeDec, i, 1 );
                        silk_decode_pulses( psRangeDec, pulses, channel_state[ n ].indices.signalType, 
                            channel_state[ n ].indices.quantOffsetType, channel_state[ n ].frame_length );
                    }
                }
            }
        }
    }

    /* Get MS predictor index */
    if( decControl->nChannelsInternal == 2 ) {
        if(   lostFlag == FLAG_DECODE_NORMAL || 
            ( lostFlag == FLAG_DECODE_LBRR && channel_state[ 0 ].LBRR_flags[ channel_state[ 0 ].nFramesDecoded ] == 1 ) ) 
        {
            silk_stereo_decode_pred( psRangeDec, &decode_only_middle, MS_pred_Q13 );
        } else {
            SKP_memcpy( MS_pred_Q13, &psDec->sStereo.pred_prev_Q13, sizeof( MS_pred_Q13 ) );
        }
    }

    /* Call decoder for one frame */
    for( n = 0; n < decControl->nChannelsInternal; n++ ) {
        if( n == 0 || decode_only_middle == 0 ) {
            ret += silk_decode_frame( &channel_state[ n ], psRangeDec, &samplesOut1_tmp[ n ][ 2 ], &nSamplesOutDec, lostFlag );
        } else {
            SKP_memset( &samplesOut1_tmp[ n ][ 2 ], 0, nSamplesOutDec * sizeof( SKP_int16 ) );
        }
    }

    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 ) {
        /* Convert Mid/Side to Left/Right */
        silk_stereo_MS_to_LR( &psDec->sStereo, samplesOut1_tmp[ 0 ], samplesOut1_tmp[ 1 ], MS_pred_Q13, channel_state[ 0 ].fs_kHz, nSamplesOutDec );
    } else {
        /* Buffering */
        SKP_memcpy( samplesOut1_tmp[ 0 ], psDec->sStereo.sMid, 2 * sizeof( SKP_int16 ) );
        SKP_memcpy( psDec->sStereo.sMid, &samplesOut1_tmp[ 0 ][ nSamplesOutDec ], 2 * sizeof( SKP_int16 ) );
    }

    /* Number of output samples */
    *nSamplesOut = SKP_DIV32( nSamplesOutDec * decControl->API_sampleRate, SKP_SMULBB( channel_state[ 0 ].fs_kHz, 1000 ) );

    /* Set up pointers to temp buffers */
    if( decControl->nChannelsAPI == 2 ) {
        resample_out_ptr = samplesOut2_tmp;
    } else {
        resample_out_ptr = samplesOut;
    }

    for( n = 0; n < SKP_min( decControl->nChannelsAPI, decControl->nChannelsInternal ); n++ ) {
        /* Resample decoded signal to API_sampleRate */
        ret += silk_resampler( &channel_state[ n ].resampler_state, resample_out_ptr, &samplesOut1_tmp[ n ][ 1 ], nSamplesOutDec );

        /* Interleave if stereo output and stereo stream */
        if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 2 ) {
            for( i = 0; i < *nSamplesOut; i++ ) {
                samplesOut[ n + 2 * i ] = resample_out_ptr[ i ];
            }
        }
    }

    /* Create two channel output from mono stream */
    if( decControl->nChannelsAPI == 2 && decControl->nChannelsInternal == 1 ) {
        for( i = 0; i < *nSamplesOut; i++ ) {
            samplesOut[ 0 + 2 * i ] = samplesOut[ 1 + 2 * i ] = resample_out_ptr[ i ];
        }
    }

    return ret;
}

/* Getting table of contents for a packet */
SKP_int silk_get_TOC(
    const SKP_uint8                     *payload,           /* I    Payload data                                */
    const SKP_int                       nBytesIn,           /* I:   Number of input bytes                       */
    const SKP_int                       nFramesPerPayload,  /* I:   Number of SILK frames per payload           */
    silk_TOC_struct                 *Silk_TOC           /* O:   Type of content                             */
)
{
    SKP_int i, flags, ret = SILK_NO_ERROR;

    if( nBytesIn < 1 ) {
        return -1;
    }
    if( nFramesPerPayload < 0 || nFramesPerPayload > 3 ) {
        return -1;
    }

    SKP_memset( Silk_TOC, 0, sizeof( Silk_TOC ) );

    /* For stereo, extract the flags for the mid channel */
    flags = SKP_RSHIFT( payload[ 0 ], 7 - nFramesPerPayload ) & ( SKP_LSHIFT( 1, nFramesPerPayload + 1 ) - 1 );

    Silk_TOC->inbandFECFlag = flags & 1;
    for( i = nFramesPerPayload - 1; i >= 0 ; i-- ) {
        flags = SKP_RSHIFT( flags, 1 );
        Silk_TOC->VADFlags[ i ] = flags & 1;
        Silk_TOC->VADFlag |= flags & 1;
    }

    return ret;
}
