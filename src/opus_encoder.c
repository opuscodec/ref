/* Copyright (c) 2010 Xiph.Org Foundation, Skype Limited
   Written by Jean-Marc Valin and Koen Vos */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "celt.h"
#include "opus_encoder.h"
#include "entenc.h"
#include "modes.h"
#include "silk_API.h"

/* Transition tables for the voice and audio modes. First column is the
   middle (memoriless) threshold. The second column is the hysteresis
   (difference with the middle) */
static const int voice_bandwidth_thresholds[10] = {
		11500, 1500, /* NB<->MB */
		14500, 1500, /* MB<->WB */
		21000, 2000, /* WB<->SWB */
		29000, 2000, /* SWB<->FB */
};
static const int audio_bandwidth_thresholds[10] = {
		30000,    0, /* MB not allowed */
		20000, 2000, /* MB<->WB */
		26000, 2000, /* WB<->SWB */
		33000, 2000, /* SWB<->FB */
};

/* Make sure everything's aligned to 4 bytes (this may need to be increased
   on really weird architectures) */
static inline int align(int i)
{
	return (i+3)&-4;
}

int opus_encoder_get_size(int channels)
{
	int silkEncSizeBytes, celtEncSizeBytes;
	int ret;
	ret = silk_Get_Encoder_Size( &silkEncSizeBytes );
	if(ret)
		return 0;
	silkEncSizeBytes = align(silkEncSizeBytes);
    celtEncSizeBytes = celt_encoder_get_size(channels);
    return align(sizeof(OpusEncoder))+silkEncSizeBytes+celtEncSizeBytes;

}

OpusEncoder *opus_encoder_init(OpusEncoder* st, int Fs, int channels)
{
	void *silk_enc;
	CELTEncoder *celt_enc;
    int err;
	int ret, silkEncSizeBytes, celtEncSizeBytes;

	memset(st, 0, sizeof(OpusEncoder));
    /* Create SILK encoder */
    ret = silk_Get_Encoder_Size( &silkEncSizeBytes );
    if( ret )
    	return NULL;
	silkEncSizeBytes = align(silkEncSizeBytes);
    celtEncSizeBytes = celt_encoder_get_size(channels);
    st->silk_enc_offset = align(sizeof(OpusEncoder));
    st->celt_enc_offset = st->silk_enc_offset+silkEncSizeBytes;
    silk_enc = (char*)st+st->silk_enc_offset;
    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    st->stream_channels = st->channels = channels;

    st->Fs = Fs;

    ret = silk_InitEncoder( silk_enc, &st->silk_mode );
    if( ret )
        goto failure;

    /* default SILK parameters */
    st->silk_mode.nChannelsAPI              = channels;
    st->silk_mode.nChannelsInternal         = channels;
    st->silk_mode.API_sampleRate            = st->Fs;
    st->silk_mode.maxInternalSampleRate     = 16000;
    st->silk_mode.minInternalSampleRate     = 8000;
    st->silk_mode.desiredInternalSampleRate = 16000;
    st->silk_mode.payloadSize_ms            = 20;
    st->silk_mode.bitRate                   = 25000;
    st->silk_mode.packetLossPercentage      = 0;
    st->silk_mode.complexity                = 10;
    st->silk_mode.useInBandFEC              = 0;
    st->silk_mode.useDTX                    = 0;
    st->silk_mode.useCBR                    = 0;

    /* Create CELT encoder */
	/* Initialize CELT encoder */
	celt_encoder_init(celt_enc, Fs, channels, &err);
	if (err != CELT_OK)
		goto failure;
    celt_encoder_ctl(celt_enc, CELT_SET_SIGNALLING(0));

	st->mode = MODE_HYBRID;
	st->bandwidth = BANDWIDTH_FULLBAND;
	st->use_vbr = 0;
    st->user_bitrate_bps = OPUS_BITRATE_AUTO;
	st->bitrate_bps = 3000+Fs*channels;
	st->user_mode = OPUS_MODE_AUTO;
	st->user_bandwidth = BANDWIDTH_AUTO;
	st->voice_ratio = 90;
	st->first = 1;

	st->encoder_buffer = st->Fs/100;
	st->delay_compensation = st->Fs/400;
	if (st->Fs > 16000)
		st->delay_compensation += 10;
	return st;
failure:
    free(st);
    return NULL;
}

