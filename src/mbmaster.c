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
 * File: $Id: mb.c,v 1.28 2010/06/06 13:54:40 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbmaster.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbmasterfunc.h"

#include "mbport.h"
#if MB_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Static variables ---------------------------------*/

static UCHAR    ucMBAddress;
static eMBMode  eMBCurrentMode;

static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 */
static peMBFrameSend peMBFrameSendCur;
static pvMBFrameStart pvMBFrameStartCur;
static pvMBFrameStop pvMBFrameStopCur;
static peMBFrameReceive peMBFrameReceiveCur;
static pvMBFrameClose pvMBFrameCloseCur;
static pvMBFrameGetBuffer pvMBFrameGetBufferCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 */
BOOL( *pxMBFrameCBByteReceived ) ( void );
BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );
BOOL( *pxMBPortCBTimerExpired ) ( void );

BOOL( *pxMBFrameCBReceiveFSMCur ) ( void );
BOOL( *pxMBFrameCBTransmitFSMCur ) ( void );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveIDRespHandler},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegisterRespHandler},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegisterRespHandler},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegisterRespHandler},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegisterRespHandler},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegisterRespHandler},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoilsRespHandler},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoilRespHandler},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoilsRespHandler},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputsRespHandler},
#endif
};

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBInit( eMBMode eMode, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ucMBAddress = 0;

    switch ( eMode )
    {
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        pvMBFrameStartCur = eMBRTUStart;
        pvMBFrameStopCur = eMBRTUStop;
        peMBFrameSendCur = eMBRTUSend;
        peMBFrameReceiveCur = eMBRTUReceive;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
        pvMBFrameGetBufferCur = vMBRTUGetBuffer;
        pxMBFrameCBByteReceived = xMBRTUReceiveFSM;
        pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM;
        pxMBPortCBTimerExpired = xMBRTUTimerT35Expired;

        eStatus = eMBRTUInit( ucMBAddress, ucPort, ulBaudRate, eParity );
        break;
#endif
#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        pvMBFrameStartCur = eMBASCIIStart;
        pvMBFrameStopCur = eMBASCIIStop;
        peMBFrameSendCur = eMBASCIISend;
        peMBFrameReceiveCur = eMBASCIIReceive;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
        pvMBFrameGetBufferCur = vMBASCIIGetBuffer;
        pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
        pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
        pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

        eStatus = eMBASCIIInit( ucMBAddress, ucPort, ulBaudRate, eParity );
        break;
#endif
    default:
        eStatus = MB_EINVAL;
    }

    if( eStatus == MB_ENOERR )
    {
        if( !xMBPortEventInit(  ) )
        {
            /* port dependent event module initalization failed. */
            eStatus = MB_EPORTERR;
        }
        else
        {
            eMBCurrentMode = eMode;
            eMBState = STATE_DISABLED;
        }
    }
    return eStatus;
}

#if MB_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit( USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( ucTCPPort ) ) != MB_ENOERR )
    {
        eMBState = STATE_DISABLED;
    }
    else if( !xMBPortEventInit(  ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        pvMBFrameStartCur = eMBTCPStart;
        pvMBFrameStopCur = eMBTCPStop;
        peMBFrameReceiveCur = eMBTCPReceive;
        peMBFrameSendCur = eMBTCPSend;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        pvMBFrameGetBufferCur = eMBTCPGetBuffer;
        ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        eMBCurrentMode = MB_TCP;
        eMBState = STATE_DISABLED;
    }
    return eStatus;
}
#endif


