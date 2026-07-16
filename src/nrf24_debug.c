/**
   @file nrf24_debug.c

   @brief Define debugging functions for the NRF24L01+ Radio Module.

   @see docs\nRF24L01P_Product_Specification_1_0.pdf
        Section 8.5  — Interrupt / STATUS register      (p. 57)
        Section 9.1  — Register map table               (p. 58-64)
        Register 0x07 STATUS
        Register 0x08 OBSERVE_TX
        Register 0x09 RPD
        Register 0x17 FIFO_STATUS
*/

#include "nrf24_debug.h"
#include "nrf24_commands.h"
#include <stdio.h>      /* printf — replace with your UART logger if needed */

/* ── Register addresses — datasheet Section 9.1 ────────────────────────── */
#define NRF24_REG_CONFIG        0x00U
#define NRF24_REG_EN_AA         0x01U
#define NRF24_REG_EN_RXADDR     0x02U
#define NRF24_REG_SETUP_AW      0x03U
#define NRF24_REG_SETUP_RETR    0x04U
#define NRF24_REG_RF_CH         0x05U
#define NRF24_REG_RF_SETUP      0x06U
#define NRF24_REG_STATUS        0x07U
#define NRF24_REG_OBSERVE_TX    0x08U
#define NRF24_REG_RPD           0x09U
#define NRF24_REG_FIFO_STATUS   0x17U

/* ── STATUS register bits (0x07) — datasheet Section 9.1 ───────────────── */
#define NRF24_STATUS_RX_DR      (1U << 6)   /* RX data ready               */
#define NRF24_STATUS_TX_DS      (1U << 5)   /* TX data sent                */
#define NRF24_STATUS_MAX_RT     (1U << 4)   /* Max retransmits reached      */
#define NRF24_STATUS_RX_P_NO    (0x07U << 1)/* RX pipe number, bits [3:1]  */
#define NRF24_STATUS_TX_FULL    (1U << 0)   /* TX FIFO full                 */

/* ── OBSERVE_TX register bits (0x08) — datasheet Section 9.1 ───────────── */
/*
 * PLOS_CNT [7:4] — count of lost packets (overflow-protected at 15).
 *                  Reset by writing to RF_CH register.
 * ARC_CNT  [3:0] — count of retransmits for the current packet.
 *                  Reset when a new packet transmission starts.
 */
#define NRF24_OBSERVE_TX_PLOS_CNT_MASK (0x0FU << 4)
#define NRF24_OBSERVE_TX_ARC_CNT_MASK  (0x0FU)

/* ── FIFO_STATUS register bits (0x17) — datasheet Section 9.1 ──────────── */
#define NRF24_FIFO_TX_REUSE     (1U << 6)   /* TX payload reuse active      */
#define NRF24_FIFO_TX_FULL      (1U << 5)   /* TX FIFO full                 */
#define NRF24_FIFO_TX_EMPTY     (1U << 4)   /* TX FIFO empty                */
#define NRF24_FIFO_RX_FULL      (1U << 1)   /* RX FIFO full                 */
#define NRF24_FIFO_RX_EMPTY     (1U << 0)   /* RX FIFO empty                */

/* ── CONFIG register bits (0x00) — datasheet Section 9.1 ───────────────── */
#define NRF24_CONFIG_MASK_RX_DR (1U << 6)
#define NRF24_CONFIG_MASK_TX_DS (1U << 5)
#define NRF24_CONFIG_MASK_MAX_RT(1U << 4)
#define NRF24_CONFIG_EN_CRC     (1U << 3)
#define NRF24_CONFIG_CRCO       (1U << 2)
#define NRF24_CONFIG_PWR_UP     (1U << 1)
#define NRF24_CONFIG_PRIM_RX    (1U << 0)

/* ── RF_SETUP register bits (0x06) — datasheet Section 9.1 ─────────────── */
#define NRF24_RF_DR_LOW         (1U << 5)
#define NRF24_RF_DR_HIGH        (1U << 3)
#define NRF24_RF_PWR_MASK       (0x03U << 1)


