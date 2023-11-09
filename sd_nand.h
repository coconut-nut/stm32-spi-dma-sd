#ifndef _sd_nand_H
#define _sd_nand_H

#include "spi.h"
#include "usart.h"

// SD卡类型定义  
#define SD_TYPE_MMC     0
#define SD_TYPE_V1      1
#define SD_TYPE_V2      2
#define SD_TYPE_V2HC    4

/* SPI总线速度设置*/
#define SPI_SPEED_LOW   0
#define SPI_SPEED_HIGH  1

/* SD传输数据结束后是否释放总线宏定义 */
#define NO_RELEASE      0
#define RELEASE         1

#define A    0
#define B    1
   
// SD卡指令表  	   
#define CMD0    0       //卡复位
#define CMD9    9       //命令9 ，读CSD数据
#define CMD10   10      //命令10，读CID数据
#define CMD12   12      //命令12，停止数据传输
#define CMD16   16      //命令16，设置SectorSize 应返回0x00
#define CMD17   17      //命令17，读sector
#define CMD18   18      //命令18，读Multi sector
#define ACMD23  23      //命令23，设置多sector写入前预先擦除N个block
#define CMD24   24      //命令24，写sector
#define CMD25   25      //命令25，写Multi sector
#define ACMD41  41      //命令41，应返回0x00
#define CMD55   55      //命令55，应返回0x01
#define CMD58   58      //命令58，读OCR信息
#define CMD59   59      //命令59，使能/禁止CRC，应返回0x00

//数据写入回应字意义
#define MSD_DATA_OK                0x05
#define MSD_DATA_CRC_ERROR         0x0B
#define MSD_DATA_WRITE_ERROR       0x0D
#define MSD_DATA_OTHER_ERROR       0xFF

//SD卡回应标记字
#define MSD_RESPONSE_NO_ERROR      0x00
#define MSD_IN_IDLE_STATE          0x01
#define MSD_ERASE_RESET            0x02
#define MSD_ILLEGAL_COMMAND        0x04
#define MSD_COM_CRC_ERROR          0x08
#define MSD_ERASE_SEQUENCE_ERROR   0x10
#define MSD_ADDRESS_ERROR          0x20
#define MSD_PARAMETER_ERROR        0x40
#define MSD_RESPONSE_FAILURE       0xFF 							   						 					    	  



#define SD_DET()            1     //!GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2) //检测有卡
                                                                    //1-有 0-无

extern uint8_t SD_Type;//SD卡的类型
//函数申明区 
void SPI1_Configuration(void);
void SPI_SetSpeed(uint8_t SpeedSet);

uint8_t SD_WaitReady(void);                          //等待SD卡就绪
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc);     //SD卡发送一个命令
//uint8_t SD_SendCommand_NoDeassert(uint8_t cmd, uint32_t arg, uint8_t crc);
uint8_t SD_Init(void);                               //SD卡初始化
uint8_t SD_GetResponse(uint8_t Response);

uint8_t SD_ReceiveData(uint8_t *data, uint16_t len);//SD卡读数据
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd);

uint8_t SD_GetCID(uint8_t *cid_data);                     //读SD卡CID
uint8_t SD_GetCSD(uint8_t *csd_data);                     //读SD卡CSD
uint32_t SD_GetCapacity(void);                       //取SD卡容量

//uint8_t SD_ReadSingleBlock(uint32_t sector, uint8_t *buffer);  //读一个sector
//uint8_t SD_WriteSingleBlock(uint32_t sector, const uint8_t *buffer); //写一个sector
uint8_t SD_ReadMultiBlock(uint32_t sector, uint8_t *buffer, uint8_t count); //读多个sector
uint8_t SD_WriteMultiBlock(uint32_t sector,uint8_t *data, uint8_t count);  //写多个sector
void SD_CS_DISABLE(void);
uint8_t SD_CS_ENABLE(void);
#endif


