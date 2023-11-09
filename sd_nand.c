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
 	SPI_setspeed(SPI_BAUDRATEPRESCALER_256);//���õ�����ģʽ	
}

//after initial,work highspeed
void SD_SPI_SpeedHigh(void)
{
 	SPI_setspeed(SPI_BAUDRATEPRESCALER_2);//���õ�����ģʽ	
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

//�õ���Ӧ//Response:Ҫ�õ��Ļ�Ӧֵ
u8 SD_GetResponse(u8 Response)
{
	u16 Count=0xFFFF;//�ȴ�����	   						  
	while ((SPI_ReadWriteByte(0XFF)!=Response)&&Count)Count--;//�ȴ��õ�׼ȷ�Ļ�Ӧ  	  
	if (Count==0)return MSD_RESPONSE_FAILURE;//�õ���Ӧʧ��   
	else return MSD_RESPONSE_NO_ERROR;//��ȷ��Ӧ
}

//Ƭѡ
uint8_t SD_CS_ENABLE(void)
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET);
	if(SD_WaitReady()==0)return 0;//�ȴ��ɹ�
	SD_CS_DISABLE();
	return 1;//�ȴ�ʧ��
}

//ȡ��Ƭѡ
void SD_CS_DISABLE(void)
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET);
	SPI_ReadWriteByte(0xff);//�ṩ�����8��ʱ��
}



/*******************************************************************************
* Function Name  : SD_SendCommand
* Description    : ��SD������һ������(�����ǲ�ʧ��Ƭѡ�����к������ݴ�����
* Input          : uint8_t cmd   ���� 
*                  uint32_t arg  �������
*                  uint8_t crc   crcУ��ֵ
* Output         : None
* Return         : uint8_t r1 SD�����ص���Ӧ
*******************************************************************************/
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    unsigned char r1;
    unsigned char Retry = 0;
		uint8_t rxdata;

    SPI_ReadWriteByte(0xff);
    //Ƭѡ���õͣ�ѡ��SD��
    SD_CS_ENABLE();

    //����
    #ifndef  SPI_DMA_SEND_CMD
//			SPI_ReadWriteByte(cmd | 0x40);   // �ֱ�д������
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

    //�ȴ���Ӧ����ʱ�˳�
    while((r1 = SPI_ReadWriteByte(0xFF))==0xFF)
    {
        Retry++;
        if(Retry > 200)
        {
            break;
        }
    }
    //������Ӧֵ
    return r1;
}

