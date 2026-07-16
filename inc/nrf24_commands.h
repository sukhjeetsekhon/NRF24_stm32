/**
   @file nrf24_commands.h

   @brief Declares the SPI Commands for the NRF24L01+ Radio Module

   @see Embedded\docs\pdf\nRF24L01P_Product_Specification_1_0.pdf

   Pages: 50-52
*/

#ifndef NRF24_COMMANDS_H
#define NRF24_COMMANDS_H

#include "nrf24_registers.h"
#include "nrf24_config.h"
#include "stm32_hal.h"
#include "stm32h7xx_hal_spi.h"

#define COMMAND_WORD_SIZE 1 // bytes
/* Maximum valid DPL payload width per datasheet Section 7.3.4 */
#define NRF24_MAX_PAYLOAD_WIDTH  32U
/* Valid ACK payload pipe range per datasheet Table 20: PPP = 000 to 101 */
#define NRF24_ACK_PAYLOAD_PIPE_MAX  5U

/*

   Every new command must be started by a high to low transition on CSN

   The STATUS register is serially shifted out on the MISO pin simultaneously to the SPI command word shifting to the MOSI pin.

   The serial shifting SPI commands is in the following format:
      <Command word: MSBit to LSBit (one byte)>
      <Data bytes: LSByte to MSByte, MSBit in each byte first>

*/

/**
   @brief drive the CSN signal HIGH
*/
void nrf24_write_csn_high(void);

/**
   @brief drive the CSN signal LOW
*/
void nrf24_write_csn_low(void);

/**
   @brief start a new spi command by driving CSN LOW

   @note this is the same as nrf24_write_csn_low()
*/
void nrf24_start_spi_command(void);

/**
   @brief end a spi command by driving CSN HIGH

   @note this is the same as nrf24_write_csn_high()
*/
void nrf24_end_spi_command(void);

/**
 * @brief  Read 1–5 bytes from an nRF24L01+ register via SPI.
 *
 * Per datasheet Section 8.3.1:
 *   - Command word (R_REGISTER | address) is sent on MOSI
 *   - STATUS register is simultaneously received on MISO
 *   - Data bytes follow: LSByte first, MSBit in each byte first
 *
 * @param  hspiX              SPI handle
 * @param  registerMapAddress 5-bit register address (AAAAA in 000AAAAA)
 * @param  status             Output: STATUS register byte received during command
 * @param  data               Output: buffer for register data (caller-allocated, min dataSize bytes)
 * @param  dataSize           Number of bytes to read (1 for most registers, up to 5 for address regs)
*/
void nrf24_read_register(
   SPI_HandleTypeDef *hspiX,
   const uint8_t registerMapAddress,
   uint8_t *status,
   uint8_t *data,
   const uint8_t dataSize
);

