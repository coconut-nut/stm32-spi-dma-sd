#include "sd_nand.h"



uint8_t SD_Type=0;//SDcard type

//define use dma or not
//#define SPI_DMA_SEND_CMD
//#define SPI_DMA_READ_SECTOR
//#define SPI_DMA_WRITE_SECTOR
//if you dont want to use dma,please shut the 3 above

//use at sd initial
void SD_SPI_SpeedLow(void)
{
 	SPI_setspeed(SPI_BAUDRATEPRESCALER_256);//设置到低速模式	
}

//after initial,work highspeed
void SD_SPI_SpeedHigh(void)
{
 	SPI_setspeed(SPI_BAUDRATEPRESCALER_2);//设置到高速模式	
}

uint8_t SD_WaitReady(void)
{printf("[%s] %d:\r\n",__func__,__LINE__);
	uint8_t r1;
	uint16_t retry= 0;
	do
	{
		r1 = SPI_ReadWriteByte(0xFF);
		if(retry==0xfffe)
		{
			return 1;
		}
	}while(r1!=0xFF);
	return 0;
}

//得到回应//Response:要得到的回应值
u8 SD_GetResponse(u8 Response)
{
	u16 Count=0xFFFF;//等待次数	   						  
	while ((SPI_ReadWriteByte(0XFF)!=Response)&&Count)Count--;//等待得到准确的回应  	  
	if (Count==0)return MSD_RESPONSE_FAILURE;//得到回应失败   
	else return MSD_RESPONSE_NO_ERROR;//正确回应
}

//片选
uint8_t SD_CS_ENABLE(void)
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);
	if(SD_WaitReady()==0)return 0;//等待成功
	SD_CS_DISABLE();
	return 1;//等待失败
}

//取消片选
void SD_CS_DISABLE(void)
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);
	SPI_ReadWriteByte(0xff);//提供额外的8个时钟
}



/*******************************************************************************
* Function Name  : SD_SendCommand
* Description    : 向SD卡发送一个命令(结束是不失能片选，还有后续数据传来）
* Input          : uint8_t cmd   命令 
*                  uint32_t arg  命令参数
*                  uint8_t crc   crc校验值
* Output         : None
* Return         : uint8_t r1 SD卡返回的响应
*******************************************************************************/
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    unsigned char r1;
    unsigned char Retry = 0;
		uint8_t rxdata;

    SPI_ReadWriteByte(0xff);
    //片选端置低，选中SD卡
    SD_CS_ENABLE();

    //发送
    #ifndef  SPI_DMA_SEND_CMD
//			SPI_ReadWriteByte(cmd | 0x40);   // 分别写入命令
//			SPI_ReadWriteByte(arg >> 24);
//			SPI_ReadWriteByte(arg >> 16);
//			SPI_ReadWriteByte(arg >> 8);
//			SPI_ReadWriteByte(arg);	  
//			SPI_ReadWriteByte(crc);
			uint8_t txdata[6];
			txdata[0] = (uint8_t) (cmd | 0x40);
			txdata[1] = (uint8_t) (arg >> 24);
			txdata[2] = (uint8_t) (arg >> 16);
			txdata[3] = (uint8_t) (arg >> 8);
			txdata[4] = (uint8_t) (arg);
			txdata[5] = (uint8_t) (crc);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[0],&rxdata,1,10);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[1],&rxdata,1,10);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[2],&rxdata,1,10);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[3],&rxdata,1,10);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[4],&rxdata,1,10);
			HAL_SPI_TransmitReceive(&hspi1,&txdata[5],&rxdata,1,10);
		#else
			uint8_t txdata[6];
			txdata[0] = (uint8_t) (cmd | 0x40);
			txdata[1] = (uint8_t) (arg >> 24);
			txdata[2] = (uint8_t) (arg >> 16);
			txdata[3] = (uint8_t) (arg >> 8);
			txdata[4] = (uint8_t) (arg);
			txdata[5] = (uint8_t) (crc);
			SD_WriteBuffer_DMA(txdata, 6);
		#endif

    //等待响应，或超时退出
    while((r1 = SPI_ReadWriteByte(0xFF))==0xFF)
    {
        Retry++;
        if(Retry > 200)
        {
            break;
        }
    }
    //返回响应值
    return r1;
}

