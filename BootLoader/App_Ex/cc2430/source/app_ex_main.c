/******************************************************************************
Filename:     app_ex_main.c
Target:       cc2430
Revised:      16/12-2005  cdwxl
Revision:     1.0

Description:
    Application example
******************************************************************************/

#include "app_ex_main.h"
#include "app_ex_util.h"

/******************************************************************************
* @fn  initUART
*
* @brief
*      Initializes components for the UART application example.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void initUART(void)
{
   // Setup for UART0
   IO_PER_LOC_UART0_AT_PORT0_PIN2345();

   SET_MAIN_CLOCK_SOURCE(CRYSTAL);

   UART_SETUP(0, 57600, HIGH_STOP);

   UTX0IF = 1;

}

/******************************************************************************
* @fn  main
*
* @brief
*      Main function of application example.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void main(void)
{
   //SET_MAIN_CLOCK_SOURCE(CRYSTAL);
   //SET_32KHZ_CLOCK_SOURCE(CRYSTAL);
   //RFPWR = 0x04;
   //while(RFPWR & 0x10);
   //initUART();
   //IO_DIR_PORT_PIN(0,5,IO_OUT);
   //IO_DIR_PORT_PIN(1,3,IO_OUT);
   //IO_DIR_PORT_PIN(1,2,IO_IN);
   //P0_5 = 1;
   //P1_3 = 0;
   rf_test_main();
}
