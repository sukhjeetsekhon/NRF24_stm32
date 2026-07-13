/**

   @file nrf24_config.h

   @brief STM32 peripheral configuration for SPI, TIM, and GPIO

   @note make sure the information matches your STM32 configuration

   @see docs\nRF24L01P_Product_Specification_1_0.pdf

   Pages: 11

*/

#ifndef NRF24_CONFIG_H
#define NRF24_CONFIG_H

#define hspiX hspi1             // change number to your configured SPI peripheral
#define spi_w_timeout 1000      // milliseconds                                   
#define spi_r_timeout 1000      // milliseconds                                   
#define spi_rw_timeout 1000     // milliseconds                                   

#define csn_gpio_port GPIOA     // change port letter to your configured GPIO port
#define csn_gpio_pin GPIO_PIN_3 // change pin number to your configured GPIO pin  

#define ce_gpio_port GPIOA      // change port letter to your configured GPIO port
#define ce_gpio_pin GPIO_PIN_4  // change pin number to your configured GPIO pin  

#define htimX htim1             // change number to your configured timer

#endif /* NRF24_CONFIG_H */