/*******************************************************************************
* Function Name  : SD_Init
* Description    : 初始化SD卡
* Input          : None
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  99：NO_CARD
*******************************************************************************/
uint8_t SD_Init(void)
{
    uint16_t i;      // 用来循环计数
    uint8_t r1;      // 存放SD卡的返回值
    uint16_t retry;  // 用来进行超时计数
    uint8_t buff[6];

    //SD卡上电
    //SD_PWR_ON();
    // 纯延时，等待SD卡上电完成
    //for(i=0;i<0xf00;i++);

    //先产生>74个脉冲，让SD卡自己初始化完成
    for(i=0;i<10;i++)
    {
        SPI_ReadWriteByte(0xFF);
    }

    //-----------------SD卡复位到idle开始-----------------
    //循环连续发送CMD0，直到SD卡返回0x01,进入IDLE状态
    //超时则直接退出
    retry = 0;
    do
    {
        //发送CMD0，让SD卡进入IDLE状态
        r1 = SD_SendCommand(CMD0, 0, 0x95);
        retry++;
    }while((r1 != 0x01) && (retry<200));
    //跳出循环后，检查原因：初始化成功？or 重试超时？
    if(retry==200)
    {
        return 1;   //超时返回1
    }
    //-----------------SD卡复位到idle结束-----------------



    //获取卡片的SD版本信息
    r1 = SD_SendCommand(8, 0x1aa, 0x87);

    //如果卡片版本信息是v1.0版本的，即r1=0x05，则进行以下初始化
    if(r1 == 0x05)
    {
        //设置卡类型为SDV1.0，如果后面检测到为MMC卡，再修改为MMC
        SD_Type = SD_TYPE_V1;

        //如果是V1.0卡，CMD8指令后没有后续数据
        //片选置高，结束本次命令
        SD_CS_DISABLE();
        //多发8个CLK，让SD结束后续操作
        SPI_ReadWriteByte(0xFF);

        //-----------------SD卡、MMC卡初始化开始-----------------

        //发卡初始化指令CMD55+ACMD41
        // 如果有应答，说明是SD卡，且初始化完成
        // 没有回应，说明是MMC卡，额外进行相应初始化
        retry = 0;
        do
        {
            //先发CMD55，应返回0x01；否则出错
            r1 = SD_SendCommand(CMD55, 0, 0);
            if(r1 != 0x01)
            {
                return r1;  
            }
            //得到正确响应后，发ACMD41，应得到返回值0x00，否则重试200次
            r1 = SD_SendCommand(ACMD41, 0, 0);
            retry++;
        }while((r1!=0x00) && (retry<400));
        // 判断是超时还是得到正确回应
        // 若有回应：是SD卡；没有回应：是MMC卡
        
        //----------MMC卡额外初始化操作开始------------
        if(retry==400)
        {
            retry = 0;
            //发送MMC卡初始化命令（没有测试）
            do
            {
                r1 = SD_SendCommand(1, 0, 0);
                retry++;
            }while((r1!=0x00)&& (retry<400));
            if(retry==400)
            {
                return 1;   //MMC卡初始化超时
            }
            //写入卡类型
            SD_Type = SD_TYPE_MMC;
        }
        //----------MMC卡额外初始化操作结束------------
        
        //设置SPI为高速模式
        SD_SPI_SpeedHigh();

		SPI_ReadWriteByte(0xFF);
        
        //禁止CRC校验

//		r1 = SD_SendCommand(CMD59, 0, 0x01);
//        if(r1 != 0x00)
//        {
//            return r1;  //命令错误，返回r1
//        }
  
        //设置Sector Size
        r1 = SD_SendCommand(CMD16, 512, 0xff);
        if(r1 != 0x00)
        {
            return r1;  //命令错误，返回r1
        }
        //-----------------SD卡、MMC卡初始化结束-----------------

    }//SD卡为V1.0版本的初始化结束
    

    //下面是V2.0卡的初始化
    //其中需要读取OCR数据，判断是SD2.0还是SD2.0HC卡
    else if(r1 == 0x01)
    {
        //V2.0的卡，CMD8命令后会传回4字节的数据，要跳过再结束本命令
        buff[0] = SPI_ReadWriteByte(0xFF);  //should be 0x00
        buff[1] = SPI_ReadWriteByte(0xFF);  //should be 0x00
        buff[2] = SPI_ReadWriteByte(0xFF);  //should be 0x01
        buff[3] = SPI_ReadWriteByte(0xFF);  //should be 0xAA
     
        SD_CS_DISABLE();
        //the next 8 clocks
        SPI_ReadWriteByte(0xFF);
        
        //判断该卡是否支持2.7V-3.6V的电压范围
        if(buff[2]==0x01 && buff[3]==0xAA)
        {
            //支持电压范围，可以操作
            retry = 0;
            //发卡初始化指令CMD55+ACMD41
    		do
    		{
    			r1 = SD_SendCommand(CMD55, 0, 0);
    			if(r1!=0x01)
    			{
    				return r1;
    			}
    			r1 = SD_SendCommand(ACMD41, 0x40000000, 0);
                if(retry>200)   
                {
                    return r1;  //超时则返回r1状态
                }
            }while(r1!=0);
          
            //初始化指令发送完成，接下来获取OCR信息

            //-----------鉴别SD2.0卡版本开始-----------
            r1 = SD_SendCommand(CMD58, 0, 0);
            if(r1!=0x00)
            {
                return r1;  //如果命令没有返回正确应答，直接退出，返回应答
            }
            //读OCR指令发出后，紧接着是4字节的OCR信息
            buff[0] = SPI_ReadWriteByte(0xFF);
            buff[1] = SPI_ReadWriteByte(0xFF); 
            buff[2] = SPI_ReadWriteByte(0xFF);
            buff[3] = SPI_ReadWriteByte(0xFF);

            //OCR接收完成，片选置高
            SD_CS_DISABLE();
            SPI_ReadWriteByte(0xFF);

            //检查接收到的OCR中的bit30位（CCS），确定其为SD2.0还是SDHC
            //如果CCS=1：SDHC   CCS=0：SD2.0
            if(buff[0]&0x40)    //检查CCS
            {
                SD_Type = SD_TYPE_V2HC;
            }
            else
            {
                SD_Type = SD_TYPE_V2;
            }
            //-----------鉴别SD2.0卡版本结束-----------

            
            //设置SPI为高速模式
            SD_SPI_SpeedHigh();  
        }

    }
    return r1;
}
//写入一个数据包，内容512字节
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd)
{	
	u16 t;
	u8 rxdata;
	uint8_t crcdata[2]={0xFF,0xFF};
	if(SD_WaitReady())return 1;//等待准备失效
	SPI_ReadWriteByte(cmd);
	if(cmd!=0XFD)//不是结束指令
	{
		#ifndef SPI_DMA_WRITE_SECTOR
			for(t=0;t<512;t++)
//		SPI_ReadWriteByte(buf[t]);//提高速度,减少函数传参时间
		HAL_SPI_TransmitReceive(&hspi1,&buf[t],&rxdata,1,10);
	    SPI_ReadWriteByte(0xFF);//忽略crc
	    SPI_ReadWriteByte(0xFF);
		#else
			SD_WriteBuffer_DMA(buf,512);//
			SD_WriteBuffer_DMA(crcdata,2);//忽略crc
		#endif
		
		t=SPI_ReadWriteByte(0xFF);//接收响应
		if((t&0x1F)!=0x05)return 2;//响应错误									  					    
	}						 									  					    
    return 0;//写入成功
}

