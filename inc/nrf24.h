/**
   @file nrf24.h

   @brief

   @see

   Pages: 22-23
*/

#ifndef NRF24_H
#define NRF24_H


/**
 * @brief  Enter nRF24L01+ Power Down mode by clearing PWR_UP (bit 1) in CONFIG.
 *
 * Per datasheet Section 6.1.2:
 *   "Power down mode is entered by setting the PWR_UP bit in the CONFIG
 *    register low."
 *
 * Uses read-modify-write to preserve all other CONFIG bits:
 *   MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT, EN_CRC, CRCO, PRIM_RX.
 *
 * In power down mode:
 *   - Minimal current consumption (900nA typ per datasheet Table 4)
 *   - All register values are maintained
 *   - SPI remains active — registers can still be read/written
 *
 * Per datasheet Table 16, re-entering TX or RX mode from power down
 * requires passing through standby-I first, with a delay of Tpd2stby
 * (max 1.5ms for crystal Ls < 30mH) before asserting CE.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte received during command phase
 * @return HAL_OK      on success
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_enter_power_down_mode(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
);


#endif /* NRF24_H */