/**
   @file nrf24.c

   @brief

   @see

*/

#include "nrf24.h"
#include "nrf24_commands.h"

HAL_StatusTypeDef nrf24_enter_power_down_mode(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    HAL_StatusTypeDef result;
    uint8_t configValue = 0;

    /*
     * Step 1: Read current CONFIG register value.
     * Preserves all bits we are not responsible for changing:
     * MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT, EN_CRC, CRCO, PRIM_RX.
     */
    result = nrf24_read_register(
        hspiX,
        NRF24_REG_CONFIG,       /* CONFIG register address = 0x00 */
        status,
        &configValue,
        1                       /* CONFIG is 1 byte wide */
    );

    if (result != HAL_OK) {
        return result;
    }

    /*
     * Step 2: Clear PWR_UP (bit 1) only.
     * Per datasheet Section 6.1.2: "Power down mode is entered by
     * setting the PWR_UP bit in the CONFIG register low."
     */
    configValue &= ~(1U << 1);  /* clear bit 1 = PWR_UP */

    /*
     * Step 3: Write modified CONFIG back.
     * All other bits are preserved exactly as read.
     */
    return nrf24_write_register(
        hspiX,
        NRF24_REG_CONFIG,
        status,
        &configValue,
        1                       /* CONFIG is 1 byte wide */
    );
}