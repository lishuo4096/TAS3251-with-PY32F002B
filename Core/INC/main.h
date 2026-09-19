/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.h
  * @brief   This file provides code for the configuration
  *          of all used MAIN.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by Mcu Studio under BSD 3-Clause license,
  * the License ; You may not use this file except in compliance with the
  * License.You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rcc.h"
#include "gpio.h"
#include "i2c.h"
#include "tim.h"
#include "adc.h"
#include "stdbool.h"
#include "stdint.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern volatile uint32_t pvdd_mv;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
/* USER CODE BEGIN EFP */
void WS2812_Send_Data(uint8_t WS2812_Num,uint8_t Red,uint8_t Green,uint8_t Blue);
void HSV2RGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
extern uint16_t Get_ADC_Code(uint16_t channel);
extern uint32_t Get_VCCA_mV(void);
extern uint32_t Get_PVDD_mV(void);
extern bool PVDD_Is_PowerDrop(void);
void TAS3251_Set_Vol_dB(int8_t Volume);
void TAS3251_Init(void);
void TAS3251_Set_Input_Format(uint8_t format, uint8_t word_len);
void TAS3251_Set_Vol_dB(int8_t Volume);
void AMP_Fault_Handler(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
//GPIOA
#define AMP_FAULT_PIN GPIO_PIN_6
#define AMP_RESET_PIN GPIO_PIN_5
#define AMP_CLIP_PIN GPIO_PIN_4
#define PVDD_ADC_PIN GPIO_PIN3
//GPIOB
#define DAC_MUTE_PIN GPIO_PIN_5
#define WS2812_DATA_PIN GPIO_PIN_2

/* USER CODE END Private defines */


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */
