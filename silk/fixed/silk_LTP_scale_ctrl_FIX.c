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

#include "silk_main_FIX.h"

void silk_LTP_scale_ctrl_FIX(
    silk_encoder_state_FIX      *psEnc,     /* I/O  encoder state FIX                           */
    silk_encoder_control_FIX    *psEncCtrl  /* I/O  encoder control FIX                         */
)
{
    SKP_int round_loss;

    /* 1st order high-pass filter */
    psEnc->HPLTPredCodGain_Q7 = SKP_max_int( psEncCtrl->LTPredCodGain_Q7 - SKP_RSHIFT( psEnc->prevLTPredCodGain_Q7, 1 ), 0 ) 
        + SKP_RSHIFT( psEnc->HPLTPredCodGain_Q7, 1 );
    psEnc->prevLTPredCodGain_Q7 = psEncCtrl->LTPredCodGain_Q7;

    /* Only scale if first frame in packet */
    if( psEnc->sCmn.nFramesEncoded == 0 ) {
        round_loss = psEnc->sCmn.PacketLoss_perc + psEnc->sCmn.nFramesPerPacket - 1;
        psEnc->sCmn.indices.LTP_scaleIndex = (SKP_int8)SKP_LIMIT( 
            SKP_SMULWB( SKP_SMULBB( round_loss, psEnc->HPLTPredCodGain_Q7 ), SILK_FIX_CONST( 0.1, 9 ) ), 0, 2 );
    } else {
        /* Default is minimum scaling */
        psEnc->sCmn.indices.LTP_scaleIndex = 0;
    }
    psEncCtrl->LTP_scale_Q14 = silk_LTPScales_table_Q14[ psEnc->sCmn.indices.LTP_scaleIndex ];
}
