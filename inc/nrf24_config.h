/**

   @file nrf24_config.h

   @brief

   @see docs\nRF24L01P_Product_Specification_1_0.pdf

   Pages: 11

*/

#ifndef NRF24_CONFIG_H
#define NRF24_CONFIG_H

/* ── Register addresses ─────────────────────────────────────────────────── */
#define NRF24_REG_CONFIG        0x00U   /* Configuration register           */
#define NRF24_REG_RF_CH         0x05U   /* RF channel frequency             */
#define NRF24_REG_RF_SETUP      0x06U   /* RF air data rate + output power  */
#define NRF24_REG_RPD           0x09U   /* Received Power Detector          */

/* ── CONFIG register bits ──────────────────────────────────────────────── */
#define NRF24_CONFIG_PRIM_RX    (1U << 0)   /* 1 = PRX, 0 = PTX            */

/* ── RF_SETUP register bits (datasheet Section 9.1, register 0x06) ─────── */
#define NRF24_RF_SETUP_RF_DR_LOW    (1U << 5)   /* 250kbps select          */
#define NRF24_RF_SETUP_RF_DR_HIGH   (1U << 3)   /* 2Mbps select            */
#define NRF24_RF_SETUP_RF_DR_MASK   (NRF24_RF_SETUP_RF_DR_LOW | NRF24_RF_SETUP_RF_DR_HIGH)
#define NRF24_RF_SETUP_RF_PWR_MASK  (0x03U << 1) /* RF_PWR bits [2:1]      */

/* ── RF_CH register constraint ─────────────────────────────────────────── */
/*
 * Valid channels: 0–125 (0x00–0x7D).
 * F0 = 2400 + RF_CH MHz — channel 125 = 2525 MHz, within the 2.4835GHz
 * ISM band upper limit of 2483.5 MHz... however the datasheet recommends
 * channels 0–83 for 2Mbps to avoid the 2.483GHz WLAN guard band.
 * Absolute hardware maximum the 7-bit field can hold is 127 (0x7F).
 */
#define NRF24_RF_CH_MAX         0x7FU

/* ── RPD register bit ───────────────────────────────────────────────────── */
#define NRF24_RPD_BIT           (1U << 0)   /* 1 = received power > -64dBm */


/* ── Air data rate enum — Section 6.2 ─────────────────────────────────── */
typedef enum {
   NRF24_DATA_RATE_1MBPS   = 0,    /* RF_DR_LOW=0, RF_DR_HIGH=0          */
   NRF24_DATA_RATE_2MBPS   = 1,    /* RF_DR_LOW=0, RF_DR_HIGH=1          */
   NRF24_DATA_RATE_250KBPS = 2,    /* RF_DR_LOW=1, RF_DR_HIGH=0          */
} nrf24_data_rate_t;

/* ── Output power enum — Section 6.5 ──────────────────────────────────── */
/*
 * Values are pre-shifted to sit in RF_SETUP bits [2:1], matching the
 * datasheet RF_PWR[1:0] encoding exactly — so they can be OR'd directly
 * into the RF_SETUP byte after masking, with no extra shift required.
 */
typedef enum {
   NRF24_OUTPUT_PWR_NEG18DBM = (0x00U << 1),  /* -18 dBm — minimum power */
   NRF24_OUTPUT_PWR_NEG12DBM = (0x01U << 1),  /* -12 dBm                 */
   NRF24_OUTPUT_PWR_NEG6DBM  = (0x02U << 1),  /*  -6 dBm                 */
   NRF24_OUTPUT_PWR_0DBM     = (0x03U << 1),  /*   0 dBm — maximum power */
} nrf24_output_pwr_t;

/* ── RX/TX role enum — Section 6.6 ────────────────────────────────────── */
typedef enum {
   NRF24_ROLE_PTX = 0,     /* Primary Transmitter — PRIM_RX = 0          */
   NRF24_ROLE_PRX = 1,     /* Primary Receiver    — PRIM_RX = 1          */
} nrf24_role_t;


