/******************************************************************************
Filename:     rfReceivePacket.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"

BYTE volatile length;

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
BYTE halRfReceivePacket(BYTE* pData, BYTE* pRssi, BYTE* pLqi, BYTE timeOut){
   BYTE i = 0x00;

   ISFLUSHRX;   // Making sure that the rX FIFO is empty.
   ISFLUSHRX;   // Issuing the command twice to reset the SFD.

   RFIF &= ~IRQ_FIFOP;

   // Turning on RX
   ISRXON;


   while(!(RFIF & IRQ_FIFOP))
   {
      if(timeOut)
      {
         //temp_count= 4;
         halWait(1);
         //temp_count= 0;
         timeOut--;
      }
      else
      {
         return 0;
      }
   }


   length = (RFD & 0x7F);

   // Storing packet
   for(i=0; i<(length-2); i++){
      pData[i] = RFD;
   }

    *pRssi = RFD;
    *pLqi = RFD;

   // Last two bytes contain RSSI level and Correlation value in addition to CRC OK.
   // Checking that the CRC value is OK
   if(*pLqi & 0x80){
      return i;
   }
   else {
      return 0;
   }
}



