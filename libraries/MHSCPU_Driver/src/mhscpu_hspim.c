/************************ (C) COPYRIGHT Megahuntmicro ******************************************************************
 * @file                : mhscpu_hspim.c
 * @author              : Megahuntmicro
 * @version             : V1.0.0
 * @date                : 5-November-2019
 * @brief               : This file provides all the HSPIM firmware functions
 *********************************************************************************************************************/

 /* Includes --------------------------------------------------------------------------------------------------------*/
#include "mhscpu_hspim.h"

/** @defgroup HSPIM_Private_Defines
  * @{
  */

#define HSPIM_CR0_CLEAR_MASK										((uint32_t)~0xFFEEFFFF)
#define HSPIM_CR0_MODE_SELECT_CLEAR_MASK							((uint32_t)~0x1C00)
#define HSPIM_CR1_CLEAR_MASK										((uint32_t)~0xFFFFF)
#define HSPIM_FCR_CLEAR_MASK										((uint32_t)~0x3F3F3F00)
#define HSPIM_DCR_RECEIVE_LEVEL_CLEAR_MASK							((uint32_t)~0x3F80)
#define HSPIM_DCR_TRANSMIT_LEVEL_CLEAR_MASK							((uint32_t)~0x7F)


#define HSPIM_CR0_PARAM_ENABLE_POS									(0x18)
#define HSPIM_CR0_PARAM_DMA_RECEIVE_ENABLE_POS						(0x14)
#define HSPIM_CR0_PARAM_DMA_TRANSMIT_ENABLE_POS						(0x10)
#define HSPIM_CR0_PARAM_INTERRPUT_ENABLE_POS						(0x0D) 
#define HSPIM_CR0_PARAM_MODEL_SELECT_POS							(0x0A)
#define HSPIM_CR0_PARAM_FIRST_BIT_POS								(0x09)
#define HSPIM_CR0_PARAM_CPOL_POS									(0x08)
#define HSPIM_CR0_PARAM_CPHA_POS									(0x07)
#define HSPIM_CR0_PARAM_DIVIDE_ENABLE_POS							(0x02)
#define HSPIM_CR0_PARAM_TRANSMIT_ENABLE_POS							(0x01)
#define HSPIM_CR0_PARAM_BUSY_POS									(0x00)

#define HSPIM_CR1_PARAM_BAUDRATE_POS								(0x0A)
#define HSPIM_CR1_PARAM_RECEIVE_DATA_LENGTH_POS						(0x00)

#define HSPIM_DCR_PARAM_DMA_RECEIVE_LEVEL_POS						(0x07)
#define HSPIM_DCR_PARAM_DMA_TRANSMIT_LEVEL_POS						(0x00)		

#define HSPIM_FCR_PARAM_TRANSIMIT_FIFO_EMPTY_THRESHOULD_POS			(0x08)
#define HSPIM_FCR_PARAM_RECEIVE_FIFO_FULL_THRESHOULD_POS			(0x10)

/**
  * @}
  */

/**
  * @brief  Initializes the HSPIM peripheral according to the specified parameters.
  * @param  HSPIM_InitStruct: pointer to a HSPIM_InitTypeDef structure that
  *         contains the configuration information for the specified HSPIM peripheral.
  * @retval None
  */
void HSPIM_Init(HSPIM_InitTypeDef* HSPIM_InitStruct)
{
	assert_param(IS_HSPIM_MODE(HSPIM_InitStruct->HSPIM_Mode));
	assert_param(IS_HSPIM_CPOL(HSPIM_InitStruct->HSPIM_CPOL));
	assert_param(IS_HSPIM_CPHA(HSPIM_InitStruct->HSPIM_CPHA));
	assert_param(IS_HSPIM_FIRST_BIT(HSPIM_InitStruct->HSPIM_FirstBit));
	assert_param(IS_HSPIM_BAUDRATE_PRESCALER(HSPIM_InitStruct->HSPIM_BaudRatePrescaler));
	assert_param(IS_HSPIM_TX_FIFO_EMPTY_THRESHOLD(HSPIM_InitStruct->HSPIM_TXFIFOEmptyThreshold));
	assert_param(IS_HSPIM_RX_FIFO_FULL_THRESHOLD(HSPIM_InitStruct->HSPIM_RXFIFOFullThreshold));
	
			
	HSPIM->CR1 = (uint32_t)((HSPIM->CR1 & HSPIM_CR1_CLEAR_MASK) 
							| ((HSPIM_InitStruct->HSPIM_BaudRatePrescaler & 0x1FF) << HSPIM_CR1_PARAM_BAUDRATE_POS)
							| ((HSPIM_InitStruct->HSPIM_ReceiveDataLength & 0x3FF) << HSPIM_CR1_PARAM_RECEIVE_DATA_LENGTH_POS));
	
	HSPIM->FCR = (uint32_t)((HSPIM->FCR & HSPIM_FCR_CLEAR_MASK)
							| ((HSPIM_InitStruct->HSPIM_TXFIFOEmptyThreshold & 0x3F) << HSPIM_FCR_PARAM_TRANSIMIT_FIFO_EMPTY_THRESHOULD_POS)
							| ((HSPIM_InitStruct->HSPIM_RXFIFOFullThreshold & 0x3F) << HSPIM_FCR_PARAM_RECEIVE_FIFO_FULL_THRESHOULD_POS));	

	HSPIM->CR0 = (uint32_t)((HSPIM->CR0 & HSPIM_CR0_CLEAR_MASK) 
							| ((HSPIM_InitStruct->HSPIM_CPOL & 0x01) << HSPIM_CR0_PARAM_CPOL_POS)
							| ((HSPIM_InitStruct->HSPIM_CPHA & 0x01) << HSPIM_CR0_PARAM_CPHA_POS)
							| ((HSPIM_InitStruct->HSPIM_FirstBit & 0x01) << HSPIM_CR0_PARAM_FIRST_BIT_POS)
							| ((HSPIM_InitStruct->HSPIM_Mode & 0x07) << HSPIM_CR0_PARAM_MODEL_SELECT_POS)
							| ((HSPIM_InitStruct->HSPIM_DivideEnable & 0x01) << HSPIM_CR0_PARAM_DIVIDE_ENABLE_POS)
							| ((HSPIM_InitStruct->HSPIM_Enable & 0x01) << HSPIM_CR0_PARAM_ENABLE_POS));			
	
}

