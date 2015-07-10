/******************************************************************************
Filename:     rf_test.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0

Description:
    This file provides 4 small tests which demonstrates use of the radio.

******************************************************************************/

#include "ioCC2430.h"
#include "cul.h"
#include "hal.h"
#include "RF04EB.h"
#include "flash.h"

//#define ADDRESS_0 0x01
//#define ADDRESS_1 0x02

#define SEND     0
#define RECEIVE  1
#define RECEIVE_TIMEOUT  200

void initRfTest();
//void receivePacket(UINT8 *receiveByte);
//void sendPacket(UINT8 sendByte);
void Enter_IAP_Mode(void);
void Process_IAP_Data(void);
BOOL RADIO_Receive_Data(void);
BOOL VerifyProgram(BYTE *Data , BYTE length , BYTE CalcResult);
BOOL CheckAPP(INT32 App_Addr);
BOOL CheckTable(UINT16 Length,UINT8 CalcResult);
void Init_Watchdog(void);
void BT_Init(void);
BOOL Mac_Judge(BYTE * ,BYTE *);
void GetConfig(BYTE num);
BOOL Is_APP1_Full();
void Read_Flash();

static void (*APPEntry)(void);
void Delay(UINT8 second);
void FlashLed(UINT8 LedType,UINT8 FlashNumber);

void UartTX_Send_String(UINT8 *Data,int len);
void PutChar(UINT8 data);


#pragma location=0x2000        //8K+
const __code UINT8 APP_INFO1[12];//应用程序完整性标志(4 Bytes)+应用程序版本号+应用程序校验和+应用程序入口地址高字节+
//应用程序入口地址低字节+应用程序起始地址高字节+应用程序起始地址低字节+应用程序结束地址高字节+应用程序结束地址低字节
//#pragma location=0x7000        //32K+
//const __code UINT8 APP_INFO2[12];

#pragma location=0xC000        //48K+
const __code UINT8 Config_Table[8];//配置表完整性标志(4 Bytes)+版本号+校验和+配置表大小高字节+低字节

#pragma location=0xD800        //54K+
const __code UINT8 Node_Info[8];//物理地址(4 Bytes)+逻辑地址+0xFF+0xFF+0xFF  频段+是否获得逻辑地址标志+本地地址+远程地址

BYTE WriteFlag[4]={0xFF,0xFF,0xFF,0xFF};//应用程序完整性标志
BYTE WriteInfo[8]={0x02,0x02,0x02,0x02,0x55,0xFF,0xFF,0xFF};
//*+长度+汇聚ID+消息地址+ACK标志会话结束标志+流水号+消息类型+消息参数列表+#
//BYTE IAP_ACK[10]={0x2A,0x0A,0xFF,0xFE,0x11,0x31,0x81,0x01,0x00,0x23};
//BYTE IAP_Request[8]={0x2A,0x08,0xFF,0xFE,0x11,0x21,0x21,0x23};
//会话结束标志+流水号+消息类型+消息参数列表
BYTE IAP_ACK[6]={0x11,0x31,0x81,0x01,0x00,0x00};
BYTE IAP_Request[4]={0x11,0x21,0x21,0x01};
BYTE IAP_NetWork[7]={0x11,0x24,0x24,0x00,0x00,0x00,0x00}; //组网请求

UINT8 myAddr;
UINT8 remoteAddr;
BYTE* receiveBuffer;
BYTE receiveLength;
BYTE sender;
UINT32 frequency;

//BOOL Check_Result0=FALSE;//APP Program check result
//BOOL Check_Result1=FALSE;//Config Table check result
UINT16 MainAddr;
//BYTE RX_Data[128];
//BYTE RX_Length;
UINT16 Table_Length;
BYTE Retry_Times=0;

