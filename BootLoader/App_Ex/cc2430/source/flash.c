#include "flash.h"

// ��������
WORD PageJudge( INT32 flashAddress , INT16 length ) ;
WORD WriteAddrConvert( INT32 flashAddress );
WORD ReadAddrConvert( INT32 flashAddress );
void writeFlashUsingDMA(BYTE* pSrcAddr, INT16 length, WORD flashAddress, BOOL erase);


volatile DMA_DESC dmaChannel;  //DMA�Ľṹ�����
BYTE PageData[2048];           //�������������洢1ҳ��flash����,��Ҫ�ͷţ�����RAM��������

//flash�洢���ռ��ַ:0x00000~0x1FFFF����128KB
//������0x01000~0x1FFFF��Χ��д����


/**********************************************************************
����ԭ�ͣ�void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length )

����������������������д��flash�С��Ƚ�flashAddress��ַ���ڵ�flashҳ���뻺������
          Ȼ���ڻ�����д�����ݣ����ԭflashҳ�е�����ȫ���������ѻ�����������
          ����д�롣���ǵ��п����漰��ҳ���ϵ�flashд�룬��Ҫ��������ۡ�

��    ����BYTE *pSrcAddr
          ָ����Ҫд��flash�����ݵ�ָ��

          INT32 flashAddress
          ��Ҫд���flash��ַ

          INT16 length
          ��Ҫд�����ݵĳ���
***********************************************************************/
void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length ) @ "FLASH"
{
  //BYTE PageData[2048];
  
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return;//flash��ַ���Ϸ�������
  
  WORD c = PageJudge( flashAddress , length );         //c��ʾ��Ҫ�Ķ���flashҳ����
  
  if( c == 1 )//ֻ��Ҫдһҳflash
  {
    WORD PageNumber = ( flashAddress / 0x800);        //�ж�������ַ�����ĸ�flashҳ
    INT32 PageStart = (INT32)PageNumber * 2048;       //����flashҳ���׵�ַ
    WORD WriteAddress = WriteAddrConvert( PageStart );//flashд���ַת��
    BYTE *p = pSrcAddr;                               //ָ��pָ����Ҫд�������
    INT16 i;
    //error?flashAddress?==>PageStart?
    FlashRead( PageStart , PageData , 2048 );    //������flashҳ�����ݶ��뻺����
    WORD AddrOffset = ( flashAddress % 0x800 );     //����flashAddress��ַ����flashҳ��ƫ����
    
    for( i=0 ; i<length ; i++ )                     //�ڻ������н�����д�뵽ָ��λ��
    {
      PageData[ AddrOffset+i ] = *p;
      p++;
    }//end for
    
    //������������������д��flashҳ��д��ǰ�Ƚ�flashҳ����
    while( FCTL&0x40 );
    writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
  }//end if
  
  else//��Ҫд��ҳ���ϵ�flash
  {
    WORD StartPageNumber = ( flashAddress / 0x800 ); //�ж�������ַ�����ĸ�flashҳ
    WORD NowPageNumber;                              //��ǰflashҳ
    INT32 PageStart;                                 //flashҳ���׵�ַ
    WORD WriteAddress;                               //flashд���ַת��
    BYTE *p = pSrcAddr;                              //ָ��pָ����Ҫд�������
    INT16 i,j;
    
    for( j=0 ; j<c ; j++ )                           //��ҳд��flash����
    {
      NowPageNumber = StartPageNumber + j;
      PageStart = (INT32)NowPageNumber * 2048;
      WriteAddress = WriteAddrConvert( PageStart );
      FlashRead( PageStart , PageData , 2048 );      //������flashҳ�����ݶ��뻺����
      
      if( j==0 )//��Ҫ�Ķ����ǵ�һҳflash
      {
        WORD AddrOffset = ( flashAddress % 0x800 ); //����flashAddress��ַ�ڵ�һҳ��ƫ����
        for( i=0 ; (AddrOffset+i)<=2047 ; i++ )     //�ڻ������н����ݴ�offset��ĩβд��
        {
          PageData[ AddrOffset+i ] = *p;
          p++;
          length--;
        }//end for
        while( FCTL&0x40 );
        writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
      }//end if
      
      else if( j == c-1 )//��Ҫ�Ķ��������һҳflash
      {
        for( i=0 ; length>0 ; length-- )            //�ڻ������дӵ�0��Ԫ�ؿ�ʼд��ֱ������д��
        {
          PageData[ i ] = *p;
          p++;
          i++;
        }//end for
        while( FCTL&0x40 );
        writeFlashUsingDMA( PageData , 2048 , WriteAddress , TRUE );
      }//end else if
      
      else//��Ҫ�Ķ������м�flashҳ
      {
        for( i=0 ; i<=2047 ; i++ )                  //�ڻ�������ȫ��д������
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
����ԭ�ͣ�void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length )

�����������Ӹ���flash��ַ��ʼ��ȡָ�����ȵ����ݡ�flash��ȡ��������Ҫ��flash��
          ������ӳ�䵽code�Σ��ٴ�code���ж������ݡ�����ÿ��ֻ��ӳ��1��bank��
          32KB�����ݣ����Ҫ���ǿ�bank��ȡ���ݵ������

��    ����INT32 flashAddress
          ��ȡflash����ʼ��ַ

          BYTE *Dataout
          ��flash�ж������ݵĴ�ŵ�ַ

          INT16 length
          ��Ҫ��ȡ���ݵĳ���
***********************************************************************/
void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000)
    return;//flash��ַ���Ϸ�������
  
  WORD StartBank =  flashAddress / 0x8000;                //��ʼflash��ַ���ڵ�bank
  WORD EndBank = ( flashAddress+length ) / 0x8000;        //д�����ݺ�flashĩ��ַ����bank
  BYTE __code *ReadAddress;                               //ָ��flash��code��ӳ���ַ��ָ��
  
  if( EndBank==0 || EndBank==1 || (EndBank-StartBank==0))//��0��1��bank����ͬbankʱ����Ҫ����FMAPֵ,ֱ�Ӷ����ݼ���
  {
    ReadAddress = ( BYTE __code *)ReadAddrConvert( flashAddress );
    while(length--)
    {
      *Dataout = *ReadAddress;
      Dataout++;
      ReadAddress++;
    }//end while
  }
  
  else//��bank��flash����(ֻ��bank1��bank2��bank2��bank3�������)����Ҫ�ı�FMAP��ֵ��Ѱַ��ͬbank��Ȼ���ȡ����
  {
    WORD NowBank;   //��ǰ��ַ���ڵ�bank
    BYTE tmp = FMAP;//�����ֳ�
    
    while(length--)
    {
      NowBank = flashAddress / 0x8000;
      FMAP = NowBank;
      ReadAddress = ( BYTE __code *)ReadAddrConvert( flashAddress );
      *Dataout = *ReadAddress;
      Dataout++;
      flashAddress++;
    }//end while
    
    FMAP = tmp;//�ָ��ֳ�
  }//end if
}//end FlashRead


/**********************************************************************
����ԭ�ͣ�void FlashErase( INT32 flashAddress )

������������������flash��ַ���ڵ�flashҳ����������flashҳʱͬ������FADDRH��
          FADDRL��Ѱַ�������Ҫ17λ��16λ��flash��ַת����

��    ����WORD flashAddress
          ��Ҫ������flash��ַ
***********************************************************************/
void FlashErase( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return;//flash��ַ���Ϸ�������
  
  WORD EraseAddress = WriteAddrConvert( flashAddress );
  FLASH_ERASE_PAGE( EraseAddress >> 9 );
  FADDRH = 0x00;
  FADDRL = 0x00;
}//end FlashErase


/**********************************************************************
����ԭ�ͣ�WORD PageJudge( INT32 flashAddress , INT16 length )

������������length��������д����flashAddressΪ��ʼ��ַ�Ĵ洢���У�����flash��
          ��ҳ�ֿ飬�п���length�������ݴ��������flashҳ��Ҳ�п���length����
          ������2048���˺��������ж���Ҫ�Ķ����ٸ�flashҳ��д�����ݡ�

��    ����INT32 flashAddress
          ��Ҫд���flash��ַ

          INT16 length
          ��Ҫд������ݳ���

��    ��: flashд����ʱ�漰�޸ĵ�flashҳ����
***********************************************************************/
static WORD PageJudge( INT32 flashAddress , INT16 length ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash��ַ���Ϸ�������
  
  WORD StartPageNum = ( flashAddress / 0x800 );        //�ж�������ַ�����ĸ�flashҳ
  WORD EndPageNum = ( (flashAddress+length) / 0x800 ); //�ж�д��ȫ�����ݺ�λ���ĸ�flashҳ
  return( EndPageNum - StartPageNum + 1 );
}//end PageJudge


/**********************************************************************
����ԭ�ͣ�WORD WriteAddrConvert( INT32 flashAddress )

��������������flash�洢����128KB����Ҫ17λ��ַ����ʾ����д����ʱ��ͨ��
          FADDRH:FADDRL��16λѰַ��������߼���Ҫһ��ת������������17λ
          ��flash��ַת��Ϊ16λ��ַ��������flashд��Ͳ���������Ѱַ��

��    ����INT32 flashAddress
          ��Ҫд���17λflash��ַ

��    ��: 16λ��flash��ַ=FAADRH:FADDRL
***********************************************************************/
static WORD WriteAddrConvert( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash��ַ���Ϸ�������
  
  WORD WriteAddr = 0x00;
  WORD PageNum = flashAddress / 0x800;                  //flashҳ��λ��1ҳ=2048�ֽ�
  WORD LineNum = ( flashAddress % 0x800 ) / 256;        //ҳ���ж�λ��1��=256�ֽ�
  WORD BlockNum = (( flashAddress % 0x800 ) % 256) / 4; //���ڿ鶨λ��1��=4�ֽ�
  WriteAddr |= ( PageNum << 9 );
  WriteAddr |= ( LineNum << 6 );
  WriteAddr |= BlockNum;
  return ( WriteAddr );
}//end WriteAddrConvert


/**********************************************************************
����ԭ�ͣ�WORD ReadAddrConvert( INT32 flashAddress )

��������������flash�洢����128KB����code��ֻ����ʾ64KB�����ݣ�flash�����޷�
          ȫ����ʾ��code���У�������߼���Ҫһ��ӳ�䡣��flash��ַ�ֳ�4��bank��
          ÿ��bank32KB���ԼĴ���FMAP��Ѱַbank���Ƚ���Ҫ��ȡ���������ڵ�bank
          ӳ�䵽code���У��ٴ�code���ж������ݡ���������17λflash��ַת��Ϊ
          code���е�16λ��ַ��������flash��ȡ������Ѱַ��

��    ����INT32 flashAddress
          ��Ҫ��ȡ��flash��ַ

��    ��: ӳ�䵽code�εĵ�ַ
***********************************************************************/
static WORD ReadAddrConvert( INT32 flashAddress ) @ "FLASH"
{
  if( flashAddress < 0x00000 || flashAddress >= 0x20000 )
    return (0);//flash��ַ���Ϸ�������
  
  WORD BankNum = flashAddress / 0x8000;
  switch( BankNum )
  {
    case 0:                                    //bank0Ѱַ��Χ��0x00000~0x07FFF
      return( (WORD)flashAddress );            //ӳ��Ϊcode��0x0000~0x7FFF
    case 1:                                    //bank1Ѱַ��Χ��0x08000~0x0FFFF
      FMAP = 0x1;
      return( (WORD)flashAddress );            //ӳ��Ϊcode��0x8000~0xFFFF
    case 2:                                    //bank2Ѱַ��Χ��0x10000~0x17FFF
      FMAP = 0x2;
      return( (WORD)(flashAddress-0x8000) );   //ӳ��Ϊcode��0x8000~0xFFFF
    case 3:                                    //bank3Ѱַ��Χ��0x18000~0x1FFFF
      FMAP = 0x3;
      return( (WORD)(flashAddress-0x10000) );  //ӳ��Ϊcode��0x8000~0xFFFF
    default:
      return (0);                              //flash��ַ���Ϸ�
  }//end switch
}//end ReadAddrConvert

/******************************************************************************
* @fn  writeFlashUsingDMA
*
* @brief
*      Writes data to flash using DMA. Erases the page in advance if told to.
*      ʹ��DMAдflash �������Ҫ�Ļ����Բ���ҳ��

* Parameters:
*
* @param  BYTE* pSrcAddr
*         The start of the data to be written to flash.
*         д��flash���������

*         INT16 length
*         The number of bytes to be written to flash.
*         д��flash�����ݳ���

*         WORD flashAddress
*         The address in flash the data is to be written to.
*         flashд��ĵ�ַ

*         BOOL erase
*         Indicating whether the flash is to be erased or not.
*         �Ƿ����flashҳ
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

   // ����flash��ַ
   SET_WORD( FADDRH , FADDRL , WriteAddress );

   // ��֤����4�ֽڵ�������
   while(length & 0x0003){
      length++;
   }//end while

   SET_WORD( dmaChannel.SRCADDRH , dmaChannel.SRCADDRL ,   pSrcAddr );   // The start address of the segment
   SET_WORD( dmaChannel.DESTADDRH , dmaChannel.DESTADDRL , &X_FWDATA );  // Input of the AES module
   SET_WORD( dmaChannel.LENH , dmaChannel.LENL , length );               // Setting the length of the transfer (bytes)
   dmaChannel.VLEN      = VLEN_USE_LEN;      // ѡ��LEN��Ϊ���ͳ���
   dmaChannel.PRIORITY  = PRI_HIGH;          // �������ȼ�Ϊ��
   dmaChannel.M8        = M8_USE_8_BITS;     // ѡ��8λ�����ֽ�����������
   dmaChannel.IRQMASK   = FALSE;             // DMA����жϱ�־
   dmaChannel.DESTINC   = DESTINC_0;         // �̶���Ŀ�ĵ�ַ
   dmaChannel.SRCINC    = SRCINC_1;          // Դ��ַ����Ϊ1�ֽ�
   dmaChannel.TRIG      = DMATRIG_FLASH;     // ����FLASHģ�鴥��DMA
   dmaChannel.TMODE     = TMODE_SINGLE;      // ÿ�δ���һ���ֽ�
   dmaChannel.WORDSIZE  = WORDSIZE_BYTE;     // Set to count bytes.

   // ��������ã�������DMA
   // ���DMA��ɱ�־
   oldPointerH = DMA0CFGH;
   oldPointerL = DMA0CFGL;
   DMA_SET_ADDR_DESC0( &dmaChannel );
   DMA_ABORT_CHANNEL(0);
   DMAIRQ &= ~DMA_CHANNEL_0;
   DMA_ARM_CHANNEL(0);

   // ��ʼдflash
   if( erase == TRUE )
      FLASH_ERASE_PAGE( WriteAddress >> 9 );//д֮ǰ�Ƚ�flashҳ����
   halWait( 0x8F );
   FLASH_CONFIG( WRITE );

   // �ȴ�DMA���
   while( !(DMAIRQ & DMA_CHANNEL_0) );//�ȴ�FLASH����DMA�жϱ�־DMAIF0=1
   DMAIRQ &= ~DMA_CHANNEL_0;        //����DMA�жϱ�־��DMAIF0=0

   // ����ԭ��������
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
