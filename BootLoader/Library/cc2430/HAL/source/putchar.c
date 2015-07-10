/******************************************************************************
Filename:     putchar.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
int putchar (int c)  {
   if (c == '\n')  {
      while (!UTX0IF);
      UTX0IF = 0;
      U0DBUF = 0x0d;       /* output CR  */
   }

   while (!UTX0IF);
   UTX0IF = 0;
   return (U0DBUF = c);
}

