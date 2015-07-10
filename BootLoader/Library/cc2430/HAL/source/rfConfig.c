/******************************************************************************
Filename:     rfConfig.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"


//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
BOOL halRfConfig(UINT32 frequency)
{
   BOOL status;

   //Turning on crystal oscillator
   //SET_MAIN_CLOCK_SOURCE(CRYSTAL);

   // Setting the frequency
   halRfSetRadioFrequency(frequency);

   // Checking that the entered frequency is valid
   if (frequency > 2047000)
   {
      //turning on power to analog part of radio and waiting for voltage regulator.
      RFPWR = 0x04;
      while((RFPWR & 0x10)){}
      // Turning off Address Decoding
      MDMCTRL0H &= ~ADR_DECODE;

      // Setting for AUTO CRC
      MDMCTRL0L |= AUTO_CRC;

      // Turning on AUTO_TX2RX
      FSMTC1 = ((FSMTC1 & (~AUTO_TX2RX_OFF & ~RX2RX_TIME_OFF))  | ACCEPT_ACKPKT);

      // Turning off abortRxOnSrxon.
      FSMTC1 &= ~0x20;

      status = TRUE;
   }
   else {
      status = FALSE;
   }

   return status;
}