/**
 * @brief  Set the on-air data rate via the RF_DR_LOW and RF_DR_HIGH bits
 *         in the RF_SETUP register (0x06).
 *
 * Per datasheet Section 6.2, the data rate is encoded across two bits:
 *
 *   RF_DR_LOW (bit 5) | RF_DR_HIGH (bit 3) | Rate
 *   ──────────────────┼────────────────────┼──────────
 *         0           |         0          | 1 Mbps
 *         0           |         1          | 2 Mbps
 *         1           |         0          | 250 kbps
 *         1           |         1          | Reserved — never write this
 *
 * Read-modify-write is used to preserve CONT_WAVE, PLL_LOCK, and RF_PWR.
 *
 * @param  hspiX    SPI handle
 * @param  status   Output: STATUS register byte received during command
 * @param  dataRate One of: NRF24_DATA_RATE_250KBPS
 *                          NRF24_DATA_RATE_1MBPS
 *                          NRF24_DATA_RATE_2MBPS
 * @return HAL_OK      on success
 *         HAL_ERROR   if dataRate is invalid or SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_set_air_data_rate(
   SPI_HandleTypeDef   *hspiX,
   uint8_t             *status,
   nrf24_data_rate_t    dataRate
);

/**
 * @brief  Set the RF channel frequency via the RF_CH register (0x05).
 *
 * Per datasheet Section 6.3:
 *   "F0 = 2400 + RF_CH [MHz]"
 *
 * RF_CH is a 7-bit field — bits [6:0] of register 0x05.
 * Valid hardware range: 0x00–0x7F (0–127).
 *
 * !! PRACTICAL CONSTRAINT !!
 *   The ISM band upper limit is 2483.5MHz (channel 83 at 2Mbps).
 *   At 1Mbps/250kbps, channels up to 125 are usable.
 *   The caller is responsible for selecting a channel appropriate
 *   for the configured data rate and local regulatory requirements.
 *
 * @param  hspiX    SPI handle
 * @param  status   Output: STATUS register byte received during command
 * @param  channel  RF channel number (0x00–0x7F; caller validates range)
 * @return HAL_OK      on success
 *         HAL_ERROR   if channel > 0x7F or SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_set_rf_channel_freq(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t            channel
);

/**
 * @brief  Read the Received Power Detector (RPD) register (0x09).
 *
 * Per datasheet Section 6.4:
 *   "This bit is set high when the received power exceeds -64dBm."
 *   "The RPD register must be read at least 170us after entering RX mode
 *    to get an accurate reading."
 *
 * RPD register: bit 0 only. All other bits are reserved.
 *   0 = received power <= -64dBm (channel clear or no carrier present)
 *   1 = received power >  -64dBm (carrier detected)
 *
 * Typical use: channel scanning — sweep RF_CH, read RPD to map busy
 * channels before selecting a frequency for communication.
 *
 * !! CALLER MUST ensure the device has been in RX mode for >= 170us !!
 *
 * @param  hspiX        SPI handle
 * @param  status       Output: STATUS register byte received during command
 * @param  rpd          Output: 1 if power > -64dBm, 0 if not
 * @return HAL_OK      on success
 *         HAL_ERROR   on SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_read_rpd(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *rpd
);

/**
 * @brief  Set the PA (Power Amplifier) output power via RF_PWR bits [2:1]
 *         in the RF_SETUP register (0x06).
 *
 * Per datasheet Section 6.5:
 *
 *   RF_PWR[1:0] | Output Power
 *   ────────────┼──────────────
 *     11 (0x3)  |  0 dBm   (maximum — highest current draw)
 *     10 (0x2)  | -6 dBm
 *     01 (0x1)  | -12 dBm
 *     00 (0x0)  | -18 dBm  (minimum — lowest current draw)
 *
 * Read-modify-write is used to preserve CONT_WAVE, PLL_LOCK, RF_DR_*.
 *
 * @param  hspiX      SPI handle
 * @param  status     Output: STATUS register byte received during command
 * @param  outputPwr  One of: NRF24_OUTPUT_PWR_0DBM
 *                            NRF24_OUTPUT_PWR_NEG6DBM
 *                            NRF24_OUTPUT_PWR_NEG12DBM
 *                            NRF24_OUTPUT_PWR_NEG18DBM
 * @return HAL_OK      on success
 *         HAL_ERROR   if outputPwr is invalid or SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_set_output_pwr(
   SPI_HandleTypeDef   *hspiX,
   uint8_t             *status,
   nrf24_output_pwr_t   outputPwr
);

/**
 * @brief  Set the device role to PRX or PTX via the PRIM_RX bit (bit 0)
 *         in the CONFIG register (0x00).
 *
 * Per datasheet Section 6.6:
 *   "Setting PRIM_RX high puts the nRF24L01+ in PRX role."
 *   "Setting PRIM_RX low puts the nRF24L01+ in PTX role."
 *
 * Read-modify-write is mandatory — preserves all other CONFIG bits
 * (MASK_RX_DR, MASK_TX_DS, MASK_MAX_RT, EN_CRC, CRCO, PWR_UP).
 *
 * !! NOTE !!
 *   Writing PRIM_RX alone does NOT enter RX or TX mode.
 *   Entering active RX mode also requires CE=HIGH.
 *   Use nrf24_enter_rx_mode() / nrf24_enter_tx_mode() from nrf24_modes.h
 *   for full mode transitions, which handle CE and the Tstby2a delay.
 *   This function is appropriate when fine-grained control of PRIM_RX
 *   is needed independently from CE (e.g., during init sequencing).
 *
 * @param  hspiX   SPI handle
 * @param  status  Output: STATUS register byte received during command
 * @param  role    NRF24_ROLE_PRX — set PRIM_RX=1 (primary receiver)
 *                 NRF24_ROLE_PTX — set PRIM_RX=0 (primary transmitter)
 * @return HAL_OK      on success
 *         HAL_ERROR   if role is invalid or SPI error
 *         HAL_TIMEOUT if SPI times out
 */
HAL_StatusTypeDef nrf24_rxtx_control(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   nrf24_role_t       role
);

#endif /* NRF24_CONFIG_H */
