/************************ (C) COPYRIGHT Megahuntmicro *************************
 * @file                : mhscpu_rtc.h
 * @author              : Megahuntmicro
 * @version             : V1.0.0
 * @date                : 21-October-2014
 * @brief               : This file contains all the functions prototypes for the RTC firmware library
 *****************************************************************************/

#ifndef __MHSCPU_RTC_H
#define __MHSCPU_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "mhscpu.h"

FlagStatus RTC_IsReady(void);

void RTC_ResetCounter(void);
uint32_t RTC_GetCounter(void);

void RTC_SetRefRegister(uint32_t RefValue);
uint32_t RTC_GetRefRegister(void);

void RTC_SetAlarm(uint32_t AlarmValue);

uint32_t RTC_GetAttrackTime(void);

void RTC_ITConfig(FunctionalState NewState);
void RTC_ClearITPendingBit(void);
ITStatus RTC_GetITStatus(void);

	
#ifdef __cplusplus
}
#endif

#endif

/**************************      (C) COPYRIGHT Megahunt    *****END OF FILE****/
