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
//设置用户指定的回调函数，在接收到一个正确的数据包死运行
//通过这个函数可以用程序来改变正确接收数据包后的动作。
//callBackFunction 用户指定的函数
//rxCallBackFunction 指向 FUNCTION 的全局变量指针变量
//void sppSetRxCallBackFunction(FUNCTION* callBackFunction)
//{
//   rxCallBackFunction = callBackFunction;
//} // Ends sppSetRxCallBackFunction()


//发送应答
//SPP_RX_STRUCT 定义在 cul.h
//SFR(  RFD       ,  0xD9  )   //  RF Data 定义在ioCC2430.h
// myAddress 全局变量
// ACK cul.h 中宏定义
// ISTXON hal.h 中宏定义
// srcAddress 源地址
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
//在Rx DAM  通道完成数据传输后由中断程序调用。检查接收包掉的目的地址，如果地址不是
//这个节点的，或CRC值是错误的，包装将被擦除。如果数据包被告知将发送一个应答。一个用户定义的回调函数可以运行如果定义了的话。
//  Arguments:
//      void
//
//  Return value:
//      void
//-----------------------------------------------------------------------------
//#define  RXFIFOCNT   XREG( 0xDF53 )  /*  Receive FIFO Count 定义在ioCC2430.h
// ISFLUSHRX 清 RX FIFO 命令滤波. hal.h 命令滤波中定义
// GET_DMA_DEST(dmaRx) hal.h 中的宏定义，取得一个 DMA 通道的目标地址
// static DMA_DESC* dmaRx 指向设备 DMA 描述符的指针，全局变量.DMA_DESC DMA 配置结构。
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
   receivedPacket->payloadLength = receivedPacket->payloadLength-SPP_HEADER_AND_FOOTER_LENGTH; //减去头和尾长度

   if((receivedPacket->destAddress == myAddress) /*|| (receivedPacket->destAddress == BROADCAST_ADDRESS)*/) //包目的地是本节点地址或广播地址
   {
      if(receivedPacket->payload[receivedPacket->payloadLength+1] & 0x80)	//CRC正确
      {
         //if(receivedPacket->flags == ACK)	//收到了应答包
         //{
         //   res = ackReceived(receivedPacket->srcAddress);
         //}
         //else		//收到普通数据包
         //{
            sppRxStatus = PACKET_RECEIVED;		//ssp接收状态－收到数据包
            res = TRUE;
            //if(receivedPacket->flags & DO_ACK)	//要求收到后发应答
            //{
            //   sendAck(receivedPacket); 		//发应答
            //}
            sppRxStatus = RX_COMPLETE;			//ssp接收状态－接收完成
            //if(rxCallBackFunction)
            //{
            //   rxCallBackFunction();			//调用用户指定回调函数
            //}
         //}
      }
   }

   if(res == FALSE)
   {
      ISFLUSHRX;	//清RX FIFO
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
//如果没有收到接收器返回的应答，将重新发送数据包
//  Arguments:
//      void
//
//  Return value:
//      void
//-----------------------------------------------------------------------------
// TIMER4_RUN() 宏函数在 hal.h 中定义
// #define ACK_RETRIES  3 在 hal.h 中的宏定义常数
/*void ackTimeout(void){
   culTimer4AdmClear(ackTimerNumber);

   if(pAckData != NULL)
   {
      if(retransmissionCounter < ACK_RETRIES)	//重发次数小于尝试接收应答次数
      {
         // Resending the message.
         pAckData->flags |= RETRANSMISSION;		//数据包标志－重发

         TIMER4_RUN(FALSE);						//停止定时器4

         sppSend(pAckData);						//发送数据包

         T4CNT = 0;								//定时器4计数清零
         TIMER4_RUN(TRUE);						//定时器4开始运行

         retransmissionCounter++;
      }
      else
      {
         // The packet has been resent too many times. Assuming that the node is unreacheble.
         pAckData = 0;
         retransmissionCounter = 0;
         sppTxStatus = DEST_UNREACHABLE;		//ssp发送状态－不能完成
         RFIM &= ~IRQ_FIFOP;
      }
   }
   return;
} // ends ackTimeout
*/



//-----------------------------------------------------------------------------
// See cul.h for a description of this function.
// 初始化简单的数据包装协议Simple Packet Protocol (SPP)
// 从 DMA 管理器申请两个 DMA 通道，用于分别从 Rx FIFO 和 Tx FIFO 传输数据。定时器4
// 管理器同样被设置，这个单元用于在数据包发送后接收器在一定时间内没有返回应答时
// 产生中断。无线部分配置为发送，工作在特定的频率，在发送时自动计算和插入和检查CRC值。
//-----------------------------------------------------------------------------
BOOL sppInit(UINT32 frequency, BYTE address){
   BYTE res = 0;
   BOOL status = TRUE;

   sppSetAddress(address);

   // Clearing the states of the spp.
   sppTxStatus = TX_IDLE;	//在 cul.h TX 和 RX 状态标志部分宏定义
   sppRxStatus = RX_IDLE;	//同上
   retransmissionCounter = 0;
   //ackTimerNumber = 0;
   //pAckData = 0;

   // Clearing the RF interrupt flags and enable mask and enabling RF interrupts
   RFIF = 0;
   RFIM = 0;
   INT_SETFLAG(INUM_RF,INT_CLR);
   INT_ENABLE(INUM_RF,INT_ON);

   // Setting the frequency and initialising the radio
   res = halRfConfig(frequency); //在rfconfig.c
   if(res == FALSE){
      status = FALSE;
   }

   // Setting the number of bytes to assert the FIFOP flag
   IOCFG0 = 7;

   INT_SETFLAG(INUM_RFERR, INT_CLR);
   INT_ENABLE(INUM_RFERR, INT_ON);

   // Flushing both Tx and Rx FiFo. The flush-Rx is issued twice to reset the SFD.
   // Calibrating the radio and turning on Rx to evaluate the CCA.
   // SFD 开始帧定界符
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
      RFD = (pPacketPointer->payloadLength + SPP_HEADER_AND_FOOTER_LENGTH); //加载包长,Header=3,Footer=2
      RFD = pPacketPointer->destAddress;		//加载目标地址
      RFD = myAddress;							//插入源地址（TX结构体中没有）
      RFD = pPacketPointer->flags;				//标志
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
// 收到一个应答时调用。如果应答是期望的节点，将取消数据包重发
// static SPP_TX_STRUCT* pAckData; 全局变量
// SPP_TX_STRUCT.结构体在 cul.h 定义
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
	//FSMSTATE有限状态机状态
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
