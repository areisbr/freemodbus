#include <stdio.h>

#include "mbmaster.h"
#include "port.h"
#include "mbport.h"

void vMBReadInputRegCallback ( const UCHAR *cpucBuffer, USHORT usRegCnt ) {
    const UCHAR *pucBufferCur = NULL;
    unsigned short val;

    printf("Reading %d registers:\n", usRegCnt);
    for (pucBufferCur = cpucBuffer; (pucBufferCur - cpucBuffer) != usRegCnt * 2;) {
        val = *pucBufferCur++ << 8;
        val |= *pucBufferCur++ & 0xFF;
        printf(" %hu\n", val);
    }
}

int main(int argc, char *argv[]) {
    
    if (eMBInit(MB_ASCII, 1, 38400, MB_PAR_EVEN) != MB_ENOERR) return 2;
    if (eMBEnable() != MB_ENOERR) return 2;
    for (;;) {
        if (eMBReadInputReg(0x0A, 1000, 4) != MB_ENOERR) {
            continue;
        }
        eMBPoll();
    }
    return 0;
}