OpusEncoder *opus_encoder_create(int Fs, int channels)
{
    char *raw_state = malloc(opus_encoder_get_size(channels));
    if (raw_state == NULL)
    	return NULL;
    return opus_encoder_init((OpusEncoder*)raw_state, Fs, channels);
}

int opus_encode(OpusEncoder *st, const short *pcm, int frame_size,
		unsigned char *data, int max_data_bytes)
{
	void *silk_enc;
	CELTEncoder *celt_enc;
    int i;
	int ret=0;
	SKP_int32 nBytes;
	ec_enc enc;
	int framerate, period;
    int silk_internal_bandwidth=-1;
    int bytes_target;
    int prefill=0;
    int start_band = 0;
    int redundancy = 0;
    int redundancy_bytes = 0;
    int celt_to_silk = 0;
    /* TODO: This is 60 only so we can handle 60ms speech/audio switching 
       it shouldn't bee too hard to reduce to 20 ms if needed */
    short pcm_buf[60*48*2];
    int nb_compr_bytes;
    int to_celt = 0;
    celt_int32 mono_rate;

    silk_enc = (char*)st+st->silk_enc_offset;
    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    if (st->user_bitrate_bps==OPUS_BITRATE_AUTO)
        st->bitrate_bps = 60*st->Fs/frame_size + st->Fs*st->channels;
    else
        st->bitrate_bps = st->user_bitrate_bps;

    /* Rate-dependent mono-stereo decision */
    if (st->mode == MODE_CELT_ONLY && st->channels == 2)
    {
        celt_int32 decision_rate;
        decision_rate = st->bitrate_bps + st->voice_ratio*st->voice_ratio;
        /* Add some hysteresis */
        if (st->stream_channels == 2)
            decision_rate += 4000;
        else
            decision_rate -= 4000;
        if (decision_rate>48000)
            st->stream_channels = 2;
        else
            st->stream_channels = 1;
    }
    /* Equivalent bit-rate for mono */
    mono_rate = st->bitrate_bps;
    if (st->stream_channels==2)
        mono_rate = (mono_rate+10000)/2;
    /* Compensate for smaller frame sizes assuming an equivalent overhead
       of 60 bits/frame */
    mono_rate -= 60*(st->Fs/frame_size - 50);

    /* Mode selection */
    if (st->user_mode==OPUS_MODE_AUTO)
    {
        celt_int32 decision_rate;
        /* SILK/CELT threshold is higher for voice than for music */
        decision_rate = mono_rate - 3*st->voice_ratio*st->voice_ratio;
        /* Hysteresis */
        if (st->prev_mode == MODE_CELT_ONLY)
            decision_rate += 4000;
        else if (st->prev_mode>0)
            decision_rate -= 4000;
        if (decision_rate>24000)
            st->mode = MODE_CELT_ONLY;
        else
            st->mode = MODE_SILK_ONLY;
    } else if (st->user_mode==OPUS_MODE_VOICE)
    {
        st->mode = MODE_SILK_ONLY;
    } else {/* OPUS_AUDIO_MODE */
        st->mode = MODE_CELT_ONLY;
    }

    /* Automatic (rate-dependent) bandwidth selection */
    if (st->mode == MODE_CELT_ONLY || st->first || st->silk_mode.allowBandwidthSwitch)
    {
    	const int *bandwidth_thresholds;
    	int bandwidth = BANDWIDTH_FULLBAND;

    	bandwidth_thresholds = st->mode == MODE_CELT_ONLY ? audio_bandwidth_thresholds : voice_bandwidth_thresholds;
    	do {
    		int threshold, hysteresis;
    		threshold = bandwidth_thresholds[2*(bandwidth-BANDWIDTH_MEDIUMBAND)];
    		hysteresis = bandwidth_thresholds[2*(bandwidth-BANDWIDTH_MEDIUMBAND)+1];
    		if (!st->first)
    		{
    			if (st->bandwidth >= bandwidth)
    				threshold -= hysteresis;
    			else
    				threshold += hysteresis;
    		}
    		if (mono_rate >= threshold)
    			break;
    	} while (--bandwidth>BANDWIDTH_NARROWBAND);
    	st->bandwidth = bandwidth;
    	/* Prevents any transition to SWB/FB until the SILK layer has fully
    	   switched to WB mode and turned the variable LP filter off */
    	if (st->mode != MODE_CELT_ONLY && !st->silk_mode.inWBmodeWithoutVariableLP && st->bandwidth > BANDWIDTH_WIDEBAND)
    		st->bandwidth = BANDWIDTH_WIDEBAND;
    }

    /* Prevents Opus from wasting bits on frequencies that are above
       the Nyquist rate of the input signal */
    if (st->Fs <= 24000 && st->bandwidth > BANDWIDTH_SUPERWIDEBAND)
    	st->bandwidth = BANDWIDTH_SUPERWIDEBAND;
    if (st->Fs <= 16000 && st->bandwidth > BANDWIDTH_WIDEBAND)
    	st->bandwidth = BANDWIDTH_WIDEBAND;
    if (st->Fs <= 12000 && st->bandwidth > BANDWIDTH_MEDIUMBAND)
    	st->bandwidth = BANDWIDTH_MEDIUMBAND;
    if (st->Fs <= 8000 && st->bandwidth > BANDWIDTH_NARROWBAND)
    	st->bandwidth = BANDWIDTH_NARROWBAND;

    if (st->user_bandwidth != BANDWIDTH_AUTO)
    	st->bandwidth = st->user_bandwidth;

    /* Prevents nonsensical configurations, i.e. modes that don't exist */
    if (frame_size < st->Fs/100 && st->mode != MODE_CELT_ONLY)
        st->mode = MODE_CELT_ONLY;
    if (frame_size > st->Fs/50 && st->mode != MODE_SILK_ONLY)
        st->mode = MODE_SILK_ONLY;
    if (st->mode == MODE_CELT_ONLY && st->bandwidth == BANDWIDTH_MEDIUMBAND)
        st->bandwidth = BANDWIDTH_WIDEBAND;
    if (st->mode == MODE_SILK_ONLY && st->bandwidth > BANDWIDTH_WIDEBAND)
        st->mode = MODE_HYBRID;
    if (st->mode == MODE_HYBRID && st->bandwidth <= BANDWIDTH_WIDEBAND)
        st->mode = MODE_SILK_ONLY;

	bytes_target = st->bitrate_bps * frame_size / (st->Fs * 8) - 1;

	data += 1;
	if (st->mode != MODE_CELT_ONLY && st->prev_mode == MODE_CELT_ONLY)
	{
		silk_EncControlStruct dummy;
		silk_InitEncoder( st->silk_enc, &dummy);
		prefill=1;
	}
	if (st->prev_mode > 0 &&
	       ((st->mode != MODE_CELT_ONLY && st->prev_mode == MODE_CELT_ONLY) ||
	        (st->mode == MODE_CELT_ONLY && st->prev_mode != MODE_CELT_ONLY)))
	{
	    redundancy = 1;
	    celt_to_silk = (st->mode != MODE_CELT_ONLY);
	    if (!celt_to_silk)
	    {
	        /* Switch to SILK/hybrid if frame size is 10 ms or more*/
	        if (frame_size >= st->Fs/100)
	        {
		        st->mode = st->prev_mode;
		        to_celt = 1;
	        } else {
	        	redundancy=0;
	        }
	    }
	}

	ec_enc_init(&enc, data, max_data_bytes-1);

	/* SILK processing */
    if (st->mode != MODE_CELT_ONLY)
    {
        st->silk_mode.bitRate = st->bitrate_bps - 8*st->Fs/frame_size;
        if( st->mode == MODE_HYBRID ) {
            st->silk_mode.bitRate /= st->stream_channels;
            if( st->bandwidth == BANDWIDTH_SUPERWIDEBAND ) {
                if( st->Fs == 100 * frame_size ) {
                    /* 24 kHz, 10 ms */
                    st->silk_mode.bitRate = ( ( st->silk_mode.bitRate + 2000 + st->use_vbr * 1000 ) * 2 ) / 3;
                } else {
                    /* 24 kHz, 20 ms */
                    st->silk_mode.bitRate = ( ( st->silk_mode.bitRate + 1000 + st->use_vbr * 1000 ) * 2 ) / 3;
                }
            } else {
                if( st->Fs == 100 * frame_size ) {
                    /* 48 kHz, 10 ms */
                    st->silk_mode.bitRate = ( st->silk_mode.bitRate + 8000 + st->use_vbr * 3000 ) / 2;
                } else {
                    /* 48 kHz, 20 ms */
                    st->silk_mode.bitRate = ( st->silk_mode.bitRate + 9000 + st->use_vbr * 1000 ) / 2;
                }
            }
            st->silk_mode.bitRate *= st->stream_channels;
            /* don't let SILK use more than 80% */
            if( st->silk_mode.bitRate > ( st->bitrate_bps - 8*st->Fs/frame_size ) * 4/5 ) {
                st->silk_mode.bitRate = ( st->bitrate_bps - 8*st->Fs/frame_size ) * 4/5;
            }
        }

        st->silk_mode.payloadSize_ms = 1000 * frame_size / st->Fs;
        st->silk_mode.nChannelsAPI = st->channels;
        st->silk_mode.nChannelsInternal = st->stream_channels;
        if (st->bandwidth == BANDWIDTH_NARROWBAND) {
        	st->silk_mode.desiredInternalSampleRate = 8000;
        } else if (st->bandwidth == BANDWIDTH_MEDIUMBAND) {
        	st->silk_mode.desiredInternalSampleRate = 12000;
        } else {
            SKP_assert( st->mode == MODE_HYBRID || st->bandwidth == BANDWIDTH_WIDEBAND );
            st->silk_mode.desiredInternalSampleRate = 16000;
        }
        if( st->mode == MODE_HYBRID ) {
            /* Don't allow bandwidth reduction at lowest bitrates in hybrid mode */
            st->silk_mode.minInternalSampleRate = 16000;
        } else {
            st->silk_mode.minInternalSampleRate = 8000;
        }
        st->silk_mode.maxInternalSampleRate = 16000;

        /* Call SILK encoder for the low band */
        nBytes = max_data_bytes-1;
        if (prefill)
        {
            int zero=0;
        	silk_Encode( silk_enc, &st->silk_mode, st->delay_buffer, st->encoder_buffer, NULL, &zero, 1 );
        }

        ret = silk_Encode( silk_enc, &st->silk_mode, pcm, frame_size, &enc, &nBytes, 0 );
        if( ret ) {
            fprintf (stderr, "SILK encode error: %d\n", ret);
            /* Handle error */
        }
        if (nBytes==0)
            return 0;
        /* Extract SILK internal bandwidth for signaling in first byte */
        if( st->mode == MODE_SILK_ONLY ) {
            if( st->silk_mode.internalSampleRate == 8000 ) {
                silk_internal_bandwidth = BANDWIDTH_NARROWBAND;
            } else if( st->silk_mode.internalSampleRate == 12000 ) {
                silk_internal_bandwidth = BANDWIDTH_MEDIUMBAND;
            } else if( st->silk_mode.internalSampleRate == 16000 ) {
                silk_internal_bandwidth = BANDWIDTH_WIDEBAND;
            }
        } else {
            SKP_assert( st->silk_mode.internalSampleRate == 16000 );
        }
    }

    /* CELT processing */
	{
	    int endband=21;

	    switch(st->bandwidth)
	    {
	    case BANDWIDTH_NARROWBAND:
	    	endband = 13;
	    	break;
	    case BANDWIDTH_WIDEBAND:
	    	endband = 17;
	    	break;
	    case BANDWIDTH_SUPERWIDEBAND:
	    	endband = 19;
	    	break;
	    case BANDWIDTH_FULLBAND:
	    	endband = 21;
	    	break;
	    }
	    celt_encoder_ctl(celt_enc, CELT_SET_END_BAND(endband));
	    celt_encoder_ctl(celt_enc, CELT_SET_CHANNELS(st->stream_channels));
	}
	if (st->mode != MODE_SILK_ONLY)
	{
        celt_encoder_ctl(celt_enc, CELT_SET_VBR(0));
        celt_encoder_ctl(celt_enc, CELT_SET_BITRATE(510000));
        if (st->prev_mode == MODE_SILK_ONLY)
        {
        	unsigned char dummy[10];
        	celt_encoder_ctl(celt_enc, CELT_RESET_STATE);
        	celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(0));
        	celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(0));
        	/* TODO: This wastes CPU a bit compared to just prefilling the buffer */
        	celt_encode(celt_enc, &st->delay_buffer[(st->encoder_buffer-st->delay_compensation-st->Fs/400)*st->channels], st->Fs/400, dummy, 10);
        } else {
        	celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(2));
        }

        if (st->mode == MODE_HYBRID)
        {
            int len;

            len = (ec_tell(&enc)+7)>>3;
            if( st->use_vbr ) {
                nb_compr_bytes = len + bytes_target - (st->silk_mode.bitRate * frame_size) / (8 * st->Fs);
            } else {
                /* check if SILK used up too much */
                nb_compr_bytes = len > bytes_target ? len : bytes_target;
            }
        } else {
            if (st->use_vbr)
            {
                celt_encoder_ctl(celt_enc, CELT_SET_VBR(1));
                celt_encoder_ctl(celt_enc, CELT_SET_VBR_CONSTRAINT(st->vbr_constraint));
                celt_encoder_ctl(celt_enc, CELT_SET_BITRATE(st->bitrate_bps));
                nb_compr_bytes = max_data_bytes-1;
            } else {
                nb_compr_bytes = bytes_target;
            }
        }

        ec_enc_shrink(&enc, nb_compr_bytes);
	} else {
	    nb_compr_bytes = 0;
	}

    for (i=0;i<IMIN(frame_size, st->delay_compensation)*st->channels;i++)
        pcm_buf[i] = st->delay_buffer[(st->encoder_buffer-st->delay_compensation)*st->channels+i];
    for (;i<frame_size*st->channels;i++)
        pcm_buf[i] = pcm[i-st->delay_compensation*st->channels];
    if (st->mode != MODE_CELT_ONLY)
    {
        /* Check if we have a redundant 0-8 kHz band */
        ec_enc_bit_logp(&enc, redundancy, 12);
        if (redundancy)
        {
            redundancy_bytes = st->stream_channels*st->bitrate_bps/1600;
            ec_enc_bit_logp(&enc, celt_to_silk, 1);
            if (st->mode == MODE_HYBRID)
            	ec_enc_uint(&enc, redundancy_bytes-2, 256);
        }
        start_band = 17;
    }

    if (st->mode == MODE_SILK_ONLY)
    {
        ret = (ec_tell(&enc)+7)>>3;
        ec_enc_done(&enc);
        nb_compr_bytes = ret;
    }

    /* 5 ms redundant frame for CELT->SILK */
    if (redundancy && celt_to_silk)
    {
        celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(0));
        celt_encoder_ctl(celt_enc, CELT_SET_VBR(0));
        celt_encode(celt_enc, pcm_buf, st->Fs/200, data+nb_compr_bytes, redundancy_bytes);
        celt_encoder_ctl(celt_enc, CELT_RESET_STATE);
    }

    celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(start_band));

    if (st->mode != MODE_SILK_ONLY)
	{
	    /* Encode high band with CELT */
	    ret = celt_encode_with_ec(celt_enc, pcm_buf, frame_size, NULL, nb_compr_bytes, &enc);
	}

    /* 5 ms redundant frame for SILK->CELT */
    if (redundancy && !celt_to_silk)
    {
        int N2, N4;
        N2 = st->Fs/200;
        N4 = st->Fs/400;

        celt_encoder_ctl(celt_enc, CELT_RESET_STATE);
        celt_encoder_ctl(celt_enc, CELT_SET_START_BAND(0));
        celt_encoder_ctl(celt_enc, CELT_SET_PREDICTION(0));

        /* TODO: We could speed up prefilling here */
        celt_encode(celt_enc, pcm_buf+st->channels*(frame_size-N2-N4), N4, data+nb_compr_bytes, redundancy_bytes);

        celt_encode(celt_enc, pcm_buf+st->channels*(frame_size-N2), N2, data+nb_compr_bytes, redundancy_bytes);
    }


    if (frame_size>st->encoder_buffer)
    {
    	for (i=0;i<st->encoder_buffer*st->channels;i++)
    		st->delay_buffer[i] = pcm[(frame_size-st->encoder_buffer)*st->channels+i];
    } else {
    	int tmp = st->encoder_buffer-frame_size;
    	for (i=0;i<tmp*st->channels;i++)
    		st->delay_buffer[i] = st->delay_buffer[i+frame_size*st->channels];
    	for (i=0;i<frame_size*st->channels;i++)
    		st->delay_buffer[tmp*st->channels+i] = pcm[i];
    }

	/* Signalling the mode in the first byte */
	data--;
	framerate = st->Fs/frame_size;
	period = 0;
	while (framerate < 400)
	{
	    framerate <<= 1;
	    period++;
	}
    if (st->mode == MODE_SILK_ONLY)
    {
        data[0] = (silk_internal_bandwidth-BANDWIDTH_NARROWBAND)<<5;
        data[0] |= (period-2)<<3;
    } else if (st->mode == MODE_CELT_ONLY)
    {
        int tmp = st->bandwidth-BANDWIDTH_MEDIUMBAND;
        if (tmp < 0)
            tmp = 0;
        data[0] = 0x80;
        data[0] |= tmp << 5;
        data[0] |= period<<3;
    } else /* Hybrid */
    {
        data[0] = 0x60;
        data[0] |= (st->bandwidth-BANDWIDTH_SUPERWIDEBAND)<<4;
        data[0] |= (period-2)<<3;
    }
    data[0] |= (st->stream_channels==2)<<2;
    /*printf ("%x\n", (int)data[0]);*/

