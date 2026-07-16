/**
   @file nrf24_pins.h

   @brief Pin function definitions for the nRF24L01+ 20-pin QFN package.

   @see docs\nRF24L01P_Product_Specification_1_0.pdf
        Section 2.1 — Pin assignment    (p. 10)
        Section 2.2 — Pin functions     (p. 11)

   Pages: 10-11

*/

#ifndef NRF24_PINS_H
#define NRF24_PINS_H

/*
 * ─────────────────────────────────────────────────────────────────────────────
 * nRF24L01+ Pin Function Reference
 * Datasheet Section 2.2, Table 2 — Pin functions (p. 11)
 *
 * The nRF24L01+ is a 20-pin 4x4mm QFN package. Of the 20 pins, 8 are
 * relevant to MCU interfacing. The remaining pins are RF, power, and
 * crystal connections described below for completeness.
 *
 * ┌────────┬────────┬──────────────┬─────────────────────────────────────────┐
 * │ Pin #  │  Name  │  Direction   │  Description                            │
 * ├────────┼────────┼──────────────┼─────────────────────────────────────────┤
 * │  1     │  CE    │  Input       │  Chip Enable — activates RX or TX mode  │
 * │  2     │  CSN   │  Input       │  SPI Chip Select (active LOW)           │
 * │  3     │  SCK   │  Input       │  SPI Clock                              │
 * │  4     │  MOSI  │  Input       │  SPI Master Out Slave In                │
 * │  5     │  MISO  │  Output      │  SPI Master In Slave Out (tri-stated    │
 * │        │        │              │  when CSN is HIGH)                      │
 * │  6     │  IRQ   │  Output      │  Interrupt — active LOW                 │
 * │  7     │  VDD   │  Power       │  Power supply: +1.9V to +3.6V DC        │
 * │  8     │  VSS   │  Power       │  Ground (0V)                            │
 * │ 9–20   │  (RF)  │  RF / Power  │  XC1, XC2, VSS, VDD, ANT1, ANT2,       │
 * │        │        │              │  DVDD, IREF, VDD_PA — see Section 2.1   │
 * └────────┴────────┴──────────────┴─────────────────────────────────────────┘
 *
 * NOTE: Pin numbers above use the module breakout ordering (J1 connector).
 * QFN pad numbering differs — refer to Section 2.1 Figure 2 for the
 * exact QFN die pad assignment.
 * ─────────────────────────────────────────────────────────────────────────────
 */

/*
 * ── CE — Chip Enable ─────────────────────────────────────────────────────────
 * Direction: Digital Input
 * Datasheet Section 2.2:
 *   "Chip Enable Activates RX or TX mode."
 *
 * Behavior per operating mode (datasheet Table 15 — Section 6.1.6):
 *   CE = 0 → Standby-I (when PWR_UP=1)
 *   CE = 1, PRIM_RX=1 → RX mode (PRX)
 *   CE = 1, PRIM_RX=0, TX FIFO not empty → TX mode (PTX)
 *   CE = 1, PRIM_RX=0, TX FIFO empty     → Standby-II
 *
 * Timing constraints (datasheet Table 16 — Section 6.1.7):
 *   Thce  >= 10us  — minimum CE HIGH pulse width to initiate TX
 *   Tstby2a = 130us — standby to active mode settling time
 *
 * MUST be driven by the MCU GPIO — not hardwired.
 */
#define NRF24_PIN_CE_DESCRIPTION        "Chip Enable — activates RX or TX mode"

/*
 * ── CSN — Chip Select Not ────────────────────────────────────────────────────
 * Direction: Digital Input
 * Datasheet Section 2.2:
 *   "SPI Chip Select. Active low."
 *
 * CSN must be held LOW for the entire duration of any SPI transaction.
 * CSN HIGH deasserts the SPI bus and tri-states MISO.
 * Per datasheet Section 8.3.2: CSN must be stable for Tcss >= 2ns before
 * the first SCK rising edge.
 *
 * In this driver, CSN is controlled via:
 *   nrf24_start_spi_command()  → CSN LOW  (begin transaction)
 *   nrf24_end_spi_command()    → CSN HIGH (end transaction)
 */
#define NRF24_PIN_CSN_DESCRIPTION       "SPI Chip Select — active LOW"

/*
 * ── SCK — SPI Clock ──────────────────────────────────────────────────────────
 * Direction: Digital Input
 * Datasheet Section 2.2:
 *   "SPI Clock."
 *
 * SPI mode: CPOL=0, CPHA=0 (Mode 0) per datasheet Section 8.3.2 —
 * data is captured on the rising edge of SCK, changed on the falling edge.
 * Maximum SCK frequency: 8 MHz (datasheet Table 11 — SPI timing parameters).
 *
 * Driven by the MCU SPI peripheral — not configured in this driver directly.
 */
#define NRF24_PIN_SCK_DESCRIPTION       "SPI Clock — max 8 MHz, CPOL=0 CPHA=0"