/******************************************************************************
* @fn  initRfTest
*
* @brief
*      Initializes components for the RF test application example.
*
* Parameters:
*
* @param  void
*
* @return void
*
******************************************************************************/
void initRfTest(UINT32 frequency)
{
   //UINT32 frequency = 2425000;
   INIT_GLED();
   INIT_YLED();
   radioInit(frequency, myAddr);
}

/******************************************************************************
* @fn      Main function.
******************************************************************************/
void main(void)
{
      BOOL res;
      BYTE tempcount=10;
      BOOL Wait_Continue=TRUE;
      BYTE Mac[4];
      BYTE num=0;
      //BOOL Check_Result0;
      myAddr = 0xFF;
      remoteAddr=0x10;
      
      FlashRead(0xFFFC,Mac,4); //读取Mac地址
      halWait(100);
      
            
      SET_MAIN_CLOCK_SOURCE(CRYSTAL);
      SET_32KHZ_CLOCK_SOURCE(RC);
      
      IAP_NetWork[3]=Mac[0];
      IAP_NetWork[4]=Mac[1];
      IAP_NetWork[5]=Mac[2];
      IAP_NetWork[6]=Mac[3];
      initRfTest(2470000);
      
      BT_Init();        //清除标志，关中断
      //Init_Watchdog();
      //temp_count=0;
      
       while(tempcount)
        {
          //FeetDog();
          YLED = !YLED;
          halWait(200);
          halWait(200);
          tempcount--;
        }
       
      if(Is_APP1_Full()==TRUE)
      {
         MainAddr = BUILD_UINT16(APP_INFO1[7], APP_INFO1[6]);//MSB
         APPEntry = (void (*)(void))MainAddr;
         (*APPEntry)(); 
      }
      else
      {       
        if(Node_Info[4] != 0x55)
        {
          while(Wait_Continue)
          {
      //========== add relay configuration============//      
            GetConfig(num);
      //==========add relay configuration============// 
          //根据逻辑地址调频
            halRfSetRadioFrequency(frequency);
            
            //UartTX_Send_String(IAP_NetWork, 7);
            
            halWait(200);
            radioSend(IAP_NetWork, 7, remoteAddr, DO_NOT_ACK );
            halWait(50);
            res = RADIO_Receive_Data();
            
            //UartTX_Send_String(receiveBuffer,receiveLength);
            if(res==TRUE && receiveBuffer[2]==0x34 && Mac_Judge(receiveBuffer,Mac)==TRUE)//判断是否接收到组网请求回复消息
            {
              //UartTX_Send_String(receiveBuffer,receiveLength);
              
              Wait_Continue = FALSE;
              myAddr = receiveBuffer[5];
              sppSetAddress(myAddr);
              WriteInfo[0]=(BYTE)((frequency & 0xFF000000)/0x1000000);
              WriteInfo[1]=(BYTE)((frequency & 0x00FF0000)/0x10000);
              WriteInfo[2]=(BYTE)((frequency & 0x0000FF00)>>8);
              WriteInfo[3]=(BYTE)(frequency & 0x000000FF);
              WriteInfo[4]=0x55;
              WriteInfo[5]=myAddr;
              WriteInfo[6]=remoteAddr;
              FlashWrite( WriteInfo , 0xD800 , 8 );
              halWait(200);
            }
            else
            {
              GLED = !GLED;
              num = (num +1)%3;
            }
          }
        }
        else
        {
          //读频,逻辑地址以及目的中继地址
          frequency=((UINT32)Node_Info[0]<<24)+((UINT32)Node_Info[1]<<16)+((UINT32)Node_Info[2]<<8)+((UINT32)Node_Info[3]);
          halRfSetRadioFrequency(frequency);
          halWait(100);
          sppSetAddress(Node_Info[5]);
          remoteAddr=Node_Info[6];
        }
        
        //进入下载流程
        while(1)
        {
          //FeetDog();
          IAP_Request[1]=0x21;
          IAP_Request[2]=0x21;
          Enter_IAP_Mode();
        }
      }
}

