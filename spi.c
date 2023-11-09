/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
	#include "usart.h"
//	#include "malloc.h"
	#include <stdbool.h>	/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */
uint8_t SPI1_DMA_Flag;
uint8_t txrxdata[5120];		// 接收缓存512字节
/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

/* SPI1 init function */
void MX_SPI1_Init(void)
{
  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */
  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */
  /* USER CODE END SPI1_MspInit 0 */
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PB3 (JTDO-TRACESWO)     ------> SPI1_SCK
    PB4 (NJTRST)     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_RX Init */
    hdma_spi1_rx.Instance = DMA1_Channel2;
    hdma_spi1_rx.Init.Request = DMA_REQUEST_1;
    hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode = DMA_NORMAL;
    hdma_spi1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmarx,hdma_spi1_rx);

    /* SPI1_TX Init */
    hdma_spi1_tx.Instance = DMA1_Channel3;
    hdma_spi1_tx.Init.Request = DMA_REQUEST_1;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;//DMA_PDATAALIGN_HALFWORD  DMA_PDATAALIGN_BYTE
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;//DMA_MDATAALIGN_HALFWORD  DMA_MDATAALIGN_BYTE
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi1_tx);

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn,1, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PB3 (JTDO-TRACESWO)     ------> SPI1_SCK
    PB4 (NJTRST)     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmarx);
    HAL_DMA_DeInit(spiHandle->hdmatx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint8_t SPI_ReadWriteByte(uint8_t Txdata)
{
	uint8_t Rxdata;	
	HAL_SPI_TransmitReceive(&hspi1,&Txdata,&Rxdata,1,100);
	return Rxdata;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)//HAL_SPI_TxRxCpltCallback;SPI_DMATransmitReceiveCplt
{
	printf("[%s] %d:\r\n",__func__,__LINE__);
//  if (hspi->Instance == SPI1)
//	{
		/* SPI3的DMA传输完成标志置位 */		
		SPI1_DMA_Flag = 1;
//		printf("[%s] %d:\r\n",__func__,__LINE__);
//	}

}


/**
  * @brief  通过SPI总线读数据
  * @note   无
  * @param  RxData: 读取的数据
  * @param  Size: 数据长度，不能超过512字节
  * @retval 结果 0-成功，其他-失败
  */
int8_t SD_ReadBuffer_DMA(uint8_t *RxData, uint16_t Size)
{printf("[%s] %d:\r\n",__func__,__LINE__);
	uint32_t i = 0;					// 循环变量

	HAL_SPI_TransmitReceive_DMA(&hspi1, txrxdata, RxData, Size);
//	HAL_SPI_Receive_DMA(&hspi1,(uint8_t*) RxData, Size); 	
//HAL_DMA_PollForTransfer(&hdma_spi1_rx,HAL_DMA_FULL_TRANSFER,100);
	while (1)
  {
		if(SPI1_DMA_Flag == 1)
		{
			printf ("i=%d\r\n",i);
			printf ("SPI1_DMA_Flag=%x\r\n",SPI1_DMA_Flag);
				SPI1_DMA_Flag=0;
			return 0;
			
		}
		i++;
		if (i > 0xFFFFFF)
		{
			printf ("i=%d\r\n",i);
			printf ("SPI1_DMA_Flag=%x\r\n",SPI1_DMA_Flag);
			return 1;	/* 超时退出 */
		}
	}
//	else 
//		return 1;
//				return 0;
}


/**
  * @brief  通过SPI总线写数据
  * @note   无
  * @param  TxData: 写入的数据
  * @param  Size: 数据长度，不能超过512字节
  * @retval 结果 0-成功，其他-失败
  */
int8_t SD_WriteBuffer_DMA(const uint8_t *TxData, uint16_t Size)
{printf("[%s] %d:\r\n",__func__,__LINE__);

	uint32_t i = 0;					// 循环变量
	HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t*)TxData, txrxdata, Size);
//	HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)TxData, Size); 	
HAL_DMA_PollForTransfer(&hdma_spi1_tx,HAL_DMA_FULL_TRANSFER,100);
while (1)
  {
		if(SPI1_DMA_Flag == 1)
		{
				SPI1_DMA_Flag=0;
			printf ("i=%x\r\n",i);
			printf ("SPI1_DMA_Flag=%x\r\n",SPI1_DMA_Flag);
			return 0;
		}
		i++;
		if (i > 0xFFFFFF)
		{
			printf ("i=%x\r\n",i);
			printf ("SPI1_DMA_Flag=%x\r\n",SPI1_DMA_Flag);
			return 1;	/* 超时退出 */
		}
	}
	
//	else
//		return 1;
//			return 0;
}



//SPI1波特率设置
void SPI_setspeed(uint8_t speed)
{
	hspi1.Init.BaudRatePrescaler = speed;
}
/* USER CODE END 1 */

