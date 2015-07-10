/******************************************************************************
Filename:     app_ex_interrupt.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0

Description:
    Sets up interrupt handlers for all interrupts in the application example.

******************************************************************************/

#include "app_ex_main.h"
#include "hal.h"

__interrupt void RFERR_IRQ(void);
__interrupt void ADC_IRQ(void);
__interrupt void URX0_IRQ(void);
__interrupt void URX1_IRQ(void);
__interrupt void ENC_IRQ(void);
__interrupt void ST_IRQ(void);
__interrupt void P2INT_IRQ(void);
__interrupt void UTX0_IRQ(void);
__interrupt void T1_IRQ(void);
__interrupt void T2_IRQ(void);
__interrupt void T3_IRQ(void);
__interrupt void P0INT_IRQ(void);
__interrupt void UTX1_IRQ(void);
__interrupt void P1INT_IRQ(void);
__interrupt void WDT_IRQ(void);


void dummyInterrupt(void){
}

void (*interrupts[NBR_OF_INTERRUPTS])(void);


#pragma vector=ADC_VECTOR
__interrupt void ADC_IRQ(void){
interrupts[INUM_ADC]();
}
#pragma vector=URX1_VECTOR
__interrupt void URX1_IRQ(void){
interrupts[INUM_URX1]();
}
#pragma vector=ENC_VECTOR
__interrupt void ENC_IRQ(void){
interrupts[INUM_ENC]();
}
#pragma vector=ST_VECTOR
__interrupt void ST_IRQ(void){
interrupts[INUM_ST]();
}
#pragma vector=P2INT_VECTOR
__interrupt void P2INT_IRQ(void){
interrupts[INUM_P2INT]();
}
#pragma vector=UTX0_VECTOR
__interrupt void UTX0_IRQ(void){
interrupts[INUM_UTX0]();
}

#pragma vector=T1_VECTOR
__interrupt void T1_IRQ(void){
interrupts[INUM_T1]();
}

#pragma vector=T2_VECTOR
__interrupt void T2_IRQ(void){
interrupts[INUM_T2]();
}
#pragma vector=T3_VECTOR
__interrupt void T3_IRQ(void){
interrupts[INUM_T3]();
}

#pragma vector=P0INT_VECTOR
__interrupt void P0INT_IRQ(void){
interrupts[INUM_P0INT]();
}
#pragma vector=UTX1_VECTOR
__interrupt void UTX1_IRQ(void){
interrupts[INUM_UTX1]();
}

#pragma vector=WDT_VECTOR
__interrupt void WDT_IRQ(void){
  interrupts[INUM_WDT]();
}