void Enter_IAP_Mode(void)
{
  BOOL Start_Flag=FALSE;
  BOOL res;    
  Init_Watchdog();
  
  while(!Start_Flag)
  { 
    //FeetDog();
    
    //UartTX_Send_String(IAP_Request, 4);
    
    radioSend(IAP_Request, 4, remoteAddr, DO_NOT_ACK );
    halWait(10);
    res = RADIO_Receive_Data();
    
    if(res==TRUE &&((receiveBuffer[2]==0x31)||(receiveBuffer[2]==0x33)))
    {
      //UartTX_Send_String(receiveBuffer,receiveLength);
      Start_Flag = TRUE;
      YLED = LED_ON;//进入IAP模式工作指示灯
      Retry_Times=0;
      Process_IAP_Data();//接收到更新程序起始包
    }
    else
    {
      Retry_Times=Retry_Times+1; 
      if(Retry_Times>=6)
         HAL_SYSTEM_RESET();
    }
  }    
}

void Process_IAP_Data(void)
{   
   BYTE Flash_Value[128];
   BOOL IAP_Result = FALSE;
   BYTE length;
   BYTE CalcResult;
   UINT16 FlashAddr;
   UINT16 WriteAddr;
   BOOL res=TRUE;
       
   while(1)
   {
     if((res==TRUE)&&((receiveBuffer[2]==0x31)||(receiveBuffer[2]==0x33)))
     {
        IAP_ACK[0]=0x11;//更新会话结束标志
        IAP_ACK[1]=receiveBuffer[2];//与消息类型保持一致 
        IAP_ACK[4]=receiveBuffer[3];
        IAP_ACK[5]=receiveBuffer[4];//更新ACK的程序包流水号
        
        if(receiveBuffer[3]==0x00 && receiveBuffer[4]==0x00)//更新起始包
        {
          if(receiveBuffer[2]==0x31)//接收到更新程序包
          {
            //UartTX_Send_String((UINT8 *)receiveBuffer,receiveLength);
            
            CalcResult=receiveBuffer[13];
            length=8;
            if(receiveBuffer[9]==0x24)//APP Aplication 1 起始地址0x2400
            {
              //UartTX_Send_String("A",1);
              //for(int i=0;i<4;i++)
              //  WriteFlag[i]=0x11; 
             // WriteAddr=0x2000;
              //halWait(100);
              FlashWrite((receiveBuffer+5) , 0x2004, length);
              halWait(20);
              FlashRead(0x2004, Flash_Value , length );
              //halWait(200);
            }
          }
          
          if(receiveBuffer[2]==0x33)//接收到更新配置表包
          {
            //UartTX_Send_String((UINT8 *)receiveBuffer,receiveLength);
            
            CalcResult=receiveBuffer[9];
            length=4;
            //for(int i=0;i<4;i++)
             //   WriteFlag[i]=0x88;
             // WriteAddr=0xC000;
            //halWait(100);
            FlashWrite((receiveBuffer+5) , 0xC004, length);
            halWait(20);
            FlashRead(0xC004, Flash_Value , length );
            //halWait(200);
          }    
        }
        else                    //更新数据包
        {
          //UartTX_Send_String((UINT8 *)receiveBuffer,receiveLength);
          
          length = receiveBuffer[5];
          CalcResult = receiveBuffer[length+8];
          FlashAddr = BUILD_UINT16(receiveBuffer[7], receiveBuffer[6]);//MSB
          FlashWrite((receiveBuffer+8) , FlashAddr, length);
          halWait(20);
          FlashRead(FlashAddr , Flash_Value , length );
        }
      
        if(TRUE==VerifyProgram(Flash_Value,length,CalcResult))
        {
            IAP_ACK[3]=0x01;//更新ACK的确认状态
            if(receiveBuffer[3]==0x00 && receiveBuffer[4]==0x01 && receiveBuffer[2]==0x31)
            {//应用程序最后一个包
              IAP_ACK[0]=0x00;//更新会话结束标志
              for(int i=0;i<4;i++)
                WriteFlag[i]=0x11;
              WriteAddr=0x2000;
              FlashWrite( WriteFlag , WriteAddr , 4 );
              halWait(20);
              //UartTX_Send_String(WriteFlag,4);
              //for(int i=0;i<4;i++)
              //  WriteFlag[i]=0x88;
              //WriteAddr=0xC000;
              //halWait(100);
              //FlashWrite( WriteFlag , WriteAddr , 4 );
            }
            if(receiveBuffer[3]==0x00 && receiveBuffer[4]==0x01 && receiveBuffer[2]==0x33)
            {//配置表最后一个包
              IAP_ACK[0]=0x00;//更新会话结束标志
              for(int i=0;i<4;i++)
                WriteFlag[i]=0x88;
              WriteAddr=0xC000;
              FlashWrite( WriteFlag , WriteAddr , 4 );
              halWait(20);
              //UartTX_Send_String(WriteFlag,4);
            }
            if(Is_APP1_Full()==TRUE)//更新结束包
            {
              //IAP_ACK[0]=0x10;//更新会话结束标志
              IAP_Result = TRUE;
            }
        }  
        else
        {
          IAP_ACK[3]=0x00; //该次数据包写入失败
        }
         
        Retry_Times=0;
        receiveBuffer=NULL;
        receiveLength=0;
        res=FALSE;
      }
      else
      {
        Retry_Times=Retry_Times+1; 
        if(Retry_Times>=18)
           HAL_SYSTEM_RESET();
      }
      
      //FeetDog();
     
      //UartTX_Send_String(IAP_ACK, 6);
      
      //halWait(200);
      radioSend(IAP_ACK, 6, remoteAddr, DO_NOT_ACK );//无论接收是否成功，都要发ACK标志
      halWait(50);
      
        //res=RADIO_Receive_Data();
        /*if(res==TRUE && receiveBuffer[2]==0x3F)
        {
          UartTX_Send_String((UINT8 *)receiveBuffer,receiveLength);
          //halWait(100);
          Wait=TRUE;
          Retry_Times=0;
        }
        else
        {
          Retry_Times=Retry_Times+1; 
          if(Retry_Times>=10)
             HAL_SYSTEM_RESET();
        }
      }*/
      //radioSend(IAP_ACK, 5, remoteAddr, DO_NOT_ACK );
      //halWait(100);
      
      //Wait=FALSE;
      
      if(IAP_Result == TRUE)
      {
        //UartTX_Send_String("Z",1);
        //FeetDog();
        
        //Read_Flash();
        HAL_SYSTEM_RESET(); 
      }
      
      //halWait(100);
      res = RADIO_Receive_Data();
  }  
}

