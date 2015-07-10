#include "ioCC2430.h"
#include "hal.h"

//flash存储器空间地址:0x00000~0x1FFFF，共128KB
//建议在0x01000~0x1FFFF范围内写数据

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
void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length );



/**********************************************************************
函数原型：void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length )

功能描述：将给定的数据写入flash中。先将flashAddress地址所在的flash页读入缓存区，
          然后在缓存区写入数据，最后将原flash页中的内容全部擦除，把缓存区的内容
          重新写入。考虑到有可能涉及两页以上的flash写入，需要分情况讨论。

参    数：BYTE *pSrcAddr
          指向需要写入flash中的数据

          INT32 flashAddress
          需要写入的flash地址

          INT16 length
          需要写入数据的长度
***********************************************************************/
void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length );



/**********************************************************************
函数原型：void FlashErase( INT32 flashAddress )

功能描述：将给定的flash地址所在的flash页擦除。擦除flash页时同样是以FADDRH和
          FADDRL来寻址，因此需要17位到16位的flash地址转换。

参    数：WORD flashAddress
          需要擦除的flash地址
***********************************************************************/
void FlashErase( INT32 flashAddress );
