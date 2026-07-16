/**
   @file nrf24_modes.c

   @brief

   @see

*/

#include "nrf24_modes.h"
#include "nrf24_commands.h"

/* CONFIG register — datasheet Section 9.1, address 0x00 */
#define NRF24_REG_CONFIG            0x00U
#define NRF24_CONFIG_MASK_RX_DR     (1U << 6)
#define NRF24_CONFIG_MASK_TX_DS     (1U << 5)
#define NRF24_CONFIG_MASK_MAX_RT    (1U << 4)
#define NRF24_CONFIG_EN_CRC         (1U << 3)
#define NRF24_CONFIG_CRCO           (1U << 2)
#define NRF24_CONFIG_PWR_UP         (1U << 1)
#define NRF24_CONFIG_PRIM_RX        (1U << 0)

/* Timing constants — datasheet Table 16 */
#define NRF24_DELAY_STBY2A_MS       1U      /* Tstby2a = 130us; HAL_Delay min = 1ms */
#define NRF24_CE_PULSE_MS           1U      /* Thce >= 10us;    HAL_Delay min = 1ms */

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
   
   configValue &= ~NRF24_CONFIG_PWR_UP; /* clear bit 1 = PWR_UP */

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

void nrf24_enter_standby_mode_1(void) {
   /*
   * Pull CE low — this is the sole hardware requirement for Standby-I.
   * No SPI transaction is needed; no register is written.
   * Per datasheet state diagram (Section 6.1.1): CE = 0 drives the
   * transition from TX, RX, and Standby-II back to Standby-I.
   */
   HAL_GPIO_WritePin(
      CE_GPIO_PORT,
      CE_GPIO_PIN,
      GPIO_PIN_RESET      /* CE = LOW → enter Standby-I */
   );
}

void nrf24_enter_standby_mode_2(void) {
   /*
   * Hold CE HIGH continuously.
   * Per datasheet Table 15: Standby-II = PWR_UP=1, PRIM_RX=0, CE=1,
   * TX FIFO empty.
   *
   * !! PRECONDITIONS (caller must ensure before calling) !!
   *   1. Device is configured as PTX (PRIM_RX=0) — use nrf24_enter_tx_mode()
   *   2. TX FIFO is empty — if FIFO has data, CE=1 will trigger TX, not Standby-II
   *   3. PWR_UP=1 — device must be powered up
   *
   * Note: CE=1 on a PRX device (PRIM_RX=1) enters RX mode, NOT Standby-II.
   * Use nrf24_enter_rx_mode() for that path.
   */
   HAL_GPIO_WritePin(
      CE_GPIO_PORT,
      CE_GPIO_PIN,
      GPIO_PIN_SET
   );
}


HAL_StatusTypeDef nrf24_enter_rx_mode(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
) {
   HAL_StatusTypeDef result;
   uint8_t config = 0;

   /*
   * Step 1: Read current CONFIG register.
   * Read-modify-write is mandatory — other CONFIG bits (EN_CRC, CRCO,
   * MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT) must not be clobbered.
   */
   result = nrf24_read_register(
      hspiX,
      NRF24_REG_CONFIG,
      status,
      &config,
      1
   );
   if (result != HAL_OK) {
      return result;
   }

   /*
   * Step 2: Set PWR_UP and PRIM_RX bits high.
   * PWR_UP  (bit 1): 1 = powered up
   * PRIM_RX (bit 0): 1 = PRX (primary receiver)
   * All other bits preserved from the read above.
   */
   config |= NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX;

   /*
   * Step 3: Write modified CONFIG back.
   * Per datasheet Section 6.1.2, register writes are valid in
   * power down and standby modes — safe to write here.
   */
   result = nrf24_write_register(
      hspiX,
      NRF24_REG_CONFIG,
      status,
      &config,
      1
   );
   if (result != HAL_OK) {
      return result;
   }

   /*
   * Step 4: Assert CE high — activates RX mode.
   * Per datasheet Table 15: PWR_UP=1, PRIM_RX=1, CE=1 → RX mode.
   */
   HAL_GPIO_WritePin(
      CE_GPIO_PORT,
      CE_GPIO_PIN,
      GPIO_PIN_SET
   );

   /*
   * Step 5: Wait for Tstby2a — PLL settling time.
   * Per datasheet Table 16: standby → RX mode requires max 130us.
   * HAL_Delay is ms-resolution; 1ms >> 130us — safe upper bound.
   *
   * TODO: replace with a microsecond timer (TIM-based) for
   *       tighter timing if power consumption is a concern.
   */
   HAL_Delay(NRF24_DELAY_STBY2A_MS);   /* 1ms >= 130us Tstby2a */

   return HAL_OK;
}

void nrf24_exit_rx_mode(void) {
   /*
   * CE low → standby-I mode immediately.
   * Per datasheet Section 6.1.3.1:
   * "When CE is set low, the nRF24L01 returns to standby-I mode
   *  from both the TX and RX modes."
   */
   HAL_GPIO_WritePin(
      CE_GPIO_PORT,
      CE_GPIO_PIN,
      GPIO_PIN_RESET
   );
}

HAL_StatusTypeDef nrf24_enter_tx_mode(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
) {
   HAL_StatusTypeDef result;
   uint8_t configValue = 0;

   result = nrf24_read_register(hspiX, NRF24_REG_CONFIG, status, &configValue, 1);
   if (result != HAL_OK) { return result; }

   /* Set PWR_UP=1, PRIM_RX=0 → PTX configuration */
   configValue |=  NRF24_CONFIG_PWR_UP;
   configValue &= ~NRF24_CONFIG_PRIM_RX;

   result = nrf24_write_register(hspiX, NRF24_REG_CONFIG, status, &configValue, 1);
   if (result != HAL_OK) { return result; }

   /*
   * Wait for Tstby2a — PLL settling time.
   * Per datasheet Table 16: standby → TX mode requires max 130us.
   */
   HAL_Delay(NRF24_DELAY_STBY2A_MS);

   /*
   * !! CALLER MUST perform these steps after this function returns !!
   *
   * Per datasheet Section 6.1.5, TX mode requires ALL of:
   *   1. PWR_UP=1      ← done above
   *   2. PRIM_RX=0     ← done above
   *   3. Payload in TX FIFO  ← caller: nrf24_write_tx_payload()
   *   4. CE pulsed >= 10us   ← caller: GPIO_PIN_SET → delay → GPIO_PIN_RESET
   *
   * If CE is pulsed with an empty TX FIFO, the chip enters Standby-II,
   * NOT TX mode. Load the payload first, then pulse CE.
   *
   * Example:
   *   nrf24_enter_tx_mode(hspi, &status);
   *   nrf24_write_tx_payload(hspi, &status, payload, size);
   *   HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_SET);
   *   HAL_Delay(1);
   *   HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_RESET);
   */
   return HAL_OK;
}