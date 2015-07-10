/******************************************************************************
Filename:     initRandomGenerator.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include"hal.h"


#ifndef ADCTSTH
// if ADCTSTH not defined in ioCC2430.h, define it
#define  ADCTSTH ((unsigned char volatile __xdata *) 0)[0xDF3A]
#endif
//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
void halInitRandomGenerator(void)
{
   BYTE i;

   //turning on power to analog part of radio
   RFPWR = 0x04;

   //waiting for voltage regulator.
   while((RFPWR & 0x10)){}

   //Turning on 32 MHz crystal oscillator
   SET_MAIN_CLOCK_SOURCE(CRYSTAL);

   // Turning on receiver to get output from IF-ADC.
   ISRXON;
   halWait(1);

   ENABLE_RANDOM_GENERATOR();

   for(i = 0 ; i < 32 ; i++)
   {
      RNDH = ADCTSTH;
      CLOCK_RANDOM_GENERATOR();
   }

   return;
}

