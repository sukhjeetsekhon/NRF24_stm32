/**
   @file nrf24_config.c

   @brief Defines functions to configure the NRF24L01+ Radio Module.

   @see docs\nRF24L01P_Product_Specification_1_0.pdf
        Section 6.2 — Air data rate          (p. 25)
        Section 6.3 — RF channel frequency   (p. 25)
        Section 6.4 — Received Power Detector(p. 25)
        Section 6.5 — PA control             (p. 26)
        Section 6.6 — RX/TX control          (p. 26)

   Pages: 25-26
*/

#include "nrf24_config.h"
#include "nrf24_commands.h"


HAL_StatusTypeDef nrf24_set_air_data_rate(
   SPI_HandleTypeDef   *hspiX,
   uint8_t             *status,
   nrf24_data_rate_t    dataRate
) {
   HAL_StatusTypeDef result;
   uint8_t rfSetup = 0;

   /*
   * Step 1: Read current RF_SETUP register.
   * Read-modify-write is mandatory — preserves CONT_WAVE, PLL_LOCK, RF_PWR.
   */
   result = nrf24_read_register(hspiX, NRF24_REG_RF_SETUP, status, &rfSetup, 1);
   if (result != HAL_OK) {
      return result;
   }

   /* Step 2: Clear both data rate bits before applying the new value */
   rfSetup &= ~NRF24_RF_SETUP_RF_DR_MASK;

   /*
   * Step 3: Apply the new data rate encoding.
   * Per datasheet Section 6.2 Table 14:
   *   250 kbps → RF_DR_LOW=1, RF_DR_HIGH=0
   *   1 Mbps   → RF_DR_LOW=0, RF_DR_HIGH=0
   *   2 Mbps   → RF_DR_LOW=0, RF_DR_HIGH=1
   */
   switch (dataRate) {
      case NRF24_DATA_RATE_250KBPS:
         rfSetup |= NRF24_RF_SETUP_RF_DR_LOW;   /* bit 5 = 1, bit 3 = 0 */
         break;

      case NRF24_DATA_RATE_1MBPS:
         /* Both bits already cleared above — nothing to set */
         break;

      case NRF24_DATA_RATE_2MBPS:
         rfSetup |= NRF24_RF_SETUP_RF_DR_HIGH;  /* bit 5 = 0, bit 3 = 1 */
         break;

      default:
         return HAL_ERROR;   /* reject any value not in the enum */
   }

   /* Step 4: Write modified RF_SETUP back */
   return nrf24_write_register(hspiX, NRF24_REG_RF_SETUP, status, &rfSetup, 1);
}

HAL_StatusTypeDef nrf24_set_rf_channel_freq(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t            channel
) {
   if (channel > NRF24_RF_CH_MAX) {
      return HAL_ERROR;
   }

   /*
   * RF_CH is a single-byte register. No read-modify-write needed —
   * bits [7] is reserved and must be written 0; channel fits in [6:0].
   * Writing the channel value directly is safe since bit 7 of a valid
   * channel (0x00–0x7F) is always 0.
   */
   return nrf24_write_register(hspiX, NRF24_REG_RF_CH, status, &channel, 1);
}

HAL_StatusTypeDef nrf24_read_rpd(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *rpd
) {
   HAL_StatusTypeDef result;
   uint8_t rpdRegister = 0;

   result = nrf24_read_register(hspiX, NRF24_REG_RPD, status, &rpdRegister, 1);
   if (result != HAL_OK) {
      return result;
   }

   /*
   * Mask bit 0 only — all other bits in the RPD register are reserved.
   * Caller receives a clean 0 or 1, not the raw register byte.
   */
   *rpd = (rpdRegister & NRF24_RPD_BIT) ? 1U : 0U;

   return HAL_OK;
}

HAL_StatusTypeDef nrf24_set_output_pwr(
   SPI_HandleTypeDef   *hspiX,
   uint8_t             *status,
   nrf24_output_pwr_t   outputPwr
) {
   HAL_StatusTypeDef result;
   uint8_t rfSetup = 0;

   /*
   * Step 1: Read current RF_SETUP register.
   * Read-modify-write is mandatory — preserves CONT_WAVE, PLL_LOCK, RF_DR.
   */
   result = nrf24_read_register(hspiX, NRF24_REG_RF_SETUP, status, &rfSetup, 1);
   if (result != HAL_OK) {
      return result;
   }

   /* Step 2: Validate the enum value before writing anything */
   switch (outputPwr) {
      case NRF24_OUTPUT_PWR_0DBM:
      case NRF24_OUTPUT_PWR_NEG6DBM:
      case NRF24_OUTPUT_PWR_NEG12DBM:
      case NRF24_OUTPUT_PWR_NEG18DBM:
         break;
      default:
         return HAL_ERROR;
   }

   /*
   * Step 3: Clear RF_PWR bits [2:1], then write the new value.
   * The enum values are pre-shifted to sit in bits [2:1] — see nrf24_config.h.
   */
   rfSetup &= ~NRF24_RF_SETUP_RF_PWR_MASK;
   rfSetup |=  (uint8_t)outputPwr;

   /* Step 4: Write modified RF_SETUP back */
   return nrf24_write_register(hspiX, NRF24_REG_RF_SETUP, status, &rfSetup, 1);
}

HAL_StatusTypeDef nrf24_rxtx_control(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   nrf24_role_t       role
) {
   HAL_StatusTypeDef result;
   uint8_t configValue = 0;

   /*
   * Step 1: Read current CONFIG register.
   * Preserves MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT, EN_CRC, CRCO, PWR_UP.
   */
   result = nrf24_read_register(hspiX, NRF24_REG_CONFIG, status, &configValue, 1);
   if (result != HAL_OK) {
      return result;
   }

   /*
   * Step 2: Set or clear PRIM_RX bit only.
   * Per datasheet Section 6.6: PRIM_RX=1 → PRX, PRIM_RX=0 → PTX.
   */
   switch (role) {
      case NRF24_ROLE_PRX:
         configValue |=  NRF24_CONFIG_PRIM_RX;
         break;

      case NRF24_ROLE_PTX:
         configValue &= ~NRF24_CONFIG_PRIM_RX;
         break;

      default:
         return HAL_ERROR;
   }

   /* Step 3: Write modified CONFIG back */
   return nrf24_write_register(hspiX, NRF24_REG_CONFIG, status, &configValue, 1);
}