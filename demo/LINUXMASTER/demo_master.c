#include <stdio.h>

#include "mbmaster.h"
#include "port.h"
#include "mbport.h"

void vMBReadInputRegCallback ( const UCHAR *cpucBuffer, USHORT usRegCnt ) {
    const UCHAR *pucBufferCur = NULL;
    unsigned short val;

    printf("Reading %d input registers:\n", usRegCnt);
    for (pucBufferCur = cpucBuffer; (pucBufferCur - cpucBuffer) != usRegCnt * 2;) {
        val = *pucBufferCur++ << 8;
        val |= *pucBufferCur++ & 0xFF;
        printf(" %hu\n", val);
    }
}

void vMBReadHoldingRegCallback ( const UCHAR *cpucBuffer, USHORT usRegCnt ) {
    const UCHAR *pucBufferCur = NULL;
    unsigned short val;

    printf("Reading %d holding registers:\n", usRegCnt);
    for (pucBufferCur = cpucBuffer; (pucBufferCur - cpucBuffer) != usRegCnt * 2;) {
        val = *pucBufferCur++ << 8;
        val |= *pucBufferCur++ & 0xFF;
        printf(" %hu\n", val);
    }
}

int main(int argc, char *argv[]) {
     unsigned short v_array[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    
    if (eMBInit(MB_ASCII, 1, 38400, MB_PAR_EVEN) != MB_ENOERR) return 2;
    if (eMBEnable() != MB_ENOERR) return 2;
    for (;;) {
        if (eMBReadInputReg(0x0A, 1000, 4) != MB_ENOERR) {
            continue;
        }
        eMBPoll();

        if (eMBWriteMultRegister(0x0A, 2000, 8, v_array) != MB_ENOERR) {
            continue;
        }
        if (eMBReadOutputReg(0x0A, 2000, 8) != MB_ENOERR) {
            continue;
        }
        eMBPoll();

        if (eMBWriteRegister(0x0A, 2000, 321) != MB_ENOERR) {
            continue;
        }
        if (eMBReadOutputReg(0x0A, 2000, 1) != MB_ENOERR) {
            continue;
        }
        eMBPoll();
    }
    return 0;
}