//接收数据包格式：会话结束标志+流水号+消息类型+消息参数列表
BOOL  RADIO_Receive_Data(void)
{  
   BOOL Waiting_Continue = TRUE;      
   BOOL Receive_Result=FALSE;
   UINT16 timeout=20;
   BOOL res;
   
   while(Waiting_Continue)
   {
      FeetDog();
      res = radioReceive(&receiveBuffer, &receiveLength, RECEIVE_TIMEOUT, &sender);
      //halWait(200);
      if(res == TRUE)
      {
        GLED = !GLED;
        Waiting_Continue = FALSE;
        Receive_Result=TRUE;
      }
      else
      {
         if(timeout>0)
         {
             halWait(1);
             timeout--;
         }
         else
         {
           Waiting_Continue = FALSE;
           Receive_Result=FALSE;
         }
     
       }
   }
   return Receive_Result;
}

BOOL VerifyProgram(BYTE *Data , BYTE length , BYTE CalcResult)
{
  //UartTX_Send_String("HH",2);
  BOOL result = FALSE;
  UINT8 XCalcResult=0;
  
  for(UINT8 i=0;i<length;i++)
     XCalcResult+=Data[i];
  
  //UartTX_Send_String(Data,8);
  if(XCalcResult==CalcResult)
    result = TRUE;
  else
    result = FALSE;
  
  return result;
}

