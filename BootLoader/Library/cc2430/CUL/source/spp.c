/******************************************************************************
Filename:     spp.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include <string.h>
#include "cul.h"
#include <stdio.h>


// protos
void rxCallBack(void);
//void ackTimeout(void);
//BOOL ackReceived(BYTE sourceAddress);
//void sendAck(SPP_RX_STRUCT* receivedPacket);
//void waitForAck(void);


static DMA_DESC* dmaTx;          // pointer to the DMA descriptor for transmit.
static DMA_DESC* dmaRx;          // pointer to the DMA descriptor for receive.
static BYTE dmaNumberTx = 1;     // number indicating which DMA channel is used for transmit.
static BYTE dmaNumberRx = 2;     // number indicating which DMA channel is used for receive.
static BYTE myAddress;
volatile BYTE sppRxStatus = 0;
volatile BYTE  sppTxStatus = 0;
//static BYTE pAckBuffer[7];
//static SPP_TX_STRUCT* pAckData;
static volatile UINT8 retransmissionCounter;
//static UINT8 ackTimerNumber;
//static FUNCTION* rxCallBackFunction;


//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
//�����û�ָ���Ļص��������ڽ��յ�һ����ȷ�����ݰ�������
//ͨ��������������ó������ı���ȷ�������ݰ���Ķ�����
//callBackFunction �û�ָ���ĺ���
//rxCallBackFunction ָ�� FUNCTION ��ȫ�ֱ���ָ�����
//void sppSetRxCallBackFunction(FUNCTION* callBackFunction)
//{
//   rxCallBackFunction = callBackFunction;
//} // Ends sppSetRxCallBackFunction()


//����Ӧ��
//SPP_RX_STRUCT ������ cul.h
//SFR(  RFD       ,  0xD9  )   //  RF Data ������ioCC2430.h
// myAddress ȫ�ֱ���
// ACK cul.h �к궨��
// ISTXON hal.h �к궨��
// srcAddress Դ��ַ
/*void sendAck(SPP_RX_STRUCT* receivedPacket)
{
   RFD = SPP_HEADER_AND_FOOTER_LENGTH + SPP_ACK_LENGTH;
   RFD = receivedPacket->srcAddress;
   RFD = myAddress;
   RFD = ACK;
   RFD = 0;
   RFIF &= ~IRQ_TXDONE;
   ISTXON;
   while(!(RFIF & IRQ_TXDONE));

   return;
}
*/
//------------------------------------------------------------------------------------------------------
// void rxCallBack(...)
//
//  Description:
//      This function is called by the interrupt routine when the Rx DMA channel
//      finishes the data transfer. The received packet's destination address
//      is checked. If not addressed to this node, or if the CRC value is not
//      correct, the packet is erased. An ACK is sent if the packet
//      tells to. A user defined callback function may is run if set (set
//      with setRxCallBackFunction())
//��Rx DAM  ͨ��������ݴ�������жϳ�����á������հ�����Ŀ�ĵ�ַ�������ַ����
//����ڵ�ģ���CRCֵ�Ǵ���ģ���װ����������������ݰ�����֪������һ��Ӧ��һ���û�����Ļص���������������������˵Ļ���
//  Arguments:
//      void
//
//  Return value:
//      void
//-----------------------------------------------------------------------------
//#define  RXFIFOCNT   XREG( 0xDF53 )  /*  Receive FIFO Count ������ioCC2430.h
// ISFLUSHRX �� RX FIFO �����˲�. hal.h �����˲��ж���
// GET_DMA_DEST(dmaRx) hal.h �еĺ궨�壬ȡ��һ�� DMA ͨ����Ŀ���ַ
// static DMA_DESC* dmaRx ָ���豸 DMA ��������ָ�룬ȫ�ֱ���.DMA_DESC DMA ���ýṹ��
//
void rxCallBack(void)
{
   SPP_RX_STRUCT __xdata* receivedPacket;
   BYTE res = FALSE;

   if(RXFIFOCNT > 0)
   {
      ISFLUSHRX;
      ISFLUSHRX;
   }

   // Investigating the received packet.
   // Checking the destination address and that the CRC is OK.
   // The message is ACK'ed if it tells to.
   receivedPacket = (SPP_RX_STRUCT __xdata*) GET_DMA_DEST(dmaRx);
   receivedPacket->payloadLength = receivedPacket->payloadLength-SPP_HEADER_AND_FOOTER_LENGTH; //��ȥͷ��β����

   if((receivedPacket->destAddress == myAddress) /*|| (receivedPacket->destAddress == BROADCAST_ADDRESS)*/) //��Ŀ�ĵ��Ǳ��ڵ��ַ��㲥��ַ
   {
      if(receivedPacket->payload[receivedPacket->payloadLength+1] & 0x80)	//CRC��ȷ
      {
         //if(receivedPacket->flags == ACK)	//�յ���Ӧ���
         //{
         //   res = ackReceived(receivedPacket->srcAddress);
         //}
         //else		//�յ���ͨ���ݰ�
         //{
            sppRxStatus = PACKET_RECEIVED;		//ssp����״̬���յ����ݰ�
            res = TRUE;
            //if(receivedPacket->flags & DO_ACK)	//Ҫ���յ���Ӧ��
            //{
            //   sendAck(receivedPacket); 		//��Ӧ��
            //}
            sppRxStatus = RX_COMPLETE;			//ssp����״̬���������
            //if(rxCallBackFunction)
            //{
            //   rxCallBackFunction();			//�����û�ָ���ص�����
            //}
         //}
      }
   }

   if(res == FALSE)
   {
      ISFLUSHRX;	//��RX FIFO
      ISFLUSHRX;

      // rearming DMA channel
      DMA_ARM_CHANNEL(dmaNumberRx);
    //=============add my codes here --zpf===========//
      RFIF = 0x00;                        // Clear all interrupt flags
      INT_SETFLAG(INUM_RF, INT_CLR);    // Clear MCU interrupt flag
      //if(sppRxStatus == RX_WAIT)
      //{
         sppRxStatus = RX_IN_PROGRESS;
         //RFIM &= ~IRQ_SFD;
      //}
    //=============add my codes here --zpf===========//
      //RFIM |= IRQ_SFD;
      //sppRxStatus = RX_WAIT;
   }
   return;
}   // ends rxCallBack