/**
  * @brief  Initializes the HSPIM DMA interface according to the specified parameters.
  * @param  HSPIM_DMAInitStruct: pointer to a HSPIM_DMAInitStruct structure that contains the configuration information.
  * @retval None
  */
void HSPIM_DMAInit(HSPIM_DMAInitTypeDef* HSPIM_DMAInitStruct)
{
	if (ENABLE == HSPIM_DMAInitStruct->HSPIM_DMAReceiveEnable)
	{	
		assert_param(IS_HSPIM_DMA_RECEIVE_LEVEL(HSPIM_DMAInitStruct->HSPIM_DMAReceiveLevel));
			
		HSPIM->DCR = (uint32_t)((HSPIM->DCR & HSPIM_DCR_RECEIVE_LEVEL_CLEAR_MASK)
								|((HSPIM_DMAInitStruct->HSPIM_DMAReceiveLevel) << HSPIM_DCR_PARAM_DMA_RECEIVE_LEVEL_POS));
		
		HSPIM->CR0 |= (uint32_t)((HSPIM_DMAInitStruct->HSPIM_DMAReceiveEnable & 0x01) << HSPIM_CR0_PARAM_DMA_RECEIVE_ENABLE_POS);
	}
	else
	{
		HSPIM->CR0 &= ~(uint32_t)(0x01 << HSPIM_CR0_PARAM_DMA_RECEIVE_ENABLE_POS);
	}
	
	if (ENABLE == HSPIM_DMAInitStruct->HSPIM_DMATransmitEnable)
	{
		assert_param(IS_HSPIM_DMA_TRANSMIT_LEVEL(HSPIM_DMAInitStruct->HSPIM_DMATransmitLevel));	
		
		HSPIM->DCR = (uint32_t)((HSPIM->DCR & HSPIM_DCR_TRANSMIT_LEVEL_CLEAR_MASK)
								|((HSPIM_DMAInitStruct->HSPIM_DMATransmitLevel) << HSPIM_DCR_PARAM_DMA_TRANSMIT_LEVEL_POS));
		
		HSPIM->CR0 |= (uint32_t)((HSPIM_DMAInitStruct->HSPIM_DMATransmitEnable & 0x01) << HSPIM_CR0_PARAM_DMA_TRANSMIT_ENABLE_POS);
	}
	else 
	{
		HSPIM->CR0 &= ~(uint32_t)(0x01 << HSPIM_CR0_PARAM_DMA_TRANSMIT_ENABLE_POS);
	}
}

/**
  * @brief  Enable or disable the HSPIM peripheral
  * @param  NewState: new state of the HSPIM peripheral. 
  *   		This parameter can be: ENABLE or DISABLE
  * @retval None
  */
void HSPIM_Cmd(FunctionalState NewState)
{		
	if (NewState != DISABLE)
	{
		HSPIM->CR0 |= 1 << HSPIM_CR0_PARAM_ENABLE_POS;
	}
	else
	{
		HSPIM->CR0 &= ~(1 << HSPIM_CR0_PARAM_ENABLE_POS);
	}
}

/**
  * @brief  Transmits single data through the HSPIM peripheral.
  * @param  Mode: Select the HSPIM bus mode. 
  * @param  Data: the data to be transmitted
  * @retval None
  */
void HSPIM_SendData(uint8_t Data)
{
	HSPIM->WDR = Data;
}

/**
  * @brief  Receive single data through the HSPIM peripheral.
  * @param  Mode: Select the HSPIM bus mode. 
  * @retval The received data
  */
uint8_t HSPIM_ReceiveData(void)
{
	return (uint8_t)HSPIM->RDR;
}

/**
  * @brief  Checks whether the specified HSPIM flag is set or not.
  * @retval The new state of HSPIM_FLAG (SET or RESET).
  */