/**
 * @brief  Write 1–5 bytes to an nRF24L01+ register via SPI.
 *
 * Per datasheet Section 8.3.1:
 *   - Command word (W_REGISTER | address) sent on MOSI: encoding 001AAAAA
 *   - STATUS register simultaneously received on MISO during command byte
 *   - Data bytes follow: LSByte first, MSBit in each byte first
 *   - Executable in power down or standby modes only (per register map table)
 *
 * @param  hspiX              SPI handle
 * @param  registerMapAddress 5-bit register address (AAAAA in 001AAAAA)
 * @param  status             Output: STATUS register byte received during command phase
 * @param  data               Input:  buffer of bytes to write (caller-allocated, min dataSize bytes)
 * @param  dataSize           Number of bytes to write (1 for most registers, up to 5 for address regs)
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_write_register(
   SPI_HandleTypeDef *hspiX,
   const uint8_t     registerMapAddress,
   uint8_t          *status,
   const uint8_t    *data,
   const uint8_t     dataSize
);

/**
 * @brief  Read RX payload from the nRF24L01+ RX FIFO via SPI.
 *
 * Per datasheet Section 8.3.1 (R_RX_PAYLOAD command = 0b01100001):
 *   - Reads 1–32 bytes from RX FIFO, starting at byte 0
 *   - Payload is deleted from FIFO after it is read
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in RX mode
 *
 * For Dynamic Payload Length (DPL): caller must first issue R_RX_PL_WID
 * to determine payloadSize before calling this function. Per datasheet,
 * if R_RX_PL_WID returns > 32, the packet is corrupt — flush RX FIFO
 * with FLUSH_RX and do NOT call this function.
 *
 * For Static Payload Length: payloadSize must match the configured
 * RX_PW_Px register value for the receiving pipe.
 *
 * @param  hspiX       SPI handle
 * @param  status      Output: STATUS register byte received during command phase
 * @param  payload     Output: caller-allocated buffer, minimum payloadSize bytes
 * @param  payloadSize Number of bytes to read from RX FIFO (1–32)
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_read_rx_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *payload,
   const uint8_t      payloadSize
);

/**
 * @brief  Write TX payload to the nRF24L01+ TX FIFO via SPI.
 *
 * Per datasheet Section 8.3.1, Table 20 (W_TX_PAYLOAD = 0b10100000):
 *   - Writes 1–32 bytes into the TX FIFO, starting at byte 0
 *   - Data is LSByte first, MSBit in each byte first
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in TX mode only
 *
 * The TX FIFO has three 32-byte levels. Check FIFO_STATUS.TX_FULL
 * before calling to avoid overflowing the FIFO.
 *
 * After writing, pulse CE high for >= 10us to begin transmission.
 * If MAX_RT IRQ is asserted, the payload is NOT removed from TX FIFO —
 * caller must handle retransmission or call FLUSH_TX explicitly.
 *
 * @param  hspiX       SPI handle
 * @param  status      Output: STATUS register byte received during command phase
 * @param  payload     Input: caller-allocated buffer of bytes to write
 * @param  payloadSize Number of bytes to write to TX FIFO (1–32)
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_write_tx_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t     *payload,
   const uint8_t      payloadSize
);

/**
 * @brief  Flush the nRF24L01+ TX FIFO via SPI.
 *
 * Per datasheet Table 20 (FLUSH_TX = 0b11100001):
 *   - Zero data bytes — this is a command-only transaction
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in TX mode on PTX devices
 *   - Also valid on PRX devices to unblock a full TX FIFO
 *     (when all pending ACK payloads are addressed to a lost PTX link)
 *
 * Primary use cases:
 *   1. MAX_RT IRQ asserted — payload is NOT auto-removed from TX FIFO,
 *      caller must flush before loading a new payload
 *   2. PRX TX FIFO blocked — all pending ACK payloads addressed to
 *      an unreachable PTX
 *
 * Note: Per datasheet Table 20, FLUSH_RX "should not be executed during
 * transmission of acknowledge" — the same caution applies to FLUSH_TX:
 * do not call while a transmission is actively in progress.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte received during command phase
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_flush_tx(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

/**
 * @brief  Flush the nRF24L01+ RX FIFO via SPI.
 *
 * Per datasheet Table 20 (FLUSH_RX = 0b11100010):
 *   - Zero data bytes — this is a command-only transaction
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in RX mode
 *
 * !! WARNING !!
 * Per datasheet Table 20, FLUSH_RX "should not be executed during
 * transmission of acknowledge — the acknowledge package will not be
 * completed." Caller must ensure no ACK transmission is in progress
 * before calling this function.
 *
 * Primary use cases:
 *   1. Corrupt packet received — R_RX_PL_WID returned > 32 bytes,
 *      indicating a malformed packet that must be discarded:
 *
 *        if (payloadWidth > 32) {
 *            nrf24_flush_rx(hspi, &status);
 *        }
 *
 *   2. RX FIFO is full (FIFO_STATUS.RX_FULL = 1) and incoming packets
 *      are being discarded — flush to make room
 *   3. Startup/reset — ensure RX FIFO is clean before entering RX mode
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte received during command phase
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_flush_rx(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

/**
 * @brief  Command the nRF24L01+ to reuse the last transmitted TX payload.
 *
 * Per datasheet Table 20 (REUSE_TX_PL = 0b11100011):
 *   - Zero data bytes — this is a command-only transaction
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used on PTX devices only
 *
 * !! WARNINGS !!
 *   1. Per datasheet Table 20: "TX payload reuse must NOT be activated
 *      or deactivated during package transmission." Caller must ensure
 *      no transmission is actively in progress before calling.
 *
 *   2. This command sets the TX_REUSE flag in FIFO_STATUS (bit 6).
 *      TX_REUSE remains active until W_TX_PAYLOAD or FLUSH_TX is issued.
 *      Do NOT call nrf24_write_tx_payload() while TX_REUSE is active
 *      without intending to cancel reuse mode.
 *
 *   3. This command is an ALTERNATIVE to Auto Retransmit (ART).
 *      Per datasheet Section 7.4.2, the MCU must manually initiate
 *      each retransmission by pulsing CE >= 10us after this command.
 *
 * Typical usage pattern:
 *   nrf24_reuse_tx_pl(hspi, &status);        // activate reuse mode
 *   for (int i = 0; i < n; i++) {
 *       // pulse CE >= 10us to trigger each retransmission
 *       HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_SET);
 *       HAL_Delay(1);                         // 1ms >> 10us minimum
 *       HAL_GPIO_WritePin(CE_PORT, CE_PIN, GPIO_PIN_RESET);
 *       // wait for TX_DS IRQ before next pulse
 *   }
 *   nrf24_flush_tx(hspi, &status);           // deactivates TX_REUSE
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte received during command phase
 * @return HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure
 */
