#include "flash.h"

// 函数声明
WORD PageJudge( INT32 flashAddress , INT16 length ) ;
WORD WriteAddrConvert( INT32 flashAddress );
WORD ReadAddrConvert( INT32 flashAddress );
void writeFlashUsingDMA(BYTE* pSrcAddr, INT16 length, WORD flashAddress, BOOL erase);


volatile DMA_DESC dmaChannel;  //DMA的结构体变量
BYTE PageData[2048];           //缓存区，用来存储1页的flash数据,需要释放，否则RAM满，卡死

//flash存储器空间地址:0x00000~0x1FFFF，共128KB
//建议在0x01000~0x1FFFF范围内写数据


/**********************************************************************
函数原型：void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length )

功能描述：将给定的数据写入flash中。先将flashAddress地址所在的flash页读入缓存区，
          然后在缓存区写入数据，最后将原flash页中的内容全部擦除，把缓存区的内容
          重新写入。考虑到有可能涉及两页以上的flash写入，需要分情况讨论。

参    数：BYTE *pSrcAddr
          指向需要写入flash中数据的指针

          INT32 flashAddress
          需要写入的flash地址

          INT16 length
          需要写入数据的长度
***********************************************************************/
void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length ) @ "FLASH"
{
  //BYTE PageData[2048];
  
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return;//flash地址不合法，返回
  
  WORD c = PageJudge( flashAddress , length );         //c表示需要改动的flash页个数
  
  if( c == 1 )//只需要写一页flash
  {
    WORD PageNumber = ( flashAddress / 0x800);        //判断所给地址属于哪个flash页
    INT32 PageStart = (INT32)PageNumber * 2048;       //计算flash页的首地址
    WORD WriteAddress = WriteAddrConvert( PageStart );//flash写入地址转换
    BYTE *p = pSrcAddr;                               //指针p指向需要写入的数据
    INT16 i;
    //error?flashAddress?==>PageStart?
    FlashRead( PageStart , PageData , 2048 );    //将整个flash页的内容读入缓冲区
    WORD AddrOffset = ( flashAddress % 0x800 );     //计算flashAddress地址在其flash页中偏移量
    
    for( i=0 ; i<length ; i++ )                     //在缓存区中将数据写入到指定位置
    {
      PageData[ AddrOffset+i ] = *p;
      p++;
    }//end for
    
    //将缓存区中数据重新写入flash页，写入前先将flash页擦除
    while( FCTL&0x40 );
    writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
  }//end if
  
  else//需要写两页以上的flash
  {
    WORD StartPageNumber = ( flashAddress / 0x800 ); //判断所给地址属于哪个flash页
    WORD NowPageNumber;                              //当前flash页
    INT32 PageStart;                                 //flash页的首地址
    WORD WriteAddress;                               //flash写入地址转换
    BYTE *p = pSrcAddr;                              //指针p指向需要写入的数据
    INT16 i,j;
    
    for( j=0 ; j<c ; j++ )                           //分页写入flash数据
    {
      NowPageNumber = StartPageNumber + j;
      PageStart = (INT32)NowPageNumber * 2048;
      WriteAddress = WriteAddrConvert( PageStart );
      FlashRead( PageStart , PageData , 2048 );      //将整个flash页的内容读入缓冲区
      
      if( j==0 )//需要改动的是第一页flash
      {
        WORD AddrOffset = ( flashAddress % 0x800 ); //计算flashAddress地址在第一页中偏移量
        for( i=0 ; (AddrOffset+i)<=2047 ; i++ )     //在缓存区中将数据从offset到末尾写满
        {
          PageData[ AddrOffset+i ] = *p;
          p++;
          length--;
        }//end for
        while( FCTL&0x40 );
        writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
      }//end if
      
      else if( j == c-1 )//需要改动的是最后一页flash
      {
        for( i=0 ; length>0 ; length-- )            //在缓冲区中从第0个元素开始写，直到数据写完
        {
          PageData[ i ] = *p;
          p++;
          i++;
        }//end for
        while( FCTL&0x40 );
        writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
      }//end else if
      
      else//需要改动的是中间flash页
      {
        for( i=0 ; i<=2047 ; i++ )                  //在缓冲区中全部写满数据
        {
          PageData[ i ] = *p;
          p++;
          length--;
        }//end for
        while( FCTL&0x40 );
        writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
      }//end else
    }//end for
  }//end if's else
}//end FlashWrite


