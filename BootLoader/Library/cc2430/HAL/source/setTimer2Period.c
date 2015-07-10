/******************************************************************************
Filename:     setTimer2Period.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
BOOL halSetTimer2Period(BYTE mode, DWORD period){

   if(mode&TIMER2_MAC_TIMER){
      T2CAPHPH = 0x28;  // setting for 320 u-second periods as specified by 802.15.4
      T2CAPLPL = 0x00;  // (0x2800) / 32 = 320 u-seconds
   }
   else {
       T2CAPHPH = 0x7D; // Setting timer to have 1 m-second period
       T2CAPLPL = 0x00; // (0x7D00) / 32 = 1000 u-seconds
   }

   if(period){
       if(period&0xFFF00000) {return 0;}// Setting the number of periods (timer overflows) to generate
       T2PEROF0 = (BYTE) period;        // an interrupt.
       period = (period >> 8);
       T2PEROF1 = (BYTE) period;
       period = ((period >> 8)&0x0F);
       T2PEROF2 = ( T2PEROF2&~0x0F | (BYTE)period );
   }
   return 1;
}


