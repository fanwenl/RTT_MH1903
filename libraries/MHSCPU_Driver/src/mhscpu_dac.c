/************************ (C) COPYRIGHT Megahuntmicro *************************
 * File Name            : mhscpu_dac.c
 * Author               : Megahuntmicro
 * Version              : V1.0.0
 * Date                 : 05/28/2017
 * Description          : DAC module functions set.
 *****************************************************************************/
 
/* Include ------------------------------------------------------------------*/
#include "mhscpu_dac.h"

/**
  * @brief  Initializes the DAC peripheral according to the specified parameters.
  * @param  DAC_InitStruct: pointer to a DAC_InitTypeDef structure that contains the configuration information.
  * @retval None
  */
void DAC_Init(DAC_InitTypeDef *DAC_InitStruct)
{
	assert_param(IS_DAC_CURR_SEL(DAC_InitStruct->DAC_CurrSel));
	assert_param(IS_DAC_FIFO_THR(DAC_InitStruct->DAC_FIFOThr));
	
	DAC_Cmd(DISABLE);
	
	while (DAC->DAC_CR1 & DAC_CR1_IS_RUNNING);
	
	DAC->DAC_CR1 = (DAC->DAC_CR1 & ~DAC_CURR_SEL_MASK) | DAC_InitStruct->DAC_CurrSel;
	DAC->DAC_TIMER = DAC_InitStruct->DAC_TimerExp;
	DAC->DAC_FIFO_THR = DAC_InitStruct->DAC_FIFOThr;
}

/**
  * @brief  Initializes each DAC_InitStruct member with its default value.
  * @param  DAC_InitStruct: pointer to a DAC_InitTypeDef structure which will be initialized.
  * @retval None
  */
void DAC_StructInit(DAC_InitTypeDef *DAC_InitStruct)
{
	DAC_InitStruct->DAC_CurrSel = DAC_CURR_SEL_2K;
	DAC_InitStruct->DAC_TimerExp = 1;
	DAC_InitStruct->DAC_FIFOThr = 1;
}

/**
  * @brief  Reset the DAC FIFO
  * @param  None
  * @retval None
  */
void DAC_FIFOReset(void)
{
	DAC->DAC_CR1 |= DAC_CR1_FIFO_RESET;
}

/**
  * @brief  Enable or disable the DAC peripheral.
  * @param  NewState: new state of the DAC peripheral.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void DAC_Cmd(FunctionalState NewState)
{
	assert_param(IS_FUNCTIONAL_STATE(NewState));
	
	if (DISABLE != NewState)
	{
		DAC->DAC_CR1 &= ~DAC_CR1_POWER_DOWN;
		ADC0->ADC_CR1 &= ~ADC_CR1_POWER_DOWN;
	}
	else
	{
		DAC->DAC_CR1 |= DAC_CR1_POWER_DOWN;
	}
}

/**
  * @brief  Enable or disable the DAC DMA request.
  * @param  NewState: new state of the DAC DMA request.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void DAC_DMACmd(FunctionalState NewState)
{
	assert_param(IS_FUNCTIONAL_STATE(NewState));
	
	if (DISABLE != NewState)
	{
		DAC->DAC_CR1 |= DAC_CR1_DMA_ENABLE;
	}
	else
	{
		DAC->DAC_CR1 &= ~DAC_CR1_DMA_ENABLE;	
	}
}

/**
  * @brief  Set the specified data for DAC.
  * @param  Data: Data to be loaded in the DAC_DATA register.
  * @retval None
  */
void DAC_SetData(uint16_t Data)
{
	DAC->DAC_DATA = Data & 0x3FF;
}

/**
  * @brief  Checks whether the specified DAC flag is set or not. 
  * @param  DAC_Flag: specifies the flag to check.
  *         This parameter can be the following values:
  *    @arg DAC_FLAG_RUNNING: DAC is running
  *    @arg DAC_FLAG_OVERFLOW: DAC FIFO overflow
  * @retval the new state of DAC_Flag(SET or RESET).
  */
FlagStatus DAC_GetFlagStatus(uint32_t DAC_Flag)
{
	assert_param(IS_DAC_FLAG(DAC_Flag));
	
	if (RESET != (DAC->DAC_CR1 & DAC_Flag))
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/**
  * @brief  Enable or disable the specified DAC interrupt.
  * @param  DAC_IT: specifies the DAC interrupt sources to be enabled or disabled.
  *         This parameter can be the following values:
  *    @arg DAC_IT_FIFO_THR: DAC FIFO Threshold interrupt mask
  *    @arg DAC_IT_FIFO_OVERFLOW: DAC FIFO Overflow interrupt mask
  * @param  NewState: new state of the specified DAC interrupts.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void DAC_ITConfig(uint32_t DAC_IT, FunctionalState NewState)
{
	assert_param(IS_DAC_IT(DAC_IT));
	assert_param(IS_FUNCTIONAL_STATE(NewState));

	if (DISABLE != NewState)
	{
		DAC->DAC_CR1 |= DAC_IT;
	}
	else
	{
		DAC->DAC_CR1 &= ~DAC_IT;
	}
}

/**
  * @brief  Checks whether the specified DAC interrupt has occurred or not. 
  * @param  DAC_IT: specifies the DAC interrupt sources to check.
  *         This parameter can be the following values:
  *    @arg DAC_IT_FIFO_THR: DAC FIFO Threshold interrupt mask
  *    @arg DAC_IT_FIFO_OVERFLOW: DAC FIFO Overflow interrupt mask
  * @retval the new state of DAC_IT(SET or RESET).
  */
ITStatus DAC_GetITStatus(uint32_t DAC_IT)
{
	assert_param(IS_DAC_IT(DAC_IT));
	
	if ((RESET != (DAC->DAC_CR1 & (DAC_IT << DAC_IT_STATUS_SHIFT))) && (DAC->DAC_CR1 & DAC_IT))
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void DAC_ClearITPendingBit(uint32_t DAC_IT)
{
	uint32_t cr1tmp;
	
	assert_param(DAC_IT_FIFO_OVERFLOW == DAC_IT);
	
	cr1tmp = DAC->DAC_CR1;
}


/************************ (C) COPYRIGHT 2017 Megahuntmicro ****END OF FILE****/