//-----------------------------------------------------------------------------
// void ackTimeout(...)
//
//  Description:
//      This function resends a packet if it is not ACK'ed by the recipient
//      within _ACK_TIMEOUT_ m-seconds. The message is resent _ACK_RETRIES_ times.
//      If the message remains un-ACK'ed, transmission is aborted and spp TX
//      status is set to DEST_UNREACHABLE.
//���û���յ����������ص�Ӧ�𣬽����·������ݰ�
//  Arguments:
//      void
//
//  Return value:
//      void
//-----------------------------------------------------------------------------
// TIMER4_RUN() �꺯���� hal.h �ж���
// #define ACK_RETRIES  3 �� hal.h �еĺ궨�峣��
/*void ackTimeout(void){
   culTimer4AdmClear(ackTimerNumber);

   if(pAckData != NULL)
   {
      if(retransmissionCounter < ACK_RETRIES)	//�ط�����С�ڳ��Խ���Ӧ�����
      {
         // Resending the message.
         pAckData->flags |= RETRANSMISSION;		//���ݰ���־���ط�

         TIMER4_RUN(FALSE);						//ֹͣ��ʱ��4

         sppSend(pAckData);						//�������ݰ�

         T4CNT = 0;								//��ʱ��4��������
         TIMER4_RUN(TRUE);						//��ʱ��4��ʼ����

         retransmissionCounter++;
      }
      else
      {
         // The packet has been resent too many times. Assuming that the node is unreacheble.
         pAckData = 0;
         retransmissionCounter = 0;
         sppTxStatus = DEST_UNREACHABLE;		//ssp����״̬���������
         RFIM &= ~IRQ_FIFOP;
      }
   }
   return;
} // ends ackTimeout
*/



//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
// ��ʼ���򵥵����ݰ�װЭ��Simple Packet Protocol (SPP)
// �� DMA �������������� DMA ͨ�������ڷֱ�� Rx FIFO �� Tx FIFO �������ݡ���ʱ��4
// ������ͬ�������ã������Ԫ���������ݰ����ͺ��������һ��ʱ����û�з���Ӧ��ʱ
// �����жϡ����߲�������Ϊ���ͣ��������ض���Ƶ�ʣ��ڷ���ʱ�Զ�����Ͳ���ͼ��CRCֵ��
//-----------------------------------------------------------------------------
BOOL sppInit(UINT32 frequency, BYTE address){
   BYTE res = 0;
   BOOL status = TRUE;

   sppSetAddress(address);

   // Clearing the states of the spp.
   sppTxStatus = TX_IDLE;	//�� cul.h TX �� RX ״̬��־���ֺ궨��
   sppRxStatus = RX_IDLE;	//ͬ��
   retransmissionCounter = 0;
   //ackTimerNumber = 0;
   //pAckData = 0;

   // Clearing the RF interrupt flags and enable mask and enabling RF interrupts
   RFIF = 0;
   RFIM = 0;
   INT_SETFLAG(INUM_RF,INT_CLR);
   INT_ENABLE(INUM_RF,INT_ON);

   // Setting the frequency and initialising the radio
   res = halRfConfig(frequency); //��rfconfig.c
   if(res == FALSE){
      status = FALSE;
   }

   // Setting the number of bytes to assert the FIFOP flag
   IOCFG0 = 7;

   INT_SETFLAG(INUM_RFERR, INT_CLR);
   INT_ENABLE(INUM_RFERR, INT_ON);

   // Flushing both Tx and Rx FiFo. The flush-Rx is issued twice to reset the SFD.
   // Calibrating the radio and turning on Rx to evaluate the CCA.
   // SFD ��ʼ֡�����
   SRXON;
   SFLUSHTX;
   SFLUSHRX;
   SFLUSHRX;
   STXCALN;
   ISSTART;


   // Using the timer 4 administrator to generate interrupt to check if a message is unacked...
   //culTimer4AdmInit();

   // Initialising the DMA administrator
   culDmaInit();

   // Requesting a DMA channel for transmit data. No callback function is used. Instead the TX_DONE
   // interrupt is used to determine when a transfer is finished. Configuring the DMA channel for
   // transmit. The data address and length will be set prior to each specific transmission.
   dmaTx = culDmaAllocChannel(&dmaNumberTx, 0);
   if((dmaNumberTx == 0) || (dmaNumberTx > 4)){
      status = FALSE;
   }

   culDmaToRadio(dmaTx, 0, 0, FALSE);

   // Requesting a DMA channel for receiving data. Giving the address of the callback function.
   // Configuring the DMA channel for receive. The data address will be set prior to each specific
   // reception.
   dmaRx = culDmaAllocChannel(&dmaNumberRx, &rxCallBack);
   if((dmaNumberRx == 0) || (dmaNumberRx > 4)){
      status = FALSE;
   }
   culDmaFromRadio(dmaRx, 0, TRUE);

   // Making sure that none of the channels are armed.
   DMA_ABORT_CHANNEL(dmaNumberRx);
   DMA_ABORT_CHANNEL(dmaNumberTx);
   INT_ENABLE(INUM_DMA, INT_ON);

   DMAIF = 1;
   
   return status;
} // ends sppInit




//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
void sppSetAddress(BYTE address){
   myAddress = address;
} // Ends sppSetAddress()




// Internal function which enables the timeout when waiting for an ACK.
//void waitForAck(void)
//{
//   ackTimerNumber = culTimer4AdmSet(ACK_TIMEOUT, &ackTimeout);
//   SET_DMA_DEST(dmaRx,pAckBuffer);
//   SET_DMA_LENGTH(dmaRx,7);

//}   // Ends waitForAck()