eMBErrorCode
eMBClose( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        if( pvMBFrameCloseCur != NULL )
        {
            pvMBFrameCloseCur(  );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBEnable( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    if( eMBState == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        pvMBFrameStartCur(  );
        if( xMBPortEventGet( &eEvent ) == TRUE )
        {
            if( eEvent == EV_READY ) 
            {
                eMBState = STATE_ENABLED;
            }
            else
            {
                eStatus = MB_EILLSTATE;
            }
        }
        else
        {
            eStatus = MB_EILLSTATE;
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBDisable( void )
{
    eMBErrorCode    eStatus;

    if( eMBState == STATE_ENABLED )
    {
        pvMBFrameStopCur(  );
        eMBState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( eMBState == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBPoll( void )
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static USHORT   usLength;
    static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    /* Check if the protocol stack is ready. */
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( xMBPortEventGet( &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = peMBFrameReceiveCur( &ucRcvAddress, &ucMBFrame, &usLength );
            if( eStatus == MB_ENOERR )
            {
                if( ( ucRcvAddress == ucMBAddress ) )
                {
                    ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
                    eException = MB_EX_ILLEGAL_FUNCTION;
                    for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
                    {
                        /* No more function handlers registered. Abort. */
                        if( xFuncHandlers[i].ucFunctionCode == 0 )
                        {
                            break;
                        }
                        else if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                        {
                            eException = xFuncHandlers[i].pxHandler( ucMBFrame, &usLength );
                            break;
                        }
                    }
                    if( eException != MB_EX_NONE)
                    {
                        eStatus = MB_EIO;
                    }
                }
            }
            break;

        case EV_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}

eMBErrorCode
eMBReadInputReg( UCHAR ucId, USHORT usStartAddr, USHORT usLen ) 
{
    eMBErrorCode eStatus = MB_ENOERR;
    UCHAR *pucFrame = NULL, *pucFrameCur = NULL;
    eMBEventType eEvent;
    ucMBAddress = ucId;

    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    pvMBFrameGetBufferCur( & pucFrame );
    if( pucFrame == NULL )
    {
        return MB_EILLSTATE;
    }

    if (usStartAddr <= 0 || usStartAddr > 10000) 
    {
        return MB_EINVAL;
    }

    pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
    *pucFrameCur++ = MB_FUNC_READ_INPUT_REGISTER;
    usStartAddr--;
    *pucFrameCur++ = ( UCHAR ) ( usStartAddr >> 8 );
    *pucFrameCur++ = ( UCHAR ) ( usStartAddr & 0xFF );
    *pucFrameCur++ = ( UCHAR ) ( usLen >> 8 );
    *pucFrameCur++ = ( UCHAR ) ( usLen & 0xFF );
    if( peMBFrameSendCur( ucId, pucFrame, pucFrameCur - pucFrame ) != MB_ENOERR )
    {
        return MB_EIO;
    }

    eStatus = xMBPortSerialPoll( ) ? MB_ENOERR : MB_EIO ;
    eMBPoll( );

    return eStatus;
}

eMBErrorCode
eMBReadOutputReg( UCHAR ucId, USHORT usStartAddr, USHORT usLen )
{
    UCHAR *pucFrame = NULL, *pucFrameCur = NULL;
    eMBEventType eEvent;
    ucMBAddress = ucId;

    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    pvMBFrameGetBufferCur( & pucFrame );
    if( pucFrame == NULL )
    {
        return MB_EILLSTATE;
    }

    pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
    *pucFrameCur++ = MB_FUNC_READ_HOLDING_REGISTER;
    *pucFrameCur++ = ( UCHAR ) ( usStartAddr >> 8 );
    *pucFrameCur++ = ( UCHAR ) ( usStartAddr & 0xFF );
    *pucFrameCur++ = ( UCHAR ) ( usLen >> 8 );
    *pucFrameCur++ = ( UCHAR ) ( usLen & 0xFF );
    if( peMBFrameSendCur( ucId, pucFrame, pucFrameCur - pucFrame ) != MB_ENOERR )
    {
        return MB_EIO;
    }

    return eMBPoll();
}

eMBErrorCode
eMBWriteRegister( UCHAR ucId, USHORT usAddr, const USHORT cusData )
{
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }
    return MB_ENOERR;
}

eMBErrorCode
eMBWriteMultRegister( UCHAR ucId, USHORT usStartAddr, USHORT usLen, const USHORT *cusData )
{
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }
    return MB_ENOERR;
}


