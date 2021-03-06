/******************************************************************************
Filename:     getkey.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
#include "hal.h"

char getkey ()
{
   char c;
   // Turning on reception
   U0CSR |= UART_ENABLE_RECEIVE;

   while (!URX0IF);
   c = U0DBUF;
   URX0IF = FALSE;

   return c;
}