/* ─────────────────────────────────────────────────────────────────────────
 * nrf24_read_status
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Read and decode the STATUS register (0x07).
 *
 * Per datasheet Section 9.1: STATUS is shifted out on MISO simultaneously
 * with every SPI command word. This function uses NOP to read it without
 * any side effects.
 *
 * STATUS bit layout:
 *   Bit 6 : RX_DR   — RX data ready (write 1 to clear)
 *   Bit 5 : TX_DS   — TX data sent  (write 1 to clear)
 *   Bit 4 : MAX_RT  — Max retransmits (write 1 to clear; MUST clear to
 *                     re-enable TX)
 *   Bits 3:1 : RX_P_NO — Pipe number with payload ready
 *              000–101 = pipe 0–5, 111 = RX FIFO empty
 *   Bit 0 : TX_FULL — TX FIFO full
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: raw STATUS register byte
 * @return HAL_OK / HAL_ERROR / HAL_TIMEOUT
 */
HAL_StatusTypeDef nrf24_read_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    /*
     * NOP command — zero data bytes, no side effects.
     * STATUS is always returned on MISO during the command byte.
     * Per datasheet Table 20: "Might be used to read the STATUS register."
     */
    return nrf24_nop(hspiX, status);
}


/* ─────────────────────────────────────────────────────────────────────────
 * nrf24_print_status
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Read the STATUS register and print a human-readable field decode.
 *
 * Example output:
 *   [NRF24 STATUS] raw=0x0E
 *     RX_DR   : 0  (no RX data ready)
 *     TX_DS   : 0  (TX not complete / not sent)
 *     MAX_RT  : 0  (no max retransmit)
 *     RX_P_NO : 7  (RX FIFO empty)
 *     TX_FULL : 0  (TX FIFO has space)
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: raw STATUS byte
 * @return HAL_OK / HAL_ERROR / HAL_TIMEOUT
 */
HAL_StatusTypeDef nrf24_print_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    HAL_StatusTypeDef result = nrf24_nop(hspiX, status);
    if (result != HAL_OK) {
        printf("[NRF24 STATUS] SPI error — could not read\r\n");
        return result;
    }

    uint8_t rx_p_no = (*status & NRF24_STATUS_RX_P_NO) >> 1;

    printf("[NRF24 STATUS] raw=0x%02X\r\n", *status);
    printf("  RX_DR   : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_RX_DR)  ? 1 : 0),
        (*status & NRF24_STATUS_RX_DR)  ? "RX data ready in FIFO"   : "no RX data ready");
    printf("  TX_DS   : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_TX_DS)  ? 1 : 0),
        (*status & NRF24_STATUS_TX_DS)  ? "packet sent + ACK received" : "TX not complete");
    printf("  MAX_RT  : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_MAX_RT) ? 1 : 0),
        (*status & NRF24_STATUS_MAX_RT) ? "!! MAX retransmits reached — TX blocked !!" : "ok");
    printf("  RX_P_NO : %u  (%s)\r\n",
        (unsigned)rx_p_no,
        (rx_p_no == 0x07) ? "RX FIFO empty" :
        (rx_p_no == 0x06) ? "unused"         : "payload available");
    printf("  TX_FULL : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_TX_FULL) ? 1 : 0),
        (*status & NRF24_STATUS_TX_FULL) ? "TX FIFO full" : "TX FIFO has space");

    return HAL_OK;
}


/* ─────────────────────────────────────────────────────────────────────────
 * nrf24_print_fifo_status
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Read and decode the FIFO_STATUS register (0x17).
 *
 * Per datasheet Section 9.1, FIFO_STATUS contains:
 *   Bit 6 : TX_REUSE  — reuse last TX payload active
 *   Bit 5 : TX_FULL   — TX FIFO full
 *   Bit 4 : TX_EMPTY  — TX FIFO empty  (reset value: 1)
 *   Bit 1 : RX_FULL   — RX FIFO full
 *   Bit 0 : RX_EMPTY  — RX FIFO empty  (reset value: 1)
 *
 * Note: STATUS (0x07) also has a TX_FULL flag, but FIFO_STATUS is the
 * only register that exposes TX_EMPTY, RX_FULL, RX_EMPTY, and TX_REUSE.
 *
 * Example output:
 *   [NRF24 FIFO_STATUS] raw=0x11
 *     TX_REUSE : 0  (not active)
 *     TX_FULL  : 0  (space available)
 *     TX_EMPTY : 1  (TX FIFO empty)
 *     RX_FULL  : 0  (space available)
 *     RX_EMPTY : 1  (RX FIFO empty)
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS byte (returned on MISO during command)
 * @return HAL_OK / HAL_ERROR / HAL_TIMEOUT
 */