/*******************************************************************************
* Function Name  : SD_ReceiveData
* Description    : 从SD卡中读回指定长度的数据，放置在给定位置
* Input          : uint8_t *data(存放读回数据的内存>len)
*                  uint16_t len(数据长度）
*                  uint8_t release(传输完成后是否释放总线CS置高 0：不释放 1：释放）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  other：错误信息
*******************************************************************************/
uint8_t SD_ReceiveData(uint8_t *data, uint16_t len)
{
    // 启动一次传输
	uint8_t order=0xff;
    SD_CS_ENABLE();
	if(SD_GetResponse(0xFE))return 1;//等待SD卡发回数据起始令牌0xFE
    //开始接收数据
    #ifndef  SPI_DMA_READ_SECTOR 
			while (len--)  // 开始接收数据
			{
//				*data = SPI_ReadWriteByte(0xFF);   // 接收数据
				HAL_SPI_TransmitReceive(&hspi1,&order,data,1,10);
				data++;
			}
			SPI_ReadWriteByte(0xFF);   // 忽略CRC
			SPI_ReadWriteByte(0xFF);
		#else
			uint8_t crcdata[2]={0xFF,0xFF};
			SD_ReadBuffer_DMA(data, len);   // 接收数据
			SD_ReadBuffer_DMA(crcdata, 2);  // 忽略CRC
		#endif
    return 0;
}
/*******************************************************************************
* Function Name  : SD_GetCID
* Description    : 获取SD卡的CID信息，包括制造商信息
* Input          : uint8_t *cid_data(存放CID的内存，至少16Byte）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  other：错误信息
*******************************************************************************/
uint8_t SD_GetCID(uint8_t *cid_data)
{
    uint8_t r1;

    //发CMD10命令，读CID
    r1 = SD_SendCommand(CMD10, 0, 0xFF);
    if(r1 != 0x00)
    {
        return r1;  //没返回正确应答，则退出，报错
    }
    //接收16个字节的数据
    SD_ReceiveData(cid_data, 16);

		//传输结束
		SD_CS_DISABLE();
		SPI_ReadWriteByte(0xFF);
		
    return 0;
}
/*******************************************************************************
* Function Name  : SD_GetCSD
* Description    : 获取SD卡的CSD信息，包括容量和速度信息
* Input          : uint8_t *cid_data(存放CID的内存，至少16Byte）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  other：错误信息
*******************************************************************************/
uint8_t SD_GetCSD(uint8_t *csd_data)
{
    uint8_t r1;

    //发CMD9命令，读CSD
    r1 = SD_SendCommand(CMD9, 0, 0x01);
    if(r1 != 0x00)
    {
        return r1;  //没返回正确应答，则退出，报错
    }
    //接收16个字节的数据
    SD_ReceiveData(csd_data, 16);
		SD_CS_DISABLE();
		SPI_ReadWriteByte(0xFF);
    return 0;
}