/**********************************************************************
函数原型：void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length )

功能描述：从给定flash地址开始读取指定长度的数据。flash读取过程中先要将flash中
          的数据映射到code段，再从code段中读出数据。由于每次只能映射1个bank即
          32KB的数据，因此要考虑跨bank读取数据的情况。

参    数：INT32 flashAddress
          读取flash的起始地址

          BYTE *Dataout
          从flash中读出数据的存放地址

          INT16 length
          需要读取数据的长度
***********************************************************************/
void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000)
    return;//flash地址不合法，返回
  
  WORD StartBank =  flashAddress / 0x8000;                //起始flash地址所在的bank
  WORD EndBank = ( flashAddress+length ) / 0x8000;        //写完数据后flash末地址所在bank
  BYTE __code *ReadAddress;                               //指向flash在code段映射地址的指针
  
  if( EndBank==0 || EndBank==1 || (EndBank-StartBank==0))//第0、1个bank和相同bank时不需要考虑FMAP值,直接读数据即可
  {
    ReadAddress = ( BYTE __code *)ReadAddrConvert( flashAddress );
    while(length--)
    {
      *Dataout = *ReadAddress;
      Dataout++;
      ReadAddress++;
    }//end while
  }
  
  else//跨bank读flash数据(只有bank1跨bank2、bank2跨bank3两种情况)，需要改变FMAP的值以寻址不同bank，然后读取数据
  {
    WORD NowBank;   //当前地址所在的bank
    BYTE tmp = FMAP;//保护现场
    
    while(length--)
    {
      NowBank = flashAddress / 0x8000;
      FMAP = NowBank;
      ReadAddress = ( BYTE __code *)ReadAddrConvert( flashAddress );
      *Dataout = *ReadAddress;
      Dataout++;
      flashAddress++;
    }//end while
    
    FMAP = tmp;//恢复现场
  }//end if
}//end FlashRead


/**********************************************************************
函数原型：void FlashErase( INT32 flashAddress )

功能描述：将给定的flash地址所在的flash页擦除。擦除flash页时同样是以FADDRH和
          FADDRL来寻址，因此需要17位到16位的flash地址转换。

参    数：WORD flashAddress
          需要擦除的flash地址
***********************************************************************/
void FlashErase( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return;//flash地址不合法，返回
  
  WORD EraseAddress = WriteAddrConvert( flashAddress );
  FLASH_ERASE_PAGE( EraseAddress >> 9 );
  FADDRH = 0x00;
  FADDRL = 0x00;
}//end FlashErase


/**********************************************************************
函数原型：WORD PageJudge( INT32 flashAddress , INT16 length )

功能描述：将length长的数据写入以flashAddress为起始地址的存储器中，由于flash是
          按页分块，有可能length长的数据处跨过两个flash页，也有可能length长度
          超过了2048，此函数用来判断需要改动多少个flash页来写入数据。

参    数：INT32 flashAddress
          需要写入的flash地址

          INT16 length
          需要写入的数据长度

返    回: flash写操作时涉及修改的flash页个数
***********************************************************************/
static WORD PageJudge( INT32 flashAddress , INT16 length ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash地址不合法，返回
  
  WORD StartPageNum = ( flashAddress / 0x800 );        //判断所给地址属于哪个flash页
  WORD EndPageNum = ( (flashAddress+length) / 0x800 ); //判断写完全部数据后位于哪个flash页
  return( EndPageNum - StartPageNum + 1 );
}//end PageJudge


/**********************************************************************
函数原型：WORD WriteAddrConvert( INT32 flashAddress )

功能描述：由于flash存储器是128KB，需要17位地址来表示，而写操作时仅通过
          FADDRH:FADDRL即16位寻址，因此两者间需要一个转换。本函数将17位
          的flash地址转换为16位地址，可用于flash写入和擦除过程中寻址。

参    数：INT32 flashAddress
          需要写入的17位flash地址

返    回: 16位的flash地址=FAADRH:FADDRL
***********************************************************************/
static WORD WriteAddrConvert( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash地址不合法，返回
  
  WORD WriteAddr = 0x00;
  WORD PageNum = flashAddress / 0x800;                  //flash页定位，1页=2048字节
  WORD LineNum = ( flashAddress % 0x800 ) / 256;        //页内行定位，1行=256字节
  WORD BlockNum = (( flashAddress % 0x800 ) % 256) / 4; //行内块定位，1块=4字节
  WriteAddr |= ( PageNum << 9 );
  WriteAddr |= ( LineNum << 6 );
  WriteAddr |= BlockNum;
  return ( WriteAddr );
}//end WriteAddrConvert


/**********************************************************************
函数原型：WORD ReadAddrConvert( INT32 flashAddress )

功能描述：由于flash存储器是128KB，而code段只能显示64KB的内容，flash数据无法
          全部显示在code段中，因此两者间需要一个映射。将flash地址分成4个bank，
          每个bank32KB，以寄存器FMAP来寻址bank，先将需要读取的数据所在的bank
          映射到code段中，再从code段中读出数据。本函数将17位flash地址转换为
          code段中的16位地址，可用于flash读取过程中寻址。

参    数：INT32 flashAddress
          需要读取的flash地址

返    回: 映射到code段的地址
***********************************************************************/
static WORD ReadAddrConvert( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash地址不合法，返回
  
  WORD BankNum = flashAddress / 0x8000;
  switch( BankNum )
  {
    case 0:                                    //bank0寻址范围：0x00000~0x07FFF
      return( (WORD)flashAddress );            //映射为code段0x0000~0x7FFF
    case 1:                                    //bank1寻址范围：0x08000~0x0FFFF
      FMAP = 0x1;
      return( (WORD)flashAddress );            //映射为code段0x8000~0xFFFF
    case 2:                                    //bank2寻址范围：0x10000~0x17FFF
      FMAP = 0x2;
      return( (WORD)(flashAddress-0x8000) );   //映射为code段0x8000~0xFFFF
    case 3:                                    //bank3寻址范围：0x18000~0x1FFFF
      FMAP = 0x3;
      return( (WORD)(flashAddress-0x10000) );  //映射为code段0x8000~0xFFFF
    default:
      return (0);                              //flash地址不合法
  }//end switch
}//end ReadAddrConvert