/*******************************************************************************
* Function Name  : SD_Init
* Description    : ��ʼ��SD��
* Input          : None
* Output         : None
* Return         : uint8_t 
*                  0��NO_ERR
*                  1��TIME_OUT
*                  99��NO_CARD
*******************************************************************************/
uint8_t SD_Init(void)
{
    uint16_t i;      // ����ѭ������
    uint8_t r1;      // ���SD���ķ���ֵ
    uint16_t retry;  // �������г�ʱ����
    uint8_t buff[6];

    //SD���ϵ�
    //SD_PWR_ON();
    // ����ʱ���ȴ�SD���ϵ����
    //for(i=0;i<0xf00;i++);

    //�Ȳ���>74�����壬��SD���Լ���ʼ�����
    for(i=0;i<10;i++)
    {
        SPI_ReadWriteByte(0xFF);
    }

    //-----------------SD����λ��idle��ʼ-----------------
    //ѭ����������CMD0��ֱ��SD������0x01,����IDLE״̬
    //��ʱ��ֱ���˳�
    retry = 0;
    do
    {
        //����CMD0����SD������IDLE״̬
        r1 = SD_SendCommand(CMD0, 0, 0x95);
        retry++;
    }while((r1 != 0x01) && (retry<200));
    //����ѭ���󣬼��ԭ�򣺳�ʼ���ɹ���or ���Գ�ʱ��
    if(retry==200)
    {
        return 1;   //��ʱ����1
    }
    //-----------------SD����λ��idle����-----------------



    //��ȡ��Ƭ��SD�汾��Ϣ
    r1 = SD_SendCommand(8, 0x1aa, 0x87);

    //�����Ƭ�汾��Ϣ��v1.0�汾�ģ���r1=0x05����������³�ʼ��
    if(r1 == 0x05)
    {
        //���ÿ�����ΪSDV1.0����������⵽ΪMMC�������޸�ΪMMC
        SD_Type = SD_TYPE_V1;

        //�����V1.0����CMD8ָ���û�к�������
        //Ƭѡ�øߣ�������������
        SD_CS_DISABLE();
        //�෢8��CLK����SD������������
        SPI_ReadWriteByte(0xFF);

        //-----------------SD����MMC����ʼ����ʼ-----------------

        //������ʼ��ָ��CMD55+ACMD41
        // �����Ӧ��˵����SD�����ҳ�ʼ�����
        // û�л�Ӧ��˵����MMC�������������Ӧ��ʼ��
        retry = 0;
        do
        {
            //�ȷ�CMD55��Ӧ����0x01���������
            r1 = SD_SendCommand(CMD55, 0, 0);
            if(r1 != 0x01)
            {
                return r1;  
            }
            //�õ���ȷ��Ӧ�󣬷�ACMD41��Ӧ�õ�����ֵ0x00����������200��
            r1 = SD_SendCommand(ACMD41, 0, 0);
            retry++;
        }while((r1!=0x00) && (retry<400));
        // �ж��ǳ�ʱ���ǵõ���ȷ��Ӧ
        // ���л�Ӧ����SD����û�л�Ӧ����MMC��
        
        //----------MMC�������ʼ��������ʼ------------
        if(retry==400)
        {
            retry = 0;
            //����MMC����ʼ�����û�в��ԣ�
            do
            {
                r1 = SD_SendCommand(1, 0, 0);
                retry++;
            }while((r1!=0x00)&& (retry<400));
            if(retry==400)
            {
                return 1;   //MMC����ʼ����ʱ
            }
            //д�뿨����
            SD_Type = SD_TYPE_MMC;
        }
        //----------MMC�������ʼ����������------------
        
        //����SPIΪ����ģʽ
        SD_SPI_SpeedHigh();

		SPI_ReadWriteByte(0xFF);
        
        //��ֹCRCУ��

//		r1 = SD_SendCommand(CMD59, 0, 0x01);
//        if(r1 != 0x00)
//        {
//            return r1;  //������󣬷���r1
//        }
  
        //����Sector Size
        r1 = SD_SendCommand(CMD16, 512, 0xff);
        if(r1 != 0x00)
        {
            return r1;  //������󣬷���r1
        }
        //-----------------SD����MMC����ʼ������-----------------

    }//SD��ΪV1.0�汾�ĳ�ʼ������
    

    //������V2.0���ĳ�ʼ��
    //������Ҫ��ȡOCR���ݣ��ж���SD2.0����SD2.0HC��
    else if(r1 == 0x01)
    {
        //V2.0�Ŀ���CMD8�����ᴫ��4�ֽڵ����ݣ�Ҫ�����ٽ���������
        buff[0] = SPI_ReadWriteByte(0xFF);  //should be 0x00
        buff[1] = SPI_ReadWriteByte(0xFF);  //should be 0x00
        buff[2] = SPI_ReadWriteByte(0xFF);  //should be 0x01
        buff[3] = SPI_ReadWriteByte(0xFF);  //should be 0xAA
     
        SD_CS_DISABLE();
        //the next 8 clocks
        SPI_ReadWriteByte(0xFF);
        
        //�жϸÿ��Ƿ�֧��2.7V-3.6V�ĵ�ѹ��Χ
        if(buff[2]==0x01 && buff[3]==0xAA)
        {
            //֧�ֵ�ѹ��Χ�����Բ���
            retry = 0;
            //������ʼ��ָ��CMD55+ACMD41
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
                    return r1;  //��ʱ�򷵻�r1״̬
                }
            }while(r1!=0);
          
            //��ʼ��ָ�����ɣ���������ȡOCR��Ϣ

            //-----------����SD2.0���汾��ʼ-----------
            r1 = SD_SendCommand(CMD58, 0, 0);
            if(r1!=0x00)
            {
                return r1;  //�������û�з�����ȷӦ��ֱ���˳�������Ӧ��
            }
            //��OCRָ����󣬽�������4�ֽڵ�OCR��Ϣ
            buff[0] = SPI_ReadWriteByte(0xFF);
            buff[1] = SPI_ReadWriteByte(0xFF); 
            buff[2] = SPI_ReadWriteByte(0xFF);
            buff[3] = SPI_ReadWriteByte(0xFF);

            //OCR������ɣ�Ƭѡ�ø�
            SD_CS_DISABLE();
            SPI_ReadWriteByte(0xFF);

            //�����յ���OCR�е�bit30λ��CCS����ȷ����ΪSD2.0����SDHC
            //���CCS=1��SDHC   CCS=0��SD2.0
            if(buff[0]&0x40)    //���CCS
            {
                SD_Type = SD_TYPE_V2HC;
            }
            else
            {
                SD_Type = SD_TYPE_V2;
            }
            //-----------����SD2.0���汾����-----------

            
            //����SPIΪ����ģʽ
            SD_SPI_SpeedHigh();  
        }

    }
    return r1;
}
//д��һ�����ݰ�������512�ֽ�
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd)
{	
	u16 t;
	u8 rxdata;
	uint8_t crcdata[2]={0xFF,0xFF};
	if(SD_WaitReady())return 1;//�ȴ�׼��ʧЧ
	SPI_ReadWriteByte(cmd);
	if(cmd!=0XFD)//���ǽ���ָ��
	{
		#ifndef SPI_DMA_WRITE_SECTOR
			for(t=0;t<512;t++)
//		SPI_ReadWriteByte(buf[t]);//����ٶ�,���ٺ�������ʱ��
		HAL_SPI_TransmitReceive(&hspi1,&buf[t],&rxdata,1,10);
	    SPI_ReadWriteByte(0xFF);//����crc
	    SPI_ReadWriteByte(0xFF);
		#else
			SD_WriteBuffer_DMA(buf,512);//
			SD_WriteBuffer_DMA(crcdata,2);//����crc
		#endif
		
		t=SPI_ReadWriteByte(0xFF);//������Ӧ
		if((t&0x1F)!=0x05)return 2;//��Ӧ����									  					    
	}						 									  					    
    return 0;//д��ɹ�
}

