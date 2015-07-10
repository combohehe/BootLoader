/******************************************************************************
Filename:     radio.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/

#include "cul.h"

//UINT8 temp_count =0;

extern volatile BYTE sppRxStatus;
extern volatile BYTE sppTxStatus;
__no_init SPP_RX_STRUCT rxData @ "PM0_XDATA";
__no_init SPP_TX_STRUCT txData @ "PM0_XDATA";

extern DMA_TABLE_ENTRY dmaTable[DMA_ADM_NUMBER_OF_CHANNELS];//add by zpf.
//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
void radioInit(UINT32 frequency, BYTE localAddress)
{
   sppInit(frequency,localAddress);

   return;
}


//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
BOOL radioSend(BYTE* transmitData, WORD dataLength, BYTE remoteAddress, BYTE doAck)
{
   WORD sent = 0;
   BOOL status = TRUE;
   WORD remaining;
   BYTE retries;
   BYTE res;
/*
typedef struct{
    BYTE payloadLength;
    BYTE destAddress;
    BYTE flags;
    BYTE *payload;
}SPP_TX_STRUCT;

typedef struct{
    BYTE payloadLength;
    BYTE destAddress;
    BYTE srcAddress;
    BYTE flags;
    BYTE payload[SPP_MAX_PAYLOAD_LENGTH + SPP_FOOTER_LENGTH];
}SPP_RX_STRUCT;
*/

   txData.destAddress = remoteAddress;
   txData.flags = doAck;


   while((sent < dataLength) && (status == TRUE))
   {
      FeetDog();
      retries = ACK_RETRIES;  	//ÖØÊÔ4´Î
      txData.payload = transmitData + sent;
      remaining = dataLength-sent;


      if(remaining > SPP_MAX_PAYLOAD_LENGTH)//122
      {
         txData.payloadLength = SPP_MAX_PAYLOAD_LENGTH;
         sent += SPP_MAX_PAYLOAD_LENGTH;
      }
      else
      {
         txData.payloadLength = dataLength;
         sent += dataLength;
      }

      while(retries)
      {
         FeetDog();
         res = sppSend(&txData);
         if(res == CHANNEL_BUSY)
         {
            //temp_count = 1;
            halWait(10);
            //temp_count=0;
            retries--;
            if(retries == 0)
            {
               status = FALSE;
            }
         }
         else
         {
            retries = 0;
         }
      }

      //while(sppTxStatus == TX_IN_PROGRESS) FeetDog();

      //if(sppTxStatus == DEST_UNREACHABLE)
      //{
      //   status = FALSE;
      //}
   }

   return status;
}





//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
BOOL radioReceive(BYTE** receiveData, BYTE* length, WORD timeout, BYTE* sender)
{
   BOOL status = TRUE;
   BOOL continueWaiting = TRUE;
   BOOL useTimeout = FALSE;

   if(timeout)
   {
      useTimeout = TRUE;
   }
   
   sppReceive(&rxData);

   while((sppRxStatus != RX_COMPLETE) && (continueWaiting))
   {
      FeetDog();
     //===========add my code here to instead of the dma_IRQ======--//   
      if(DMAIF == 1)
      {
        DMAIF = 0;

        if(DMAIRQ & DMA_CHANNEL_0){
      // Clearing interrupt flag
          DMAIRQ &= ~DMA_CHANNEL_0;
      // Calling the callback function.
        if(dmaTable[0].callBackFunction){
          dmaTable[0].callBackFunction();
          }
        }
        if(DMAIRQ & DMA_CHANNEL_1){ 
      // Clearing interrupt flag
          DMAIRQ &= ~DMA_CHANNEL_1;
      // Calling the callback function.
        if(dmaTable[1].callBackFunction){
          dmaTable[1].callBackFunction();
          }
        }
        if(DMAIRQ & DMA_CHANNEL_2){
      // Clearing interrupt flag
          DMAIRQ &= ~DMA_CHANNEL_2;
      // Calling the callback function.
        if(dmaTable[2].callBackFunction){
          dmaTable[2].callBackFunction();
          }
        }
        if(DMAIRQ & DMA_CHANNEL_3){
      // Clearing interrupt flag
          DMAIRQ &= ~DMA_CHANNEL_3;
      // Calling the callback function.
        if(dmaTable[3].callBackFunction){
          dmaTable[3].callBackFunction();
          }
        }
        if(DMAIRQ & DMA_CHANNEL_4){
      // Clearing interrupt flag
          DMAIRQ &= ~DMA_CHANNEL_4;
      // Calling the callback function.
        if(dmaTable[4].callBackFunction){
          dmaTable[4].callBackFunction();
          }
        } 
      }
     //===========add my code here to instead of the dma_IRQ======--//
      if(useTimeout)
      {
         halWait(0x02);
         timeout--;
         if(timeout == 0)
         {
            continueWaiting = FALSE;
            status = FALSE;
            STOP_RADIO();
         }
      }
   }

   if(status == TRUE)
   {
      *receiveData = rxData.payload;
      *length = rxData.payloadLength;
      *sender = rxData.srcAddress;
   }

   return status;
}
