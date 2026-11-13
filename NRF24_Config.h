/*
 * NRF24_Config.h
 *
 *  Created on: Nov 11, 2026
 *      Author: bjsek
 */

#ifndef INC_NRF24_CONFIG_H_
#define INC_NRF24_CONFIG_H_

#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define NRF24_CE_Pin GPIO_PIN_3
#define NRF24_CE_GPIO_Port GPIOA
#define NRF24_CSN_Pin GPIO_PIN_4
#define NRF24_CSN_GPIO_Port GPIOA


#endif /* INC_NRF24_CONFIG_H_ */