/*******************************************************************************
* Function Name  : SD_ReceiveData
* Description    : ��SD���ж���ָ�����ȵ����ݣ������ڸ���λ��
* Input          : uint8_t *data(��Ŷ������ݵ��ڴ�>len)
*                  uint16_t len(���ݳ��ȣ�
*                  uint8_t release(������ɺ��Ƿ��ͷ�����CS�ø� 0�����ͷ� 1���ͷţ�
* Output         : None
* Return         : uint8_t 
*                  0��NO_ERR
*                  other��������Ϣ
*******************************************************************************/
uint8_t SD_ReceiveData(uint8_t *data, uint16_t len)
{
    // ����һ�δ���
	uint8_t order=0xff;
    SD_CS_ENABLE();
	if(SD_GetResponse(0xFE))return 1;//�ȴ�SD������������ʼ����0xFE
    //��ʼ��������
    #ifndef  SPI_DMA_READ_SECTOR 
			while (len--)  // ��ʼ��������
			{
//				*data = SPI_ReadWriteByte(0xFF);   // ��������
				HAL_SPI_TransmitReceive(&hspi1,&order,data,1,10);
				data++;
			}
			SPI_ReadWriteByte(0xFF);   // ����CRC
			SPI_ReadWriteByte(0xFF);
		#else
			uint8_t crcdata[2]={0xFF,0xFF};
			SD_ReadBuffer_DMA(data, len);   // ��������
			SD_ReadBuffer_DMA(crcdata, 2);  // ����CRC
		#endif
    return 0;
}
/*******************************************************************************
* Function Name  : SD_GetCID
* Description    : ��ȡSD����CID��Ϣ��������������Ϣ
* Input          : uint8_t *cid_data(���CID���ڴ棬����16Byte��
* Output         : None
* Return         : uint8_t 
*                  0��NO_ERR
*                  1��TIME_OUT
*                  other��������Ϣ
*******************************************************************************/
uint8_t SD_GetCID(uint8_t *cid_data)
{
    uint8_t r1;

    //��CMD10�����CID
    r1 = SD_SendCommand(CMD10, 0, 0xFF);
    if(r1 != 0x00)
    {
        return r1;  //û������ȷӦ�����˳�������
    }
    //����16���ֽڵ�����
    SD_ReceiveData(cid_data, 16);

		//�������
		SD_CS_DISABLE();
		SPI_ReadWriteByte(0xFF);
		
    return 0;
}
/*******************************************************************************
* Function Name  : SD_GetCSD
* Description    : ��ȡSD����CSD��Ϣ�������������ٶ���Ϣ
* Input          : uint8_t *cid_data(���CID���ڴ棬����16Byte��
* Output         : None
* Return         : uint8_t 
*                  0��NO_ERR
*                  1��TIME_OUT
*                  other��������Ϣ
*******************************************************************************/
uint8_t SD_GetCSD(uint8_t *csd_data)
{
    uint8_t r1;

    //��CMD9�����CSD
    r1 = SD_SendCommand(CMD9, 0, 0x01);
    if(r1 != 0x00)
    {
        return r1;  //û������ȷӦ�����˳�������
    }
    //����16���ֽڵ�����
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
	//ȡCSD��Ϣ������ڼ��������0
	if(SD_GetCSD(csd)!=0) return 0;	    
	//���ΪSDHC�����������淽ʽ����
	if((csd[0]&0xC0)==0x40)	 //V2.00�Ŀ�
	{	
	csize = csd[9] + ((u16)csd[8] << 8) + 1;
	Capacity = (u32)csize << 10;//�õ�������	 		   
	}else//V1.XX�Ŀ�
	{	
	n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
	csize = (csd[8] >> 6) + ((u16)csd[7] << 2) + ((u16)(csd[6] & 3) << 10) + 1;
	Capacity= (u32)csize << (n - 9);//�õ�������   
	}
	return Capacity;
}
/*******************************************************************************
* Function Name  : SD_ReadMultiBlock
* Description    : ��SD���Ķ��block
* Input          : uint32_t sector ȡ��ַ��sectorֵ���������ַ�� 
*                  uint8_t *buffer ���ݴ洢��ַ����С����512byte��
*                  uint8_t count ������count��block
* Output         : None
* Return         : uint8_t r1 
*                   0�� �ɹ�
*                   other��ʧ��
*******************************************************************************/
uint8_t SD_ReadMultiBlock(uint32_t sector, uint8_t *buffer, uint8_t count)
{
	uint8_t r1;
	//����Ϊ����ģʽ
	SD_SPI_SpeedHigh();
	//�������SDHC����sector��ַת��byte��ַ
	if(SD_Type!=SD_TYPE_V2HC)
	{
		sector = sector<<9;
	}
	if(count==1)
	{
		r1=SD_SendCommand(CMD17,sector,0X01);//������
		if(r1==0)//ָ��ͳɹ�
		{
			r1=SD_ReceiveData(buffer,512);//����512���ֽ�	   
		}
	}
	//�����������
	else
	{
		r1=SD_SendCommand(CMD18,sector,0X01);//����������
		do
		{
			r1=SD_ReceiveData(buffer,512);//����512���ֽ�	 
			buffer+=512;  
		}while(--count && r1==0); 	
		SD_SendCommand(CMD12,0,0X01);	//����ֹͣ����
	}   
	//�ͷ�����
	SD_CS_DISABLE();
	return r1;
}