//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
BYTE sppSend(SPP_TX_STRUCT* pPacketPointer){
   BYTE res = TRUE;
   
   // If data is to be transmitted, the DMA is set up.
   if(pPacketPointer->payloadLength)
   {
      if (pPacketPointer->payloadLength > SPP_MAX_PAYLOAD_LENGTH)
      {
         res = TOO_LONG;
         sppTxStatus = TX_IDLE;
      }
      else
      {
         // Setting up the DMA
         DMA_ABORT_CHANNEL(dmaNumberTx);
         SET_DMA_SOURCE(dmaTx,pPacketPointer->payload);
         SET_DMA_LENGTH(dmaTx,pPacketPointer->payloadLength);
      }
   }

   // Proceed if the packet length is OK.
   if (res == TRUE)
   {
      // Flipping the sequence bit if the transfer is not a retransmission.
      if(!(pPacketPointer->flags & RETRANSMISSION))
      {
         pPacketPointer->flags ^= SEQUENCE_BIT;
      }

      // Clearing RF interrupt flags and enabling RF interrupts.
      if(FSMSTATE == 6 && RXFIFOCNT > 0)
      {
         ISFLUSHRX;
         ISFLUSHRX;
      }

      RFIF &= ~IRQ_TXDONE;
      RFIM &= ~IRQ_SFD;
      INT_SETFLAG(INUM_RF, INT_CLR);

      // Writing the total packet length, addresses and flags to Tx FiFo.
      // Transferring the payload if any.RFD RF Data
      RFD = (pPacketPointer->payloadLength + SPP_HEADER_AND_FOOTER_LENGTH); //���ذ���,Header=3,Footer=2
      RFD = pPacketPointer->destAddress;		//����Ŀ���ַ
      RFD = myAddress;							//����Դ��ַ��TX�ṹ����û�У�
      RFD = pPacketPointer->flags;				//��־
      if(pPacketPointer->payloadLength)
      {
         DMA_ARM_CHANNEL(dmaNumberTx);
         DMA_START_CHANNEL(dmaNumberTx);
      }

      // If the RSSI value is not valid, enable receiver
      if(RSSIL == 0x80)
      {
         ISRXON;
         // Turning on Rx and waiting 320u-sec to make the RSSI value become valid.
         //temp_count = 3;
         halWait(1);
         //temp_count=0;
      }

      //Transmitting
      ISTXONCCA;
      //if(TX_ACTIVE)
      if(FSMSTATE > 30)
      {
         // Asserting the status flag and enabling ACK reception if expected.
         //sppTxStatus = TX_IN_PROGRESS;

         //if(pPacketPointer->flags & DO_ACK)
         //{
         //   pAckData = pPacketPointer;
         //   DMA_ABORT_CHANNEL(dmaNumberRx);
            //waitForAck();
         //}
         //else
         //{
            //pAckData = NULL;
         //}
         //RFIM |= IRQ_TXDONE;
       //=============add my codes here --zpf===========//
         while(!(RFIF & IRQ_TXDONE) && !(RFERRIF == 1))  FeetDog();
         if(RFERRIF == 1)
         {
          // If Rx overflow occurs, the Rx FiFo is reset.
          // The Rx DMA is reset and reception is started over.
           if(FSMSTATE == 17)
           {
              STOP_RADIO();
              ISFLUSHRX;
              ISFLUSHRX;
              DMA_ABORT_CHANNEL(dmaNumberRx);
              DMA_ARM_CHANNEL(dmaNumberRx);
              ISRXON;
           }
           else if(FSMSTATE == 56)
           {
              ISFLUSHTX;
           }
           INT_SETFLAG(INUM_RFERR,INT_CLR);
         }
         if(RFIF & IRQ_TXDONE)
         {
           sppTxStatus = TX_SUCCESSFUL;
           RFIF = 0x00; 
         }
         RFIF = 0x00;                        // Clear all interrupt flags
         INT_SETFLAG(INUM_RF, INT_CLR);    // Clear MCU interrupt flag
       //=============add my codes here --zpf===========//
      }
      else
      {
         ISFLUSHTX;
         res = CHANNEL_BUSY;
         RFIM &= ~IRQ_TXDONE;
         // De-flipping the sequence bit.
         if(!(pPacketPointer->flags & RETRANSMISSION))
         {
            pPacketPointer->flags ^= SEQUENCE_BIT;
         }
      }
   }
   return res;
} // ends sppSend




