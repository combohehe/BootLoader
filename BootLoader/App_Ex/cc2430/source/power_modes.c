/******************************************************************************
Filename:     power_modes.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0

Description:
    Demonstrates how to enter and exit the different power modes.

******************************************************************************/

#include "app_ex.h"

void addToSleepTimer(UINT16 sec);
void initPowerModes();
void main(void);

/******************************************************************************
* @fn  initPowerModes
*
* @brief
*     Initializes components used for the power modes application example.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void initPowerModes(void)
{
   initLcd();

   INIT_GLED();
   INIT_YLED();

   INIT_BUTTON();
   INIT_JOYSTICK_PUSH();
}


/******************************************************************************
* @fn  power_modes_main
*
* @brief
*     Main function.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
#ifdef COMPLETE_APPLICATION
void power_modes_main(void){
#else
void main(void){
#endif

   initPowerModes();

   lcdUpdate((char*)"Powermode 0", (char*)"");
   GLED = LED_ON;
   waitVisible(TRUE);


   //Powermode 1
   GLED = LED_OFF;
   lcdUpdate((char*)"Powermode 1", (char*)"(wait 1 sec)");

   addToSleepTimer(1);
   INT_GLOBAL_ENABLE(TRUE);
   INT_ENABLE(INUM_ST, INT_ON);
   SET_POWER_MODE(1);


   lcdUpdate((char*)"Powermode 0", (char*)"");
   GLED = LED_ON;
   waitVisible(TRUE);

   //Powermode 2
   GLED = LED_OFF;
   YLED = LED_ON;
   lcdUpdate((char*)"Powermode 2", (char*)"(wait 1 sec)");

   addToSleepTimer(1);
   INT_GLOBAL_ENABLE(TRUE);
   INT_ENABLE(INUM_ST, INT_ON);
   SET_POWER_MODE(2);


   lcdUpdate((char*)"Powermode 0", (char*)"");
   GLED = LED_ON;
   YLED = LED_OFF;
   waitVisible(TRUE);

   //Powermode 3
   GLED = LED_OFF;

   lcdUpdate((char*)"Powermode 3", (char*)"(S1 for wakeup)");

   INT_GLOBAL_ENABLE(TRUE);
   INT_ENABLE(INUM_ST, INT_OFF);
   ADC_DISABLE_CHANNEL(1);
   P0DIR &= ~0x02;
   P0IFG &= ~0x02; // clear interrupt flag P0_1
   PICTL |= 0x09;  // enable interrupt P0, port 0 to 3
   INT_SETFLAG(INUM_P0INT, INT_CLR);
   INT_ENABLE(INUM_P0INT, INT_ON);


   SET_MAIN_CLOCK_SOURCE(RC);
   while (!HIGH_FREQUENCY_RC_OSC_STABLE);

   SET_POWER_MODE(3);

   lcdUpdate((char*)"Powermode 0", (char*)"(LEFT to exit)");

   haltApplicationWithLED();
   return;
}


/******************************************************************************
* @fn  addToSleepTimer
*
* @brief
*     Function for updating sleep timer.
*
* Parameters:
*
* @param  UINT16 sec
*         Time (in seconds) to add to sleep timer
*
* @return void
*
******************************************************************************/
void addToSleepTimer(UINT16 sec)
{
   UINT32 sleepTimer = 0;

   sleepTimer |= ST0;
   sleepTimer |= (UINT32)ST1 <<  8;
   sleepTimer |= (UINT32)ST2 << 16;

   sleepTimer += ((UINT32)sec * (UINT32)32768);

   ST2 = (UINT8)(sleepTimer >> 16);
   ST1 = (UINT8)(sleepTimer >> 8);
   ST0 = (UINT8) sleepTimer;
}


/******************************************************************************
* @fn  powermodes_P0_IRQ
*
* @brief
*     Interrupt handler for P0.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
#ifdef COMPLETE_APPLICATION
void powermodes_P0_IRQ(void){
#else
#pragma vector=P0INT_VECTOR
__interrupt void powermodes_P0_IRQ(void){
#endif
   // waiting until the button is released...
   while(!P0_1);
   //halWait(0x50);

   P0IFG &= ~0x02; // clear interrupt flag P0_1
   INT_SETFLAG(INUM_P0INT, INT_CLR);
}


/******************************************************************************
* @fn  powermodes_ST_IRQ
*
* @brief
*     Interrupt handler for Sleep Timer.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
#ifdef COMPLETE_APPLICATION
void powermodes_ST_IRQ(void){
#else
#pragma vector=ST_VECTOR
__interrupt void powermodes_ST_IRQ(void){
#endif
   INT_SETFLAG(INUM_ST, INT_CLR);
}


/******************************************************************************
* @fn  power_init
*
* @brief
*      Initializes the power modes application example.
*
* Parameters:
*
* @param  APPLICATION *a
*         Main application
*
* @return void
*
******************************************************************************/
#ifdef COMPLETE_APPLICATION
void power_init(APPLICATION *a)
{
   a->menuText = (char*)"Power";
   a->description = (char*)"Modes";
   a->main_func = power_modes_main;
   a->interrupts[INUM_P0INT] = powermodes_P0_IRQ;
   a->interrupts[INUM_ST] = powermodes_ST_IRQ;
}
#endif