uint32_t SD_GetCapacity(void)
{
	u8 csd[16];
	u32 Capacity;  
	u8 n;
	u16 csize;  					    
	//取CSD信息，如果期间出错，返回0
	if(SD_GetCSD(csd)!=0) return 0;	    
	//如果为SDHC卡，按照下面方式计算
	if((csd[0]&0xC0)==0x40)	 //V2.00的卡
	{	
	csize = csd[9] + ((u16)csd[8] << 8) + 1;
	Capacity = (u32)csize << 10;//得到扇区数	 		   
	}else//V1.XX的卡
	{	
	n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
	csize = (csd[8] >> 6) + ((u16)csd[7] << 2) + ((u16)(csd[6] & 3) << 10) + 1;
	Capacity= (u32)csize << (n - 9);//得到扇区数   
	}
	return Capacity;
}
/*******************************************************************************
* Function Name  : SD_ReadMultiBlock
* Description    : 读SD卡的多个block
* Input          : uint32_t sector 取地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte）
*                  uint8_t count 连续读count个block
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
uint8_t SD_ReadMultiBlock(uint32_t sector, uint8_t *buffer, uint8_t count)
{
	uint8_t r1;
	//设置为高速模式
	SD_SPI_SpeedHigh();
	//如果不是SDHC，将sector地址转成byte地址
	if(SD_Type!=SD_TYPE_V2HC)
	{
		sector = sector<<9;
	}
	if(count==1)
	{
		r1=SD_SendCommand(CMD17,sector,0X01);//读命令
		if(r1==0)//指令发送成功
		{
			r1=SD_ReceiveData(buffer,512);//接收512个字节	   
		}
	}
	//发读多块命令
	else
	{
		r1=SD_SendCommand(CMD18,sector,0X01);//连续读命令
		do
		{
			r1=SD_ReceiveData(buffer,512);//接收512个字节	 
			buffer+=512;  
		}while(--count && r1==0); 	
		SD_SendCommand(CMD12,0,0X01);	//发送停止命令
	}   
	//释放总线
	SD_CS_DISABLE();
	return r1;
}


/*******************************************************************************
* Function Name  : SD_WriteMultiBlock
* Description    : 写入SD卡的N个block
* Input          : uint32_t sector 扇区地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte）
*                  uint8_t count 写入的block数目
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
uint8_t SD_WriteMultiBlock(uint32_t sector,uint8_t *data, uint8_t count)
{
	u8 r1;
	if(SD_Type!=SD_TYPE_V2HC)sector *= 512;//转换为字节地址
	if(count==1)
	{
		r1=SD_SendCommand(CMD24,sector,0X01);//读命令
		if(r1==0)//指令发送成功
		{
			r1=SD_SendBlock(data,0xFE);//写512个字节	   
		}
	}else
	{
		if(SD_Type!=SD_TYPE_MMC)
		{
			SD_SendCommand(CMD55,0,0X01);	
			SD_SendCommand(ACMD23,count,0X01);//发送指令	
		}
 		r1=SD_SendCommand(CMD25,sector,0X01);//连续读命令
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(data,0xFC);//接收512个字节	 
				data+=512;  
			}while(--count && r1==0);
			r1=SD_SendBlock(0,0xFD);//接收512个字节 
		}
	}   
	SD_CS_DISABLE();//取消片选
	return r1;//
}