HAL_StatusTypeDef nrf24_reuse_tx_pl(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

/**
 * @brief  Read the payload width of the top payload in the RX FIFO (Dynamic Payload Length).
 *
 * Per datasheet Table 20 (R_RX_PL_WID = 0b01100000):
 *   - 1 data byte returned: the width in bytes of the top RX FIFO payload
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used with Dynamic Payload Length (DPL) feature
 *
 * !! CRITICAL WARNING (per datasheet Table 20 and Section 7.3.4) !!
 *   If the returned width is greater than 32 bytes, the packet is CORRUPT.
 *   In this case:
 *     1. Do NOT call nrf24_read_rx_payload() — the data is invalid
 *     2. Call nrf24_flush_rx() immediately to discard the corrupt packet
 *     3. This function returns HAL_ERROR to make the corrupt state
 *        impossible to ignore — *payloadWidth is set to 0 in this case
 *
 * This function should only be used when Dynamic Payload Length (DPL)
 * is enabled via the EN_DPL bit in the FEATURE register. For static
 * payload length, the width is determined by the RX_PW_Px registers
 * and does not need to be queried at runtime.
 *
 * Typical DPL receive pattern:
 *
 *   uint8_t status;
 *   uint8_t width;
 *   uint8_t payload[NRF24_MAX_PAYLOAD_WIDTH];
 *
 *   HAL_StatusTypeDef res = nrf24_read_rx_pl_wid(hspi, &status, &width);
 *   if (res != HAL_OK) {
 *       // width > 32: corrupt packet — flush and discard
 *       nrf24_flush_rx(hspi, &status);
 *       return;
 *   }
 *   nrf24_read_rx_payload(hspi, &status, payload, width);
 *
 * @param  hspiX        SPI handle
 * @param  status       Output: STATUS register byte received during command phase
 * @param  payloadWidth Output: width in bytes of top RX FIFO payload (valid range 1–32)
 *                      Set to 0 if the read value is corrupt (> 32 bytes)
 * @return HAL_OK       on success and valid width (1–32)
 *         HAL_ERROR    if payloadWidth > 32 (corrupt packet) OR SPI failure
 *         HAL_TIMEOUT  if SPI times out
 */
HAL_StatusTypeDef nrf24_read_rx_pl_wid(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *payloadWidth
);

/**
 * @brief  Write payload to be sent with the next ACK packet on a specific pipe (PRX only).
 *
 * Per datasheet Table 20 (W_ACK_PAYLOAD = 0b10101PPP):
 *   - Command byte encodes the target pipe number PPP in the lower 3 bits
 *   - 1 to 32 data bytes follow: LSByte first, MSBit in each byte first
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in RX mode (PRX) only
 *
 * !! PREREQUISITES (caller must configure before use) !!
 *   1. EN_ACK_PAY bit in FEATURE register (0x1D) must be set HIGH
 *   2. EN_DPL bit in FEATURE register (0x1D) must be set HIGH
 *      (ACK payloads always use Dynamic Payload Length)
 *   3. DPL_P0 bit in DYNPD register (0x1C) must be set on both PTX and PRX
 *      (ensures ACK packets with payloads are correctly received by PTX)
 *
 * !! PIPE NUMBER CONSTRAINT !!
 *   Per datasheet Table 20, PPP is valid ONLY in the range 000–101 (pipes 0–5).
 *   Pipes 6 and 7 do not exist — this function returns HAL_ERROR if
 *   pipeNumber > 5.
 *
 * !! FIFO BEHAVIOUR !!
 *   - Maximum 3 ACK payloads can be pending in TX FIFO (PRX) simultaneously
 *   - Multiple payloads for the same pipe are handled FIFO (first in, first out)
 *   - If TX FIFO (PRX) is blocked (all payloads addressed to a lost PTX link),
 *     call nrf24_flush_tx() to unblock it
 *
 * !! ARD (Auto Retransmit Delay) WARNING !!
 *   ACK payloads affect the minimum required ARD. Per datasheet Section 7.4.2:
 *     - 2Mbps, 5-byte address: max 15-byte ACK payload at ARD=250us
 *     - 1Mbps, 5-byte address: max  5-byte ACK payload at ARD=250us
 *     - 250kbps:               ARD must be >= 500us regardless of payload size
 *
 * @param  hspiX       SPI handle
 * @param  status      Output: STATUS register byte received during command phase
 * @param  pipeNumber  Target pipe number (0–5 only; 6–7 are invalid per datasheet)
 * @param  payload     Input: caller-allocated buffer of bytes to send with ACK
 * @param  payloadSize Number of bytes to write (1–32)
 * @return HAL_OK      on success
 *         HAL_ERROR   if pipeNumber > 5 (invalid) OR SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_write_ack_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t      pipeNumber,
   const uint8_t     *payload,
   const uint8_t      payloadSize
);

/**
 * @brief  Write TX payload with Auto Acknowledgement disabled for this packet.
 *
 * Per datasheet Table 20 (W_TX_PAYLOAD_NOACK = 0b10110000):
 *   - 1 to 32 data bytes: LSByte first, MSBit in each byte first
 *   - STATUS register is shifted out on MISO during the command byte
 *   - Used in TX mode (PTX) only
 *   - Sets the NO_ACK flag in the packet control field for this
 *     specific packet only — does not affect other packets in TX FIFO
 *
 * !! PREREQUISITE !!
 *   EN_DYN_ACK bit (bit 0) in FEATURE register (0x1D) MUST be set HIGH
 *   before this command will be accepted. Per datasheet Section 7.3.3.3:
 *   "The function must first be enabled in the FEATURE register by
 *   setting the EN_DYN_ACK bit."
 *
 * !! BEHAVIORAL DIFFERENCE vs nrf24_write_tx_payload() !!
 *   nrf24_write_tx_payload()  — PTX enters RX mode after TX to wait
 *                               for ACK; retransmits on failure;
 *                               MAX_RT IRQ possible.
 *   nrf24_write_tx_no_ack()   — PTX goes DIRECTLY to standby-I mode
 *                               after transmitting. Per datasheet
 *                               Section 7.3.3.3: "The PRX does not
 *                               transmit an ACK packet when it receives
 *                               the packet." No retransmit. No MAX_RT.
 *
 * This is a per-packet setting — it does not permanently disable
 * Auto Acknowledgement. Other payloads in the TX FIFO written with
 * nrf24_write_tx_payload() are unaffected.
 *
 * Suitable for:
 *   - Fire-and-forget sensor broadcasts
 *   - One-to-many topologies where ACK is impossible
 *   - Time-critical transmissions where retransmit latency is
 *     unacceptable
 *
 * Caller responsibilities:
 *   1. Set EN_DYN_ACK in FEATURE register before first use
 *   2. After writing payload, pulse CE >= 10us to begin transmission
 *   3. Monitor TX_DS IRQ — asserted immediately after TX completes
 *      (no ACK wait, so latency is deterministic)
 *   4. Check TX FIFO not full (FIFO_STATUS.TX_FULL) before writing
 *
 * @param  hspiX       SPI handle
 * @param  status      Output: STATUS register byte received during command phase
 * @param  payload     Input: caller-allocated buffer of bytes to transmit
 * @param  payloadSize Number of bytes to write to TX FIFO (1–32)
 * @return HAL_OK      on success
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_write_tx_no_ack(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t     *payload,
   const uint8_t      payloadSize
);

/**
 * @brief  Send a NOP command to the nRF24L01+, returning the STATUS register.
 *
 * Per datasheet Table 20 (NOP = 0b11111111):
 *   - Zero data bytes — command-only transaction
 *   - STATUS register is shifted out on MISO during the command byte
 *   - No operation is performed on the chip — zero side effects
 *
 * Primary use: read the STATUS register at any time, in any operating
 * mode, without disturbing FIFO state, radio mode, or any register.
 * Per datasheet Table 20: "Might be used to read the STATUS register."
 *
 * STATUS register bit layout (datasheet Section 9.1, address 0x07):
 *
 *   Bit 7   : Reserved (always 0)
 *   Bit 6   : RX_DR    — RX data ready IRQ (write 1 to clear)
 *   Bit 5   : TX_DS    — TX data sent IRQ  (write 1 to clear)
 *   Bit 4   : MAX_RT   — Max retransmits IRQ (write 1 to clear;
 *                        MUST clear before further TX is possible)
 *   Bits 3:1: RX_P_NO  — Pipe number of payload in RX FIFO
 *                        (000–101 = pipe 0–5, 110 = unused, 111 = RX FIFO empty)
 *   Bit 0   : TX_FULL  — TX FIFO full flag (1 = full, 0 = available slots)
 *
 * Typical usage patterns:
 *
 *   // 1. Poll IRQ flags without side effects
 *   uint8_t status;
 *   nrf24_nop(hspi, &status);
 *   if (status & (1 << 6)) { // RX_DR set — data ready in RX FIFO }
 *   if (status & (1 << 5)) { // TX_DS set — last TX acknowledged     }
 *   if (status & (1 << 4)) { // MAX_RT   — max retransmits reached   }
 *
 *   // 2. Check TX FIFO full before writing payload
 *   nrf24_nop(hspi, &status);
 *   if (!(status & (1 << 0))) {
 *       nrf24_write_tx_payload(hspi, &status, payload, size);
 *   }
 *
 *   // 3. Identify which pipe received data
 *   nrf24_nop(hspi, &status);
 *   uint8_t pipe = (status >> 1) & 0x07;  // extract RX_P_NO bits 3:1
 *   if (pipe <= 5) { // valid pipe number — data available }
 *
 * Note: To CLEAR IRQ flags, use nrf24_write_register() to write 1 to
 * the corresponding bit in the STATUS register (address 0x07).
 * NOP only READS — it never clears anything.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: current STATUS register value
 * @return HAL_OK      on success
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_nop(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
);

#endif /* NRF24_COMMANDS_H */