// Internal function which is called when an ack is received.
// If the ACK is from the expected node, the retransmission of the packet is cancelled.
// �յ�һ��Ӧ��ʱ���á����Ӧ���������Ľڵ㣬��ȡ�����ݰ��ط�
// static SPP_TX_STRUCT* pAckData; ȫ�ֱ���
// SPP_TX_STRUCT.�ṹ���� cul.h ����
/*BOOL ackReceived(BYTE sourceAddress)
{
   BOOL res = FALSE;
   if(sourceAddress == pAckData->destAddress)
   {
      res = TRUE;
      culTimer4AdmClear(ackTimerNumber);
      sppTxStatus = TX_SUCCESSFUL;
      retransmissionCounter = 0;
      pAckData = 0;
   }

   return res;
}  //Ends ackReceived()
*/

/*
//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
#pragma vector=RF_VECTOR
__interrupt void spp_rf_IRQ(void)
{
   BYTE enabledAndActiveInterrupt;

   INT_GLOBAL_ENABLE(INT_OFF);
   enabledAndActiveInterrupt = RFIF;
   RFIF = 0x00;                        // Clear all interrupt flags
   INT_SETFLAG(INUM_RF, INT_CLR);    // Clear MCU interrupt flag
   enabledAndActiveInterrupt &= RFIM;

   // Start of frame delimiter (SFD)
   if(enabledAndActiveInterrupt & IRQ_SFD)
   {
      if(sppRxStatus == RX_WAIT)
      {
         sppRxStatus = RX_IN_PROGRESS;
         RFIM &= ~IRQ_SFD;
      }
   }

   // Transmission of a packet is finished. Enabling reception of ACK if required.
   if(enabledAndActiveInterrupt & IRQ_TXDONE)
   {
      if(sppTxStatus == TX_IN_PROGRESS)
      {
         if(pAckData == NULL)
         {
            sppTxStatus = TX_SUCCESSFUL;
         }
         else
         {
            DMA_ARM_CHANNEL(dmaNumberRx);
         }
      }

      // Clearing the tx done interrupt enable
      RFIM &= ~IRQ_TXDONE;

   }
   INT_GLOBAL_ENABLE(INT_ON);
}
*/




//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
void sppReceive(SPP_RX_STRUCT* pReceiveData){
  
  //===========add my code here to instead of the dma_IRQ======--//
      if(RFERRIF == 1)
        {
          // If Rx overflow occurs, the Rx FiFo is reset.
          // The Rx DMA is reset and reception is started over.
          if(FSMSTATE == 17)
          {
              STOP_RADIO();
              ISFLUSHRX;
              ISFLUSHRX;
              DMA_ABORT_CHANNEL(dmaNumberRx);
              DMA_ARM_CHANNEL(dmaNumberRx);
              ISRXON;
          }
          else if(FSMSTATE == 56)
          {
              ISFLUSHTX;
          }
          INT_SETFLAG(INUM_RFERR,INT_CLR);
        }
   //===========add my code here to instead of the dma_IRQ======--//

   sppRxStatus = RX_WAIT;

   DMA_ABORT_CHANNEL(dmaNumberRx);
   // Setting the address to where the received data are to be written.
   SET_DMA_DEST(dmaRx,pReceiveData);
   SET_DMA_LENGTH(dmaRx,255);

   // Arming the DMA channel. The receiver will initate the transfer when a packet is received.
   DMA_ARM_CHANNEL(dmaNumberRx);
	//FSMSTATE����״̬��״̬
   if(FSMSTATE == 6 && RXFIFOCNT > 0)
   {
      ISFLUSHRX;
      ISFLUSHRX;
   }

   // Turning on the receiver
   ISRXON;

   return;
}


/*
//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
//-----------------------------------------------------------------------------
#pragma vector=RFERR_VECTOR
__interrupt static void rf_error_IRQ(void)
{
   INT_GLOBAL_ENABLE(INT_OFF);

   // If Rx overflow occurs, the Rx FiFo is reset.
   // The Rx DMA is reset and reception is started over.
   if(FSMSTATE == 17)
   {
      STOP_RADIO();
      ISFLUSHRX;
      ISFLUSHRX;
      DMA_ABORT_CHANNEL(dmaNumberRx);
      DMA_ARM_CHANNEL(dmaNumberRx);
      ISRXON;
   }
   else if(FSMSTATE == 56)
   {
      ISFLUSHTX;
   }

   INT_SETFLAG(INUM_RFERR,INT_CLR);

   INT_GLOBAL_ENABLE(INT_ON);
}
*/