#if OPUS_TEST_RANGE_CODER_STATE
    st->rangeFinal = enc.rng;
#endif
    if (to_celt)
        st->prev_mode = MODE_CELT_ONLY;
    else
        st->prev_mode = st->mode;
    st->first = 0;
    return ret+1+redundancy_bytes;
}

int opus_encoder_ctl(OpusEncoder *st, int request, ...)
{
	CELTEncoder *celt_enc;
    va_list ap;

    va_start(ap, request);

    celt_enc = (CELTEncoder*)((char*)st+st->celt_enc_offset);

    switch (request)
    {
        case OPUS_SET_MODE_REQUEST:
        {
            int value = va_arg(ap, int);
            st->user_mode = value;
        }
        break;
        case OPUS_GET_MODE_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->mode;
        }
        break;
        case OPUS_SET_BITRATE_REQUEST:
        {
            int value = va_arg(ap, int);
            if (value != OPUS_BITRATE_AUTO)
            {
                if (value <= 0)
                    goto bad_arg;
                else if (value <= 500)
                    value = 500;
            }
            st->user_bitrate_bps = value;
        }
        break;
        case OPUS_GET_BITRATE_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->bitrate_bps;
        }
        break;
        case OPUS_SET_FORCE_MONO_REQUEST:
        {
            int value = va_arg(ap, int);
            st->stream_channels = value ? 1 : st->channels;
        }
        break;
        case OPUS_GET_FORCE_MONO_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->stream_channels == 1 && st->channels == 2;
        }
        break;
        case OPUS_SET_BANDWIDTH_REQUEST:
        {
            int value = va_arg(ap, int);
            if (value < BANDWIDTH_AUTO || value > BANDWIDTH_FULLBAND)
            	return OPUS_BAD_ARG;
            st->user_bandwidth = value;
            if (st->user_bandwidth == BANDWIDTH_NARROWBAND) {
                st->silk_mode.maxInternalSampleRate = 8000;
            } else if (st->bandwidth == BANDWIDTH_MEDIUMBAND) {
                st->silk_mode.maxInternalSampleRate = 12000;
            } else {
                st->silk_mode.maxInternalSampleRate = 16000;
            }
        }
        break;
        case OPUS_GET_BANDWIDTH_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->bandwidth;
        }
        break;
        case OPUS_SET_DTX_FLAG_REQUEST:
        {
            int value = va_arg(ap, int);
            st->silk_mode.useDTX = value;
        }
        break;
        case OPUS_GET_DTX_FLAG_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->silk_mode.useDTX;
        }
        break;
        case OPUS_SET_COMPLEXITY_REQUEST:
        {
            int value = va_arg(ap, int);
            st->silk_mode.complexity = value;
            celt_encoder_ctl(celt_enc, CELT_SET_COMPLEXITY(value));
        }
        break;
        case OPUS_GET_COMPLEXITY_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->silk_mode.complexity;
        }
        break;
        case OPUS_SET_INBAND_FEC_FLAG_REQUEST:
        {
            int value = va_arg(ap, int);
            st->silk_mode.useInBandFEC = value;
        }
        break;
        case OPUS_GET_INBAND_FEC_FLAG_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->silk_mode.useInBandFEC;
        }
        break;
        case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
        {
            int value = va_arg(ap, int);
            if (value < 0 || value > 100)
                return OPUS_BAD_ARG;
            st->silk_mode.packetLossPercentage = value;
            celt_encoder_ctl(celt_enc, CELT_SET_LOSS_PERC(value));
        }
        break;
        case OPUS_GET_PACKET_LOSS_PERC_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->silk_mode.packetLossPercentage;
        }
        break;
        case OPUS_SET_VBR_FLAG_REQUEST:
        {
            int value = va_arg(ap, int);
            st->use_vbr = value;
            st->silk_mode.useCBR = 1-value;
        }
        break;
        case OPUS_GET_VBR_FLAG_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->use_vbr;
        }
        break;
        case OPUS_SET_VOICE_RATIO_REQUEST:
        {
            int value = va_arg(ap, int);
            if (value>100 || value<0)
                goto bad_arg;
            st->voice_ratio = value;
        }
        break;
        case OPUS_GET_VOICE_RATIO_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->voice_ratio;
        }
        break;
        case OPUS_SET_VBR_CONSTRAINT_REQUEST:
        {
            int value = va_arg(ap, int);
            st->vbr_constraint = value;
        }
        break;
        case OPUS_GET_VBR_CONSTRAINT_REQUEST:
        {
            int *value = va_arg(ap, int*);
            *value = st->vbr_constraint;
        }
        break;
        default:
            fprintf(stderr, "unknown opus_encoder_ctl() request: %d", request);
            break;
    }
    va_end(ap);
    return OPUS_OK;
bad_arg:
    va_end(ap);
    return OPUS_BAD_ARG;
}

void opus_encoder_destroy(OpusEncoder *st)
{
	free(st);
}

#if OPUS_TEST_RANGE_CODER_STATE
int opus_encoder_get_final_range(OpusEncoder *st)
{
    return st->rangeFinal;
}
#endif