HAL_StatusTypeDef nrf24_print_fifo_status(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    uint8_t fifoStatus = 0;

    HAL_StatusTypeDef result = nrf24_read_register(
        hspiX, NRF24_REG_FIFO_STATUS, status, &fifoStatus, 1
    );
    if (result != HAL_OK) {
        printf("[NRF24 FIFO_STATUS] SPI error — could not read\r\n");
        return result;
    }

    printf("[NRF24 FIFO_STATUS] raw=0x%02X\r\n", fifoStatus);
    printf("  TX_REUSE : %u  (%s)\r\n",
        (unsigned)((fifoStatus & NRF24_FIFO_TX_REUSE) ? 1 : 0),
        (fifoStatus & NRF24_FIFO_TX_REUSE) ? "reuse active"  : "not active");
    printf("  TX_FULL  : %u  (%s)\r\n",
        (unsigned)((fifoStatus & NRF24_FIFO_TX_FULL)  ? 1 : 0),
        (fifoStatus & NRF24_FIFO_TX_FULL)  ? "TX FIFO full"  : "space available");
    printf("  TX_EMPTY : %u  (%s)\r\n",
        (unsigned)((fifoStatus & NRF24_FIFO_TX_EMPTY) ? 1 : 0),
        (fifoStatus & NRF24_FIFO_TX_EMPTY) ? "TX FIFO empty" : "data in TX FIFO");
    printf("  RX_FULL  : %u  (%s)\r\n",
        (unsigned)((fifoStatus & NRF24_FIFO_RX_FULL)  ? 1 : 0),
        (fifoStatus & NRF24_FIFO_RX_FULL)  ? "RX FIFO full"  : "space available");
    printf("  RX_EMPTY : %u  (%s)\r\n",
        (unsigned)((fifoStatus & NRF24_FIFO_RX_EMPTY) ? 1 : 0),
        (fifoStatus & NRF24_FIFO_RX_EMPTY) ? "RX FIFO empty" : "data in RX FIFO");

    return HAL_OK;
}


/* ─────────────────────────────────────────────────────────────────────────
 * nrf24_print_observe_tx
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Read and decode the OBSERVE_TX register (0x08).
 *
 * Per datasheet Section 9.1:
 *   PLOS_CNT [7:4] — total lost packets since last RF_CH write.
 *                    Overflow-protected at 15. Reset by writing RF_CH.
 *   ARC_CNT  [3:0] — retransmit count for the current packet.
 *                    Reset when a new packet transmission starts.
 *
 * These two counters together tell you:
 *   - ARC_CNT > 0 → link is marginal, retransmits are occurring
 *   - PLOS_CNT = 15 → significant packet loss, channel may be congested
 *   - ARC_CNT = 15 → MAX_RT about to fire or already fired
 *
 * Example output:
 *   [NRF24 OBSERVE_TX] raw=0x03
 *     PLOS_CNT : 0   (lost packets since last channel change)
 *     ARC_CNT  : 3   (retransmits for current packet)
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS byte
 * @return HAL_OK / HAL_ERROR / HAL_TIMEOUT
 */
HAL_StatusTypeDef nrf24_print_observe_tx(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    uint8_t observeTx = 0;

    HAL_StatusTypeDef result = nrf24_read_register(
        hspiX, NRF24_REG_OBSERVE_TX, status, &observeTx, 1
    );
    if (result != HAL_OK) {
        printf("[NRF24 OBSERVE_TX] SPI error — could not read\r\n");
        return result;
    }

    uint8_t plos = (observeTx & NRF24_OBSERVE_TX_PLOS_CNT_MASK) >> 4;
    uint8_t arc  = (observeTx & NRF24_OBSERVE_TX_ARC_CNT_MASK);

    printf("[NRF24 OBSERVE_TX] raw=0x%02X\r\n", observeTx);
    printf("  PLOS_CNT : %2u  (lost packets since last channel change%s)\r\n",
        (unsigned)plos,
        (plos == 15) ? " — OVERFLOW, counter saturated" : "");
    printf("  ARC_CNT  : %2u  (retransmits for current packet%s)\r\n",
        (unsigned)arc,
        (arc == 15)  ? " — at maximum" : "");

    return HAL_OK;
}


