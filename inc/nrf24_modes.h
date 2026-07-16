/**
   @file nrf24_modes.h

   @brief

   @see

   Pages: 22-23
*/

#ifndef NRF24_MODES_H
#define NRF24_MODES_H


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

/**
 * @brief  Enter Standby-I mode by pulling CE low.
 *
 * Per datasheet Section 6.1.3.1:
 *   "When CE is set low, the nRF24L01 returns to standby-I mode
 *    from both the TX and RX modes."
 *
 * Per datasheet Table 15, Standby-I requires:
 *   - PWR_UP = 1 in CONFIG register  (set by nrf24_power_up())
 *   - CE pin = LOW                   (set by this function)
 *
 * This is valid from ANY active mode:
 *   - From RX mode  → CE low immediately enters Standby-I
 *   - From TX mode  → CE low causes entry to Standby-I AFTER the
 *                     current packet finishes transmitting
 *   - From Standby-II → CE low immediately enters Standby-I
 *
 * Power consumption in Standby-I: 26µA (vs 900nA power down,
 * 13.5mA RX, 11.3mA TX at 0dBm).
 *
 * Re-entry to active mode from Standby-I takes max 130µs (Tstby2a).
 * This is much faster than from power down (Tpd2stby = 1.5ms–4.5ms).
 * Use Standby-I (not power down) when low-latency wake is needed.
 *
 * !! PREREQUISITE !!
 *   nrf24_power_up() must have been called and the 1.5ms crystal
 *   oscillator startup delay must have elapsed before this is
 *   meaningful. nrf24_init() handles this.
 *
 * !! TX IN PROGRESS !!
 *   If called while a packet transmission is in progress, the chip
 *   completes the current packet before entering Standby-I.
 *   Per datasheet Section 6.1.5: "If CE = 0, nRF24L01+ returns
 *   to standby-I mode" after the current TX completes.
 *   Do NOT immediately re-assert CE assuming Standby-I is instant.
 */
void nrf24_enter_standby_mode_1(void);

/**
 * @brief  Place the nRF24L01+ into Standby-II mode (PTX only).
 *
 * Per datasheet Section 6.1.3.2 and Table 15:
 *   Standby-II requires:
 *     PWR_UP  = 1  (device powered up)
 *     PRIM_RX = 0  (PTX mode — not PRX)
 *     CE      = 1  (held HIGH continuously — NOT a pulse)
 *     TX FIFO = empty
 *
 * In Standby-II, extra clock buffers are active. Current consumption
 * is higher than Standby-I (320µA vs 26µA per datasheet Table 4) but
 * the device is primed for the fastest possible TX response:
 *
 *   "If a new packet is uploaded to the TX FIFO, the PLL immediately
 *    starts and the packet is transmitted after the normal PLL settling
 *    delay (130µs)." — Section 6.1.3.2
 *
 * !! PREREQUISITES (caller must ensure before calling) !!
 *   1. PWR_UP bit = 1 in CONFIG register — device must be powered up
 *   2. PRIM_RX bit = 0 in CONFIG register — must be in PTX role
 *   3. TX FIFO must be EMPTY — if TX FIFO is not empty when CE goes
 *      high, the device enters TX mode and begins transmitting instead
 *      of entering Standby-II. Call nrf24_flush_tx() first if needed.
 *
 * !! CE PIN BEHAVIOUR !!
 *   Unlike TX mode entry (CE pulsed >= 10us), Standby-II requires CE
 *   to be held HIGH continuously. The device remains in Standby-II
 *   until either:
 *     a) CE is set LOW  → transitions to Standby-I
 *     b) A payload is written to TX FIFO via W_TX_PAYLOAD → PLL starts,
 *        transitions to TX mode after 130µs (Tstby2a)
 *
 * !! POWER CONSUMPTION WARNING !!
 *   Per datasheet Table 4:
 *     Standby-I  = 26µA
 *     Standby-II = 320µA  ← ~12x more than Standby-I
 *   Do not use Standby-II when low power is a priority and TX
 *   response latency of 130µs (from Standby-I) is acceptable.
 *
 * Typical usage pattern:
 *
 *   // Flush TX FIFO first to guarantee Standby-II entry (not TX mode)
 *   nrf24_flush_tx(hspi, &status);
 *
 *   // Enter Standby-II — CE held high
 *   nrf24_enter_standby_mode_2();
 *
 *   // Later: load a payload — PLL starts immediately, TX fires in 130µs
 *   nrf24_write_tx_payload(hspi, &status, payload, size);
 *
 *   // Exit Standby-II to Standby-I when done
 *   nrf24_enter_standby_mode_1();
 */
void nrf24_enter_standby_mode_2(void);

