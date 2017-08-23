/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mbfuncinput.c,v 1.10 2007/09/12 10:15:56 wolti Exp $
 */
#include <stdio.h>

#include "port.h"

#include "mbmaster.h"
#include "mbframe.h"
#include "mbconfig.h"
#include "mbproto.h"

#define MB_PDU_FUNC_READ_DATA_OFF            ( MB_PDU_DATA_OFF + 1 )
#define MB_PDU_FUNC_READ_REGCNT_OFF          ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_READ_REGCNT_MAX          ( 0x007D )
#define MB_READ_INPUT_REG_EXCEPTION          ( 0x84 )

#if MB_FUNC_READ_INPUT_ENABLED > 0

eMBException
eMBFuncReadInputRegisterRespHandler( UCHAR * pucFrame, USHORT * usLen )
{
    USHORT          usRegCnt;
    eMBException    eExStatus = MB_EX_NONE;

    usRegCnt = ( USHORT ) ( pucFrame[MB_PDU_FUNC_READ_REGCNT_OFF] / 2 );

    if( ( usRegCnt >= 1 ) && ( usRegCnt < MB_PDU_FUNC_READ_REGCNT_MAX ) )
    {
        if ( pucFrame[MB_PDU_FUNC_OFF] != MB_READ_INPUT_REG_EXCEPTION )
        {
            vMBReadInputRegCallback ( &pucFrame[MB_PDU_FUNC_READ_DATA_OFF], usRegCnt );
        }
        else
        {
            eExStatus = pucFrame[MB_PDU_FUNC_OFF + 1];
        }
    }
    else
    {
        eExStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return eExStatus;
}

#endif
