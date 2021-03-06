/******************************************************************************
Filename:     timer4AdmClear.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/

#include "cul.h"


extern volatile TIMER4_TABLE_ENTRY timer4Table[TIMER_ADM_TABLE_LENGTH];

//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
void culTimer4AdmClear(BYTE entry){
   BYTE status;

   // Storing the interrupt enable register, and turning off interrupts
   status = IEN0;
   INT_GLOBAL_ENABLE(INT_OFF);

   // Setting up the table.
   timer4Table[entry].timeout = 0;
   timer4Table[entry].counter = 0;
   timer4Table[entry].callBackFunction = NULL;

   // Restoring the interrupt enable status.
   IEN0 = status;
   return;
} // ends culTimer4AdmClear(...)
