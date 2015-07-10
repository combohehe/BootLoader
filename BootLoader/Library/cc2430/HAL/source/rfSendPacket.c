/******************************************************************************
Filename:     rfSendPacket.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"


//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
BYTE halRfSendPacket(BYTE* pData, BYTE length){
    BYTE i = 0x00;

    if((length > 125) || (length == 0))
    {
      return 0;
    }

    ISFLUSHTX; // Making sure that the TX FIFO is empty.

    RFIF &= ~IRQ_TXDONE;

    RFD = length + 2;

    // Inserting data
    for(i=0;i<length;i++){
        RFD = pData[i];
    }

    ISTXON; // Sending

   // Waitting for transmission to finish
   while(!(RFIF & IRQ_TXDONE))
   {;}

   RFIF &= ~IRQ_TXDONE;
   return i;
}

