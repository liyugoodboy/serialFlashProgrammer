//###########################################################################
//
// 文件名称:    Shared_verify.c
//
// 文件说名:    闪存验证功能
//
// 函数说明:
//
//     void VerifyData(void) 数据验证功能  
//
// 支持库: F2837xD Support Library v3.09.00.00
//###########################################################################

/********************************Included***********************************/
#include "c1_bootrom.h"
#include "F021_F2837xD_C28x.h"
#include "boot/flash_programming_c28.h" // Flash API例程头文件

/********************************Defines************************************/
#define NO_ERROR                    0x1000
#define BLANK_ERROR                 0x2000
#define VERIFY_ERROR                0x3000
#define PROGRAM_ERROR               0x4000
#define COMMAND_ERROR               0x5000
#define BUFFER_SIZE                 0x80 //由于ECC，必须是64位或4个字所有BUFFER_SIZE％4 == 0


/****************************** Globals  ************************************/
// GetWordData是指向与外围设备接口的函数的指针。 每个加载器都将此指针分配给它的特定GetWordData函数。
extern uint16fptr GetWordData;    

typedef struct
{
   Uint16 status;
   Uint32 address;
}  StatusCode;
extern StatusCode statusCode;


/************************* Function Prototypes *******************************/
extern void SCI_SendChecksum(void);
void VerifyData(void);
extern Uint32 GetLongData(void);
extern void ReadReservedFn(void);
extern Uint16 SCIA_GetWordData(void);


/******************************************************************************
 * 函数名称:VerifyData
 * 函数说明:
 *         此例程从主机复制多个数据块，并使用设备中的闪存验证该数据。如果验证闪存失
 *         败，则返回错误。将复制多个数据块，直到遇到00 00的块大小为止。
 *****************************************************************************/
void VerifyData()
{
    statusCode.status = NO_ERROR;
    statusCode.address = 0x12346578;

    struct HEADER {
     Uint16 BlockSize;
     Uint32 DestAddr;
    } BlockHeader;

    Uint16 wordData;
    Uint16 i,j,k;
    Uint16 Buffer[BUFFER_SIZE];
    Uint16 miniBuffer[4];
    int fail = 0;

    assert(BUFFER_SIZE % 4 == 0);

    // 
    //将GetWordData分配给该函数的SCI-A版本,GetWordData是指向函数的指针。
    //
    GetWordData = SCIA_GetWordData;

    //
    //如果KeyValue无效，请中止加载并返回Flash入口点,0x08AA为SCI-8格式数据开头两字节
    //
    if(SCIA_GetWordData() != 0x08AA)
    {
        statusCode.status = VERIFY_ERROR;
        statusCode.address = FLASH_ENTRY_POINT;
    }

    ReadReservedFn(); //读取并丢弃SCI-8格式数据中8个保留字

    GetLongData();//获取一个32位数据:应用程序入口地址

    //
    // 在开始之前发送校验和
    //
#if checksum_enable
    SCI_SendChecksum();
#endif

    //
    //获取第一个块的字大小
    //
    BlockHeader.BlockSize = (*GetWordData)();

    //
    //当块大小> 0时，将数据复制到DestAddr。 没有错误检查，因为假定DestAddr是有效的内存位置
    //

    EALLOW;
    while(BlockHeader.BlockSize != (Uint16)0x0000)
    {
       Fapi_StatusType oReturnCheck;
       Fapi_FlashStatusWordType oFlashStatusWord;

       //获取块地址
       BlockHeader.DestAddr = GetLongData();
       //装载BUFFER区
       for(i = 0; i < BlockHeader.BlockSize; i += 0)
       {
           //blockSize比BUFFER_SIZE小，这直接填充
           if(BlockHeader.BlockSize < BUFFER_SIZE)
           {
               for(j = 0; j < BlockHeader.BlockSize; j++)
               {
                   wordData = (*GetWordData)();
                   Buffer[j] = wordData;
                   i++;
               }
               for(j = BlockHeader.BlockSize; j < BUFFER_SIZE; j++)
               {
                   Buffer[j] = 0xFFFF;
               }
           }
           else //BlockHeader.BlockSize >= BUFFER_SIZE
           {
               if((BlockHeader.BlockSize - i) < BUFFER_SIZE) //剩下少于一个BUFFER_SIZE数据量
               {
                   //
                   //填充剩下的数据
                   //
                   for(j = 0; j < BlockHeader.BlockSize - i; j++)
                   {
                       wordData = (*GetWordData)();
                       Buffer[j] = wordData;
                   }

                   i += j; //偏移

                   for(; j < BUFFER_SIZE; j++)//空数据区装载为0xffff
                   {
                       Buffer[j] = 0xFFFF;
                   }
               }
               else
               {
                   //
                   // 当blockSize大于BUFFER_SIZE,填满整个BUFFER_SIZE区
                   //
                   for(j = 0; j < BUFFER_SIZE; j++)
                   {
                       wordData = (*GetWordData)();
                       Buffer[j] = wordData;
                       i++;
                   }
               }
           }
           //烧写单个BUFFER_SIZE大小数据
           for(k = 0; k < (BUFFER_SIZE / 4); k++)
           {
               miniBuffer[0] = Buffer[k * 4 + 0];
               miniBuffer[1] = Buffer[k * 4 + 1];
               miniBuffer[2] = Buffer[k * 4 + 2];
               miniBuffer[3] = Buffer[k * 4 + 3];

               //
               //检查miniBuffer是否不是全部擦除的数据
               //
               if(!((miniBuffer[0] == 0xFFFF) && (miniBuffer[1] == 0xFFFF) &&
                    (miniBuffer[2] == 0xFFFF) && (miniBuffer[3] == 0xFFFF)))
               {
                    //检查flash状态
                    while(Fapi_checkFsmForReady() == Fapi_Status_FsmBusy);

                    for(j = 0; j < 4; j += 2)
                    {
                       Uint32 toVerify = miniBuffer[j+1];
                       toVerify = toVerify << 16;
                       toVerify |= miniBuffer[j];
                       //验证32位数据
                       oReturnCheck = Fapi_doVerify((uint32 *)(BlockHeader.DestAddr+j),
                                                    1,
                                                    (uint32 *)(&toVerify),
                                                    &oFlashStatusWord);
                       if(oReturnCheck != Fapi_Status_Success)
                       {
                           if(fail == 0) //first fail
                           {
                                statusCode.status = VERIFY_ERROR;
                                statusCode.address = oFlashStatusWord.au32StatusWord[0];
                           }
                           fail++;
                       }
                    } //for j; for Fapi_doVerify
               } //check if miniBuffer does not contain all already erased data
               BlockHeader.DestAddr += 0x4;
           } //for(int k); loads miniBuffer with Buffer elements
#if checksum_enable
           //烧写完一个BUFFER_SIZE区后发送校验和
           SCI_SendChecksum();
#endif
       }
        //获取下一个块大小
        BlockHeader.BlockSize = (*GetWordData)();
    }
    EDIS;
    return;
}

//
// End of file
//
