/******************************************************************************
Filename:     rfSetRadioFrequency.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
void halRfSetRadioFrequency(UINT32 frequency)
{

   frequency /= 1000;
   frequency -= 2048;
   FSCTRLL = (BYTE) frequency;
   FSCTRLH = ((FSCTRLH & ~0x03) | (BYTE)((frequency >> 8) & 0x03));
   return;
}