FlagStatus HSPIM_GetFlagStatus(uint32_t HSPIM_Flag)
{
	if ((HSPIM->SR & HSPIM_Flag) != RESET)
	{
		return SET;  
	} 
	return RESET; 
}

/**
  * @brief  Checks whether the specified HSPIM interrupt has occurred or not.
  * @retval The new state of HSPIM_IT (SET or RESET).
  */
ITStatus HSPIM_GetITStatus(uint32_t HSPIM_IT)
{
	uint8_t itmask = 0;
	
	itmask = (uint8_t)HSPIM->RISR;
	itmask <<= 4;
	itmask = (uint8_t)(itmask | HSPIM->TISR);
	
	if ((itmask & HSPIM_IT) != RESET)
	{
		return SET;
	}	
	return RESET;
}

/**
  * @brief  Checks whether the specified HSPIM_TramsmitDone is set or not.
  * @retval The new state of HSPIM_FLAG (SET or RESET).
  */
FlagStatus HSPIM_IsTransmitDone(void)
{
	if ((HSPIM->FLSR & 0x01) != RESET)
	{
		return SET;
	}
	return RESET;
}


FlagStatus HSPIM_IsReady(void)
{
	if ((HSPIM->CR1 & 0x1) == SET)
	{
		return RESET;
	}
	return SET;
}
/**
  * @brief Reset HSPIM FIFO(TX or RX).
  */
void HSPIM_FIFOReset(uint32_t HSPIM_FIFO)
{
	HSPIM->FCR |= HSPIM_FIFO;
	
	HSPIM->FCR &= ~HSPIM_FIFO;
}

/**
  *@brief  Enable or disable the specified HSPIM interrupt.
  */

void HSPIM_ITConfig(uint32_t HSPIM_IT, FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		HSPIM->CR0 |= HSPIM_IT;
	}
	else
	{
		HSPIM->CR0 &= ~HSPIM_IT;
	}
}

/**
  * @brief  Enables or disables the HSPIM transfer.
  * @param  NewState: new state of the HSPIM peripheral. 
  *   		This parameter can be: ENABLE or DISABLEE.
  * @retval None
  */

void HSPIM_TransmitCmd(FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		HSPIM->CR0 |= 1 << HSPIM_CR0_PARAM_TRANSMIT_ENABLE_POS;
	}
	else
	{
		HSPIM->CR0 &= ~(1 << HSPIM_CR0_PARAM_TRANSMIT_ENABLE_POS);
	}
}

void HSPIM_DualQuadReceiveCmd(FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		HSPIM->FLCR |= 0x01;
	}
	else
	{
		HSPIM->FLCR &= 0x00;
	}
}


void HSPIM_ModeSwitch(uint8_t Mode)
{
	assert_param(IS_HSPIM_MODE(Mode));
	
	HSPIM_TransmitCmd(DISABLE);
	HSPIM_DualQuadReceiveCmd(DISABLE);
	HSPIM_Cmd(DISABLE);
	
	HSPIM->CR0 = (uint32_t)((HSPIM->CR0 & HSPIM_CR0_MODE_SELECT_CLEAR_MASK) 
							| ((Mode & 0x07) << HSPIM_CR0_PARAM_MODEL_SELECT_POS));
	
	HSPIM_Cmd(ENABLE);
}

#define MAX_READ_DATA_LENGTH								(64)		
#define MAX_WRITE_DATA_LENGTH								(64)

/**
  * @brief  Transmits single data through the QSPI peripheral.
  * @param  Mode: Select the QSPI bus mode. 
  * @param  size: 0~63.
  * @param  p_data: pointer to data buffer.
  * @retval None
  */

void HSPIM_WriteBuff(uint8_t Mode, uint16_t Size, uint8_t* Buff)
{
	uint16_t i;
	
	HSPIM_ModeSwitch(Mode);
	HSPIM_TransmitCmd(ENABLE);
		
	for(i = 0; i < Size; i++)
	{
		HSPIM->WDR = Buff[i];
		while(RESET == HSPIM_IsTransmitDone());		
	}		
}


/**
  * @brief  Receive single data through the QSPI peripheral.   
  * @param  Mode: Select the QSPI bus mode. 				  
  * @param  Size: 0~63.									  
  * @param  Buff: pointer to data buffer.					  
  */


void HSPIM_ReadBuff(uint8_t Mode, uint16_t Size, uint8_t* Buff)
{
	uint16_t i;
	
	HSPIM_ModeSwitch(Mode);
	
	if (Mode == HSPIM_MODE_DUAL_RECEIVE || Mode == HSPIM_MODE_QUAD_RECEIVE)
	{
		HSPIM_DualQuadReceiveCmd(ENABLE);
	}
	
	for(i = 0; i < Size; i++)
	{
		while(HSPIM_GetFlagStatus(HSPIM_FLAG_RXE) == SET);
		Buff[i] = (uint8_t)HSPIM->RDR;
	}
}



/**************************      (C) COPYRIGHT Megahunt    *****END OF FILE***********************************************************************/
