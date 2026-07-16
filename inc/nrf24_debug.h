/**
   @file nrf24_debug.h
   @brief Debugging functions for the NRF24L01+ Radio Module.
*/

#ifndef NRF24_DEBUG_H
#define NRF24_DEBUG_H

#include "nrf24_hal.h"

/**
 * @brief Read STATUS register via NOP — no side effects.
 *        Use when you only need the raw byte without printing.
 */
HAL_StatusTypeDef nrf24_read_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);

/**
 * @brief Read and print STATUS register with field-level decode.
 */
HAL_StatusTypeDef nrf24_print_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);

/**
 * @brief Read and print FIFO_STATUS register (TX/RX empty/full/reuse).
 */
HAL_StatusTypeDef nrf24_print_fifo_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);

/**
 * @brief Read and print OBSERVE_TX register (packet loss + retransmit counts).
 */
HAL_StatusTypeDef nrf24_print_observe_tx(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);

/**
 * @brief Dump all diagnostic registers in one call.
 *        First function to call when the radio is not behaving as expected.
 */
HAL_StatusTypeDef nrf24_dump_all(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);

#endif /* NRF24_DEBUG_H */