/*
 * ── MOSI — Master Out Slave In ───────────────────────────────────────────────
 * Direction: Digital Input
 * Datasheet Section 2.2:
 *   "SPI slave data input."
 *
 * Data from MCU to nRF24L01+. MSBit of each byte is transmitted first
 * per datasheet Section 8.3.1.
 */
#define NRF24_PIN_MOSI_DESCRIPTION      "SPI data input — MSBit first"

/*
 * ── MISO — Master In Slave Out ───────────────────────────────────────────────
 * Direction: Digital Output (tri-stated when CSN is HIGH)
 * Datasheet Section 2.2:
 *   "SPI slave data output. Tri-state defined by CSN."
 *
 * Data from nRF24L01+ to MCU. The STATUS register is always shifted out
 * on MISO during the first byte (command byte) of every SPI transaction,
 * regardless of the command type — per datasheet Section 8.3.1.
 *
 * MISO is tri-stated (high-impedance) whenever CSN is HIGH. Do not rely
 * on a defined logic level from MISO between SPI transactions.
 */
#define NRF24_PIN_MISO_DESCRIPTION      "SPI data output — tri-stated when CSN HIGH"

/*
 * ── IRQ — Interrupt Request ──────────────────────────────────────────────────
 * Direction: Digital Output
 * Datasheet Section 2.2:
 *   "Maskable interrupt pin. Active low."
 *
 * IRQ is asserted LOW when any unmasked interrupt source fires.
 * Three interrupt sources (datasheet Section 8.5):
 *
 *   RX_DR  (STATUS bit 6) — RX data ready: new payload in RX FIFO
 *   TX_DS  (STATUS bit 5) — TX data sent: packet transmitted + ACK received
 *   MAX_RT (STATUS bit 4) — Max retransmits reached: TX failed
 *
 * Each source can be independently masked via CONFIG register bits:
 *   MASK_RX_DR  (CONFIG bit 6): 1 = IRQ not asserted on RX_DR
 *   MASK_TX_DS  (CONFIG bit 5): 1 = IRQ not asserted on TX_DS
 *   MASK_MAX_RT (CONFIG bit 4): 1 = IRQ not asserted on MAX_RT
 *
 * IRQ is cleared by writing 1 to the corresponding STATUS bit via SPI.
 * Polling STATUS via nrf24_nop() is equivalent when IRQ pin is not wired.
 *
 * !! IMPORTANT !!
 *   MAX_RT MUST be cleared (write 1 to STATUS bit 4) before any further
 *   TX operation — the TX FIFO is blocked until MAX_RT is cleared.
 */
#define NRF24_PIN_IRQ_DESCRIPTION       "Interrupt — active LOW, maskable per CONFIG"

/*
 * ── VDD — Power Supply ───────────────────────────────────────────────────────
 * Direction: Power Input
 * Datasheet Section 2.2 / Section 4 (Operating conditions):
 *   Supply voltage: 1.9V to 3.6V DC
 *
 * !! WARNING: 3.3V supply only — do NOT connect to 5V !!
 * Logic pins (CE, CSN, SCK, MOSI) are 5V tolerant per datasheet key
 * features, but VDD must not exceed 3.6V absolute maximum.
 * Recommended: 100nF decoupling capacitor as close to VDD pin as possible
 * per datasheet Section 10.4 (PCB layout and decoupling guidelines).
 */
#define NRF24_PIN_VDD_DESCRIPTION       "Power supply — 1.9V to 3.6V DC"

/*
 * ── VSS — Ground ─────────────────────────────────────────────────────────────
 * Direction: Power Reference
 * Datasheet Section 2.2:
 *   "Ground (0V)."
 */
#define NRF24_PIN_VSS_DESCRIPTION       "Ground — 0V reference"

/*
 * ─────────────────────────────────────────────────────────────────────────────
 * MCU GPIO Configuration Summary
 *
 * The following GPIO modes are required on the STM32 side:
 *
 *  Pin   │ STM32 GPIO Mode         │ Notes
 * ───────┼─────────────────────────┼────────────────────────────────────────
 *  CE    │ Output Push-Pull        │ Default LOW; toggled by driver
 *  CSN   │ Output Push-Pull        │ Default HIGH (deasserted); pulled LOW
 *        │                         │ only during SPI transactions
 *  SCK   │ Alternate Function      │ Managed by STM32 SPI peripheral
 *  MOSI  │ Alternate Function      │ Managed by STM32 SPI peripheral
 *  MISO  │ Alternate Function      │ Managed by STM32 SPI peripheral
 *  IRQ   │ Input + EXTI (optional) │ Active LOW; connect to EXTI line if
 *        │                         │ using interrupt-driven receive;
 *        │                         │ may be left unconnected if polling
 *        │                         │ STATUS via nrf24_nop() instead
 *
 * CE and CSN GPIO port/pin are configured in nrf24_config.h:
 *   CE_GPIO_PORT  / CE_GPIO_PIN
 *   CSN_GPIO_PORT / CSN_GPIO_PIN
 * ─────────────────────────────────────────────────────────────────────────────
 */

#endif /* NRF24_PINS_H */