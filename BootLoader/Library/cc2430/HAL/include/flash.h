#include "ioCC2430.h"
#include "hal.h"

//flash�洢���ռ��ַ:0x00000~0x1FFFF����128KB
//������0x01000~0x1FFFF��Χ��д����

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
void FlashRead( INT32 flashAddress , BYTE *Dataout , INT16 length );



/**********************************************************************
����ԭ�ͣ�void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length )

����������������������д��flash�С��Ƚ�flashAddress��ַ���ڵ�flashҳ���뻺������
          Ȼ���ڻ�����д�����ݣ����ԭflashҳ�е�����ȫ���������ѻ�����������
          ����д�롣���ǵ��п����漰��ҳ���ϵ�flashд�룬��Ҫ��������ۡ�

��    ����BYTE *pSrcAddr
          ָ����Ҫд��flash�е�����

          INT32 flashAddress
          ��Ҫд���flash��ַ

          INT16 length
          ��Ҫд�����ݵĳ���
***********************************************************************/
void FlashWrite( BYTE *pSrcAddr , INT32 flashAddress , INT16 length );



/**********************************************************************
����ԭ�ͣ�void FlashErase( INT32 flashAddress )

������������������flash��ַ���ڵ�flashҳ����������flashҳʱͬ������FADDRH��
          FADDRL��Ѱַ�������Ҫ17λ��16λ��flash��ַת����

��    ����WORD flashAddress
          ��Ҫ������flash��ַ
***********************************************************************/
void FlashErase( INT32 flashAddress );
