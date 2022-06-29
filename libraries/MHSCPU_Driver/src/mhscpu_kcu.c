/************************ (C) COPYRIGHT Megahuntmicro *************************
 * @file                : mhscpu_kcu.c
 * @author              : Megahuntmicro
 * @version             : V1.0.0
 * @date                : 21-October-2014
 * @brief               : This file provides all the KCU firmware functions
 *****************************************************************************/  

/* Include ------------------------------------------------------------------*/
#include "mhscpu_kcu.h"

/* Exported functions -------------------------------------------------------*/	
/**
  * @brief  Deinitializes the KCU peripheral registers to their default reset values.
  * @param  None
  * @retval None
  */
void KCU_DeInit(void)
{
	SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_KBD, ENABLE);
}

/**
  * @brief  Initializes the KCU peripheral according to the specified
  *         parameters in the KCU_InitStruct.
  * @param  KCU_InitStruct: pointer to a KCU_InitTypeDef structure that
  *         contains the configuration information for the specified KCU peripheral.
  * @retval None
  */
void KCU_Init(KCU_InitTypeDef *KCU_InitStruct)
{
	uint32_t tmpPortSet = 0;
	uint32_t tmpDebounceTimeSet = 0;
	
	KCU_Cmd(DISABLE);
	while ((KCU->KCU_CTRL1 & KCU_CTRL1_KCU_RUNING) != 0);
	
	tmpDebounceTimeSet = KCU_InitStruct->KCU_DebounceTimeLevel << KCU_DEBOUNCETIMELEVEL_POS;
	tmpPortSet = KCU_InitStruct->KCU_PortOutput & ~(KCU_InitStruct->KCU_PortInput);
	
	KCU->KCU_CTRL0 = tmpDebounceTimeSet | tmpPortSet;
	KCU->KCU_RNG = KCU_InitStruct->KCU_Rand;
}

/**
  * @brief  Fills each KCU_InitStruct member with its default value.
  * @param  KCU_InitStruct : pointer to a KCU_InitTypeDef structure which will
  *         be initialized.
  * @retval None
  */
void KCU_StructInit(KCU_InitTypeDef *KCU_InitStruct)
{
	KCU_InitStruct->KCU_DebounceTimeLevel = KCU_DEBOUNCETIMELEVEL_1;
	KCU_InitStruct->KCU_PortInput = KCU_PORT_0 | KCU_PORT_1 | KCU_PORT_2 | KCU_PORT_3; 
	KCU_InitStruct->KCU_PortOutput = KCU_PORT_4 | KCU_PORT_5 | KCU_PORT_6 | KCU_PORT_7 | KCU_PORT_8; 
	KCU_InitStruct->KCU_Rand = 0x5A5A5A5A;
}

/**
  * @brief  Enable or disable the KCU peripheral
  * @param  NewState: new state of the KCU peripheral
  *			This parameter can be ENABLE or DISABLE
  * @retval None
  */
void KCU_Cmd(FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		KCU->KCU_CTRL1 |= KCU_CTRL1_KBD_EN;	
	}
	else
	{
		KCU->KCU_CTRL1 &= ~KCU_CTRL1_KBD_EN;	
	}
}

/**
  * @brief  Initializes the KCU peripheral rand value
  * @param  Rand: rand value of KCU peripheral
  * @retval None
  */
uint32_t KCU_SetRand(uint32_t Rand)
{
	if (0 == Rand || ~0 == Rand)
	{
		return 1;
	}

	KCU->KCU_RNG = Rand;
	return 0;
}

/**
  * @brief  Check whether the KCU interrupt has occurred or not
  * @param  KCU_IT: specifies the KCU interrupt sources
  *         This parameter can be one of the following values:
  *     @arg KCU_IT_PUSH 
  *     @arg KCU_IT_RELEASE
  *     @arg KCU_IT_OVERRUN
  * @retval The new state of KCU_IT (SET or RESET) 
  */
FlagStatus KCU_GetITStatus(uint32_t KCU_IT)
{
	uint32_t tmpStatus = 0;
	
	if (KCU_IT & KCU_IT_PUSH)
	{
		tmpStatus |= KCU_STATUS_PUSH_IT;
	}
	if (KCU_IT & KCU_IT_RELEASE)
	{
		tmpStatus |= KCU_STATUS_RELEASE_IT;
	}
	if (KCU_IT & KCU_IT_OVERRUN)
	{
		tmpStatus |= KCU_STATUS_OVERRUN_IT;
	}

	if ((KCU->KCU_STATUS & tmpStatus) != (uint16_t)RESET)
	{
		return SET;
	}

	return RESET;
}

/**
  * @brief  Enable or disable the specified KCU interrupt.
  * @param  KCU_IT: specifies the KCU interrupt sources
  *         This parameter can be one of the following values:
  *     @arg KCU_IT_PUSH 
  *     @arg KCU_IT_RELEASE
  *     @arg KCU_IT_OVERRUN
  * @param  NewState: new state of the KCU interrupt
  *			This parameter can be ENABLE or DISABLE
  * @retval None
  */
void KCU_ITConfig(uint32_t KCU_IT, FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		KCU->KCU_CTRL1 |= KCU_IT;
	}
	else
	{
		KCU->KCU_CTRL1 &= ~KCU_IT;
	}
}

/**
  * @brief  Clear all the KCU interrupt pending bit
  * @param  None
  * @retval None
  */
void KCU_ClearITPending(void)
{
	volatile uint32_t clr;
	clr = KCU->KCU_EVENT;
}

/**
  * @brief  Get KCU event
  * @param  None
  * @retval None
  */
uint32_t KCU_GetEvenReg(void)
{
	return KCU->KCU_EVENT;
}

/**************************      (C) COPYRIGHT Megahunt    *****END OF FILE****/