/******************************************************************************
* @fn  writeFlashUsingDMA
*
* @brief
*      Writes data to flash using DMA. Erases the page in advance if told to.
*      使用DMA写flash ，如果需要的话可以擦除页面

* Parameters:
*
* @param  BYTE* pSrcAddr
*         The start of the data to be written to flash.
*         写入flash的数据起点

*         INT16 length
*         The number of bytes to be written to flash.
*         写入flash的数据长度

*         WORD flashAddress
*         The address in flash the data is to be written to.
*         flash写入的地址

*         BOOL erase
*         Indicating whether the flash is to be erased or not.
*         是否擦除flash页
* @return void
*
******************************************************************************/
static void writeFlashUsingDMA( BYTE* pSrcAddr , INT16 length , WORD WriteAddress , BOOL erase ) @ "FLASH"
{
   BYTE oldPointerH = 0;
   BYTE oldPointerL = 0;
   BYTE status;

   status = IEN0;
   INT_GLOBAL_ENABLE( INT_OFF );

   // 设置flash地址
   SET_WORD( FADDRH , FADDRL , WriteAddress );

   // 保证传输4字节的整数倍
   while(length & 0x0003){
      length++;
   }//end while

   SET_WORD( dmaChannel.SRCADDRH , dmaChannel.SRCADDRL ,   pSrcAddr );   // The start address of the segment
   SET_WORD( dmaChannel.DESTADDRH , dmaChannel.DESTADDRL , &X_FWDATA );  // Input of the AES module
   SET_WORD( dmaChannel.LENH , dmaChannel.LENL , length );               // Setting the length of the transfer (bytes)
   dmaChannel.VLEN      = VLEN_USE_LEN;      // 选择LEN作为传送长度
   dmaChannel.PRIORITY  = PRI_HIGH;          // 传输优先级为高
   dmaChannel.M8        = M8_USE_8_BITS;     // 选择8位长的字节来传送数据
   dmaChannel.IRQMASK   = FALSE;             // DMA完成中断标志
   dmaChannel.DESTINC   = DESTINC_0;         // 固定的目的地址
   dmaChannel.SRCINC    = SRCINC_1;          // 源地址增量为1字节
   dmaChannel.TRIG      = DMATRIG_FLASH;     // 设置FLASH模块触发DMA
   dmaChannel.TMODE     = TMODE_SINGLE;      // 每次传输一个字节
   dmaChannel.WORDSIZE  = WORDSIZE_BYTE;     // Set to count bytes.

   // 储存旧设置，并设置DMA
   // 清除DMA完成标志
   oldPointerH = DMA0CFGH;
   oldPointerL = DMA0CFGL;
   DMA_SET_ADDR_DESC0( &dmaChannel );
   DMA_ABORT_CHANNEL(0);
   DMAIRQ &= ~DMA_CHANNEL_0;
   DMA_ARM_CHANNEL(0);

   // 开始写flash
   if( erase == TRUE )
      FLASH_ERASE_PAGE( WriteAddress >> 9 );//写之前先将flash页擦除
   halWait( 0x8F );
   FLASH_CONFIG( WRITE );

   // 等待DMA完成
   while( !(DMAIRQ & DMA_CHANNEL_0) );//等待FLASH给出DMA中断标志DMAIF0=1
   DMAIRQ &= ~DMA_CHANNEL_0;        //重置DMA中断标志，DMAIF0=0

   // 保存原来的设置
   DMA0CFGH = oldPointerH;
   DMA0CFGL = oldPointerL;
   IEN0 = status;
  
   //*PageData=NULL;//release the PageData memory---not useful
   
   return;
}//end writeFlashUsingDMA


/******************************************************************************
* @fn  halWait
*
* @brief
*      This function waits approximately a given number of m-seconds
*      regardless of main clock speed.
*
* Parameters:
*
* @param  BYTE	 wait
*         The number of m-seconds to wait.
*
* @return void
*
****************************************************************************
void halWait( BYTE wait )
{
   UINT32 largeWait;

   if( wait == 0 )
   {return;}
   largeWait = ( (UINT16)(wait << 7) );
   largeWait += 114 * wait;
   largeWait = ( largeWait >> CLKSPD );
   while( largeWait-- );
   return;
}//end halWait*/