/* ─────────────────────────────────────────────────────────────────────────
 * nrf24_dump_all
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Read and print all diagnostic registers in one call.
 *
 * Reads and decodes:
 *   0x00  CONFIG       — power, role, CRC, IRQ masks
 *   0x05  RF_CH        — channel frequency (F = 2400 + RF_CH MHz)
 *   0x06  RF_SETUP     — data rate and output power
 *   0x07  STATUS       — IRQ flags, RX pipe, TX FIFO full
 *   0x08  OBSERVE_TX   — packet loss and retransmit counters
 *   0x09  RPD          — received power detector (> -64dBm?)
 *   0x17  FIFO_STATUS  — TX/RX FIFO empty/full/reuse
 *
 * This is the first function to call when the radio is not behaving as
 * expected. A single call gives a complete snapshot of radio state.
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS byte from the last SPI transaction
 * @return HAL_OK if all reads succeeded; first error code otherwise
 */
HAL_StatusTypeDef nrf24_dump_all(
    SPI_HandleTypeDef *hspiX,
    uint8_t           *status
) {
    HAL_StatusTypeDef result;
    uint8_t reg = 0;

    printf("\r\n========================================\r\n");
    printf("        NRF24L01+ REGISTER DUMP\r\n");
    printf("========================================\r\n");

    /* ── CONFIG (0x00) ──────────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_CONFIG, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    uint8_t rf_dr_low  = (reg & NRF24_RF_DR_LOW)  ? 1 : 0; /* unused here, for RF_SETUP */
    printf("\r\n[0x00 CONFIG] raw=0x%02X\r\n", reg);
    printf("  MASK_RX_DR : %u  (IRQ on RX_DR %s)\r\n",
        (unsigned)((reg >> 6) & 1), ((reg >> 6) & 1) ? "masked"    : "enabled");
    printf("  MASK_TX_DS : %u  (IRQ on TX_DS %s)\r\n",
        (unsigned)((reg >> 5) & 1), ((reg >> 5) & 1) ? "masked"    : "enabled");
    printf("  MASK_MAX_RT: %u  (IRQ on MAX_RT %s)\r\n",
        (unsigned)((reg >> 4) & 1), ((reg >> 4) & 1) ? "masked"    : "enabled");
    printf("  EN_CRC     : %u  (CRC %s)\r\n",
        (unsigned)((reg >> 3) & 1), ((reg >> 3) & 1) ? "enabled"   : "disabled");
    printf("  CRCO       : %u  (%s)\r\n",
        (unsigned)((reg >> 2) & 1), ((reg >> 2) & 1) ? "2-byte CRC" : "1-byte CRC");
    printf("  PWR_UP     : %u  (%s)\r\n",
        (unsigned)((reg >> 1) & 1), ((reg >> 1) & 1) ? "POWER UP"  : "POWER DOWN");
    printf("  PRIM_RX    : %u  (%s)\r\n",
        (unsigned)(reg & 1),        (reg & 1)         ? "PRX mode"  : "PTX mode");

    /* ── RF_CH (0x05) ───────────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_RF_CH, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    printf("\r\n[0x05 RF_CH] raw=0x%02X\r\n", reg);
    printf("  RF_CH      : %u  (F = %u MHz)\r\n",
        (unsigned)(reg & 0x7F), (unsigned)(2400 + (reg & 0x7F)));

    /* ── RF_SETUP (0x06) ────────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_RF_SETUP, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    rf_dr_low = (reg & NRF24_RF_DR_LOW) ? 1 : 0;
    uint8_t rf_dr_high = (reg & NRF24_RF_DR_HIGH) ? 1 : 0;
    uint8_t rf_pwr     = (reg & NRF24_RF_PWR_MASK) >> 1;

    const char *data_rate_str =
        (rf_dr_low == 1 && rf_dr_high == 0) ? "250 kbps" :
        (rf_dr_low == 0 && rf_dr_high == 1) ? "2 Mbps"   :
        (rf_dr_low == 0 && rf_dr_high == 0) ? "1 Mbps"   : "RESERVED (invalid)";

    const char *pwr_str =
        (rf_pwr == 3) ?  "0 dBm"  :
        (rf_pwr == 2) ? "-6 dBm"  :
        (rf_pwr == 1) ? "-12 dBm" : "-18 dBm";

    printf("\r\n[0x06 RF_SETUP] raw=0x%02X\r\n", reg);
    printf("  CONT_WAVE  : %u  (%s)\r\n",
        (unsigned)((reg >> 7) & 1),
        ((reg >> 7) & 1) ? "continuous carrier ON (test mode)" : "normal");
    printf("  RF_DR      : [LOW=%u HIGH=%u]  %s\r\n",
        (unsigned)rf_dr_low, (unsigned)rf_dr_high, data_rate_str);
    printf("  RF_PWR     : %u  (%s)\r\n", (unsigned)rf_pwr, pwr_str);

    /* ── STATUS (0x07) ──────────────────────────────────────────────────── */
    result = nrf24_nop(hspiX, status);
    if (result != HAL_OK) { return result; }

    uint8_t rx_p_no = (*status & NRF24_STATUS_RX_P_NO) >> 1;
    printf("\r\n[0x07 STATUS] raw=0x%02X\r\n", *status);
    printf("  RX_DR      : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_RX_DR)   ? 1 : 0),
        (*status & NRF24_STATUS_RX_DR)   ? "RX data ready"           : "no data");
    printf("  TX_DS      : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_TX_DS)   ? 1 : 0),
        (*status & NRF24_STATUS_TX_DS)   ? "packet sent + ACK rcvd"  : "not sent");
    printf("  MAX_RT     : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_MAX_RT)  ? 1 : 0),
        (*status & NRF24_STATUS_MAX_RT)  ? "!! MAX retransmits — TX BLOCKED !!" : "ok");
    printf("  RX_P_NO    : %u  (%s)\r\n",
        (unsigned)rx_p_no,
        (rx_p_no == 7) ? "RX FIFO empty" :
        (rx_p_no == 6) ? "unused"         : "payload available");
    printf("  TX_FULL    : %u  (%s)\r\n",
        (unsigned)((*status & NRF24_STATUS_TX_FULL) ? 1 : 0),
        (*status & NRF24_STATUS_TX_FULL) ? "TX FIFO full" : "TX FIFO has space");

    /* ── OBSERVE_TX (0x08) ──────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_OBSERVE_TX, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    printf("\r\n[0x08 OBSERVE_TX] raw=0x%02X\r\n", reg);
    printf("  PLOS_CNT   : %2u  (lost packets since last channel change%s)\r\n",
        (unsigned)((reg & NRF24_OBSERVE_TX_PLOS_CNT_MASK) >> 4),
        ((reg >> 4) == 15) ? " — SATURATED at 15" : "");
    printf("  ARC_CNT    : %2u  (retransmits for current packet)\r\n",
        (unsigned)(reg & NRF24_OBSERVE_TX_ARC_CNT_MASK));

    /* ── RPD (0x09) ─────────────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_RPD, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    printf("\r\n[0x09 RPD] raw=0x%02X\r\n", reg);
    printf("  RPD        : %u  (received power %s -64 dBm)\r\n",
        (unsigned)(reg & 0x01),
        (reg & 0x01) ? "ABOVE" : "below");

    /* ── FIFO_STATUS (0x17) ─────────────────────────────────────────────── */
    result = nrf24_read_register(hspiX, NRF24_REG_FIFO_STATUS, status, &reg, 1);
    if (result != HAL_OK) { return result; }

    printf("\r\n[0x17 FIFO_STATUS] raw=0x%02X\r\n", reg);
    printf("  TX_REUSE   : %u  (%s)\r\n",
        (unsigned)((reg & NRF24_FIFO_TX_REUSE) ? 1 : 0),
        (reg & NRF24_FIFO_TX_REUSE) ? "TX payload reuse active"  : "not active");
    printf("  TX_FULL    : %u  (%s)\r\n",
        (unsigned)((reg & NRF24_FIFO_TX_FULL)  ? 1 : 0),
        (reg & NRF24_FIFO_TX_FULL)  ? "TX FIFO full"             : "space available");
    printf("  TX_EMPTY   : %u  (%s)\r\n",
        (unsigned)((reg & NRF24_FIFO_TX_EMPTY) ? 1 : 0),
        (reg & NRF24_FIFO_TX_EMPTY) ? "TX FIFO empty"            : "data in TX FIFO");
    printf("  RX_FULL    : %u  (%s)\r\n",
        (unsigned)((reg & NRF24_FIFO_RX_FULL)  ? 1 : 0),
        (reg & NRF24_FIFO_RX_FULL)  ? "RX FIFO full"             : "space available");
    printf("  RX_EMPTY   : %u  (%s)\r\n",
        (unsigned)((reg & NRF24_FIFO_RX_EMPTY) ? 1 : 0),
        (reg & NRF24_FIFO_RX_EMPTY) ? "RX FIFO empty"            : "data in RX FIFO");

    printf("\r\n========================================\r\n\r\n");

    return HAL_OK;
}