/*******************************************************************************
* Function Name  : SD_WriteMultiBlock
* Description    : д��SD����N��block
* Input          : uint32_t sector ������ַ��sectorֵ���������ַ�� 
*                  uint8_t *buffer ���ݴ洢��ַ����С����512byte��
*                  uint8_t count д���block��Ŀ
* Output         : None
* Return         : uint8_t r1 
*                   0�� �ɹ�
*                   other��ʧ��
*******************************************************************************/
uint8_t SD_WriteMultiBlock(uint32_t sector,uint8_t *data, uint8_t count)
{
	u8 r1;
	if(SD_Type!=SD_TYPE_V2HC)sector *= 512;//ת��Ϊ�ֽڵ�ַ
	if(count==1)
	{
		r1=SD_SendCommand(CMD24,sector,0X01);//������
		if(r1==0)//ָ��ͳɹ�
		{
			r1=SD_SendBlock(data,0xFE);//д512���ֽ�	   
		}
	}else
	{
		if(SD_Type!=SD_TYPE_MMC)
		{
			SD_SendCommand(CMD55,0,0X01);	
			SD_SendCommand(ACMD23,count,0X01);//����ָ��	
		}
 		r1=SD_SendCommand(CMD25,sector,0X01);//����������
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(data,0xFC);//����512���ֽ�	 
				data+=512;  
			}while(--count && r1==0);
			r1=SD_SendBlock(0,0xFD);//����512���ֽ� 
		}
	}   
	SD_CS_DISABLE();//ȡ��Ƭѡ
	return r1;//
}