/**
 * @brief  Configure nRF24L01+ for RX mode and begin listening.
 *
 * Implements the full RX mode entry sequence per datasheet Section 6.1.4
 * and Table 15:
 *
 *   Step 1 — Read current CONFIG register (preserves all other bits)
 *   Step 2 — Set PWR_UP (bit 1) and PRIM_RX (bit 0) high
 *   Step 3 — Write CONFIG back via SPI
 *   Step 4 — Assert CE high (activates RX mode)
 *   Step 5 — Wait >= 130us (Tstby2a: PLL settling, per datasheet Table 16)
 *
 * After this function returns, the receiver is actively listening on the
 * configured frequency channel and pipe addresses.
 *
 * To return to standby-I mode, call nrf24_exit_rx_mode() which pulls
 * CE low. Per datasheet Section 6.1.4:
 * "The nRF24L01+ remains in RX mode until the MCU configures it to
 *  standby-I mode or power down mode."
 *
 * !! PREREQUISITES (must be configured before calling) !!
 *   - RF channel set via RF_CH register (nrf24_write_register)
 *   - RX pipe addresses set via RX_ADDR_Px registers
 *   - Pipes enabled via EN_RXADDR register
 *   - Payload widths set via RX_PW_Px registers (static payload length)
 *     OR EN_DPL enabled (dynamic payload length)
 *   - Auto acknowledgement configured via EN_AA register
 *
 * !! TIMING !!
 *   This function blocks for >= 130us after asserting CE to satisfy
 *   Tstby2a (datasheet Table 16). Do not call in hard real-time
 *   contexts where this delay is unacceptable.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte from CONFIG write command phase
 * @return HAL_OK      on success — device is now in RX mode
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_enter_rx_mode(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

/**
 * @brief  Exit RX mode — return nRF24L01+ to standby-I.
 *
 * Pulls CE low per datasheet Section 6.1.4:
 * "The nRF24L01+ remains in RX mode until the MCU configures it to
 *  standby-I mode or power down mode."
 *
 * In standby-I mode:
 *   - SPI remains accessible
 *   - Register values are maintained
 *   - Current consumption drops to 26uA (vs 12.6–13.5mA in RX mode)
 *
 * Call nrf24_enter_rx_mode() to resume listening.
 */
void nrf24_exit_rx_mode(void);

/**
 * @brief  Transition the nRF24L01+ into TX mode and transmit one packet.
 *
 * Implements the PTX activation sequence per datasheet Section 6.1.5
 * and Table 15 (single-packet TX mode: CE pulsed >= 10us):
 *
 *   1. Read current CONFIG register (preserve all other bits)
 *   2. Clear PRIM_RX bit → selects PTX mode
 *   3. Write CONFIG back via nrf24_write_register()
 *   4. Wait Tstby2a (130us min) for PLL to settle per Table 16
 *   5. Pulse CE high >= 10us (Thce) to trigger transmission
 *   6. Pull CE low → nRF24L01+ returns to standby-I after packet TX
 *
 * !! PRECONDITIONS (caller must satisfy before calling) !!
 *   1. PWR_UP bit in CONFIG must be HIGH — call nrf24_power_up() first.
 *      After power-up, wait Tpd2stby (max 1.5ms) before entering TX mode
 *      per datasheet Table 16.
 *   2. TX FIFO must contain at least one payload — call
 *      nrf24_write_tx_payload() or nrf24_write_tx_no_ack() first.
 *      Per Table 15: "Data in TX FIFOs. Will empty one level in TX FIFOs."
 *   3. For Enhanced ShockBurst with ACK: RX_ADDR_P0 must equal TX_ADDR
 *      so the PTX can receive the ACK packet. Per datasheet Appendix A:
 *      "RX_ADDR_P0 must be equal to TX_ADDR in the PTX device."
 *
 * !! CE PULSE MODE vs CONTINUOUS TX !!
 *   This function uses the SINGLE-PACKET pulse mode (CE >= 10us then low).
 *   Per Table 15, this transmits ONE payload level from the TX FIFO and
 *   then returns to standby-I mode. This is the recommended operating mode.
 *
 *   Do NOT hold CE high externally while calling this function — that would
 *   engage continuous TX mode, which risks exceeding the 4ms TX time limit.
 *   Per Section 6.1.5: "It is important never to keep the nRF24L01+ in TX
 *   mode for more than 4ms at a time."
 *
 * !! AFTER THIS CALL !!
 *   Poll STATUS register using nrf24_nop() for:
 *     - TX_DS  (bit 5) — packet transmitted and ACK received (if AA enabled)
 *     - MAX_RT (bit 4) — max retransmits reached; payload still in TX FIFO
 *   Clear the asserted IRQ flag with nrf24_clear_tx_ds() or
 *   nrf24_clear_max_rt() before the next transmission.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte from CONFIG write phase
 * @return HAL_OK      on success
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_enter_tx_mode(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

#endif /* NRF24_MODES_H */