void Delay(UINT8 second)//delay ** seconds
{
  UINT16 j;
  for(j=0;j<5*second;j++)
  {
      halWait(200);
  } 
}

void FlashLed(UINT8 LedType,UINT8 FlashNumber)
{
  while(FlashNumber)
  {
    if(LedType==0)
    {
      YLED = LED_ON;
      halWait(200);
      YLED = LED_OFF;
      halWait(200);
    }
    else
    {
      GLED = LED_ON;
      halWait(200);
      GLED = LED_OFF;
      halWait(200);
    }
    FlashNumber=FlashNumber-1;
  } 
}
/****************************************************************
*函数功能 ：串口发送字符串函数					
*入口参数 : data:数据									
*			len :数据长度							
*返 回 值 ：无											
*说    明 ：				
****************************************************************/
void PutChar(BYTE data)
{
    U0DBUF = data;
    while(UTX0IF == 0);
    UTX0IF = 0;
}

void UartTX_Send_String(BYTE *Data,int len)
{       
  while(len>0)
  {    
     PutChar(*Data);  
     *Data++;
     len--;
  }       
}

BOOL CheckAPP(INT32 App_Addr)
{
  BYTE APP_INFO[12];
  BYTE APP_CalcResult=0;
  UINT16 APP_StartAddr;
  UINT16 APP_EndAddr;
  BYTE Read_Continue=TRUE;
  UINT16 Read_Length=0;
  UINT16 Read_Addr;
  BYTE APP_DATA[128];
  
  BYTE i=0;
  BYTE Temp_CalcResult=0;
  BOOL Check_Result=FALSE;
    
  FlashRead(App_Addr,APP_INFO,12);
  halWait(200);
  //halWait(200);
  APP_CalcResult = APP_INFO[5];
  APP_StartAddr = BUILD_UINT16(APP_INFO[9], APP_INFO[8]);
  APP_EndAddr = BUILD_UINT16(APP_INFO[11], APP_INFO[10]);
  Read_Addr = APP_StartAddr;
  
  //==========add interrupt vector check============//
  FlashRead(3,APP_DATA,139);
  halWait(200);
  for(i=0;i<139;i++)  
  {
    if(APP_DATA[i]!=0xFF)
      Temp_CalcResult += APP_DATA[i];
  }
  //==========add interrupt vector check============//
  
  while(Read_Continue)
  {
    FeetDog();
    if((APP_EndAddr-Read_Addr)>=128)
    {
      Read_Length = 128;
    }
    else
    {
      Read_Length = APP_EndAddr-Read_Addr;
      Read_Continue = FALSE;
    }
    FlashRead(Read_Addr,APP_DATA,Read_Length);
    halWait(200);
    
    //UartTX_Send_String(APP_DATA,Read_Length);
    
    halWait(200);
    for(i=0;i<Read_Length;i++)
      Temp_CalcResult += APP_DATA[i];
    Read_Addr += Read_Length;
  }
  if(APP_CalcResult==Temp_CalcResult)
    Check_Result=TRUE;
  else
    Check_Result=FALSE;
  
  return Check_Result;
}

BOOL CheckTable(UINT16 Length,UINT8 CalcResult)
{
  BYTE Read_Continue=TRUE;
  UINT16 Read_Addr=0xC008;
  UINT16 Read_Length=0;
  BYTE APP_DATA[128];
  BYTE i=0;
  BYTE Temp_CalcResult=0;
  BOOL Check_Result=FALSE;
  
  while(Read_Continue)
  {
    FeetDog();
    if(Length>=128)
    {
      Read_Length=128;
      Length=Length-128;
    }
    else
    {
      Read_Length = Length;
      Read_Continue = FALSE;
    }
    FlashRead(Read_Addr,APP_DATA,Read_Length);
    halWait(200);
    for(i=0;i<Read_Length;i++)
      Temp_CalcResult += APP_DATA[i];
    Read_Addr += Read_Length;
  }
  
  if(CalcResult==Temp_CalcResult)
    Check_Result=TRUE;
  else
    Check_Result=FALSE;
  return Check_Result;
}

void Init_Watchdog(void)
{
    WDCTL = 0x00;
    //时间间隔一秒，看门狗模式
    WDCTL |= 0x08;
    //启动看门狗
}

void FeetDog(void)
{
    WDCTL = 0xa0;
    WDCTL = 0x50;
}

void BT_Init(void)
{
    DISABLE_ALL_INTERRUPTS();                    //关闭所有中断
    URX0IF = 0;
    RFIF = 0;
    DMAIF = 0;
    RFERRIF = 0;
}

/*
函数功能：软件延时10.94ms
void Delay(void)
{
	uint n;
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
	for(n=50000;n>0;n--);
}
*/

void GetConfig(BYTE num)
{
  switch(num)
  {
    case 0:
      frequency = 2470000;
      remoteAddr = 0x10;
      break;
    case 1:
      frequency = 2450000;
      remoteAddr = 0x20;
      break;
    case 2:
      frequency = 2480000;
      remoteAddr = 0x30;
      break;
    case 3:
      frequency = 2480000;
      remoteAddr = 0x30;
      break;
    case 4:
      frequency = 2480000;
      remoteAddr = 0x30;
      break;
  }
}

BOOL Mac_Judge(BYTE * receive,BYTE * Mac)
{
  if(receive[6]==Mac[0]&&receive[7]==Mac[1]&&receive[8]==Mac[2]&&receive[9]==Mac[3])
    return TRUE;
  else 
    return FALSE;
}

BOOL Is_APP1_Full()
{
  if(APP_INFO1[0]!=0x11 || APP_INFO1[1]!=0x11 || APP_INFO1[2]!=0x11 || APP_INFO1[3]!=0x11
     || Config_Table[0]!=0x88 || Config_Table[1]!=0x88 || Config_Table[2]!=0x88 || Config_Table[3]!=0x88)
    return FALSE;//配置表或应用程序之一不完整，则不完整
  else
    return TRUE;
}

void Read_Flash()
{
  BYTE APP_INFO[12];
  UINT16 APP_StartAddr;
  UINT16 APP_EndAddr;
  BYTE Read_Continue=TRUE;
  UINT16 Read_Length=0;
  UINT16 Read_Addr;
  BYTE APP_DATA[128];
  
  FlashRead(0x2000,APP_INFO,12);
  halWait(200);
  //halWait(200);
  //APP_CalcResult = APP_INFO[5];
  APP_StartAddr = BUILD_UINT16(APP_INFO[9], APP_INFO[8]);
  APP_EndAddr = BUILD_UINT16(APP_INFO[11], APP_INFO[10]);
  Read_Addr = APP_StartAddr;
  
  FlashRead(0,APP_DATA,142);
  //UartTX_Send_String(APP_DATA,142);
  //UartTX_Send_String("CCCC",4);
  
  
  while(Read_Continue)
  {
    FeetDog();
    if((APP_EndAddr-Read_Addr)>=128)
    {
      Read_Length = 128;
    }
    else
    {
      Read_Length = APP_EndAddr-Read_Addr;
      Read_Continue = FALSE;
    }
    FlashRead(Read_Addr,APP_DATA,Read_Length);
    halWait(200);
    
    //UartTX_Send_String(APP_DATA,Read_Length);
    
    //halWait(200);
    //for(i=0;i<Read_Length;i++)
      //Temp_CalcResult += APP_DATA[i];
    Read_Addr += Read_Length;
  }
}
            
