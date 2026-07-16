/**
   @file nrf24_commands.c

   @brief Defines 

   @see 

   Pages: 50-52
*/

#define STM32H7xx // TODO: this is for development. delete this later
#include "nrf24.h"
#include "nrf24_commands.h"

void nrf24_write_csn_high(void) {
	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_SET
	);
}

void nrf24_write_csn_low(void) {
	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void nrf24_start_spi_command(void) {
   HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void nrf24_end_spi_command(void) {
   HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_SET
	);
}


void nrf24_read_register(
   SPI_HandleTypeDef *hspiX,
   const uint8_t registerMapAddress,
   uint8_t *status,
   uint8_t *data,
   const uint8_t dataSize
)
{
   /* Mask address to 5 bits per datasheet: command = 000AAAAA */
   uint8_t commandWord = R_REGISTER | (registerMapAddress & 0x1F);

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * Blocking call — must complete before Phase 2 begins.
   */
   HAL_SPI_TransmitReceive(hspiX, &commandWord, status, 1, HAL_MAX_DELAY);

   /*
   * Phase 2: Receive register data bytes.
   * LSByte first, per datasheet Section 8.3.1.
   */
   HAL_SPI_Receive(hspiX, data, dataSize, HAL_MAX_DELAY);

   nrf24_end_spi_command();    /* CSN high — ends SPI transaction */
}


HAL_StatusTypeDef nrf24_write_register(
   SPI_HandleTypeDef *hspiX,
   const uint8_t     registerMapAddress,
   uint8_t          *status,
   const uint8_t    *data,
   const uint8_t     dataSize
)
{
   HAL_StatusTypeDef result;

   /* Mask address to 5 bits: command encodes as 001AAAAA per datasheet */
   uint8_t commandWord = W_REGISTER | (registerMapAddress & 0x1F);

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte per datasheet.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Write data bytes only if command phase succeeded.
   * LSByte first, per datasheet Section 8.3.1.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Transmit(
         hspiX,
         (uint8_t *)data,    /* cast away const: HAL param is non-const but won't modify */
         dataSize,
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_read_rx_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *payload,
   const uint8_t      payloadSize
)
{
   HAL_StatusTypeDef result;

   /* R_RX_PAYLOAD command = 0b01100001 per datasheet Table 20 */
   uint8_t commandWord = R_RX_PAYLOAD;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Read payload bytes from RX FIFO.
   * LSByte first per datasheet. Only proceed if command phase succeeded.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Receive(
         hspiX,
         payload,
         payloadSize,
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_write_tx_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t     *payload,
   const uint8_t      payloadSize
)
{
   HAL_StatusTypeDef result;

   /* W_TX_PAYLOAD = 0b10100000 per datasheet Table 20 */
   uint8_t commandWord = W_TX_PAYLOAD;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Write payload bytes into TX FIFO.
   * LSByte first per datasheet Section 8.3.1.
   * Only transmit if command phase succeeded.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Transmit(
         hspiX,
         (uint8_t *)payload,     /* cast away const: HAL param is non-const but won't modify */
         payloadSize,
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_flush_tx(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
)
{
   /* FLUSH_TX = 0b11100001 per datasheet Table 20 */
   uint8_t commandWord = FLUSH_TX;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Single-phase transaction: send command byte, receive STATUS register.
   * There is NO data phase for FLUSH_TX — 0 data bytes per datasheet.
   */
   HAL_StatusTypeDef result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_flush_rx(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
)
{
   /* FLUSH_RX = 0b11100010 per datasheet Table 20 */
   uint8_t commandWord = FLUSH_RX;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Single-phase transaction: send command byte, receive STATUS register.
   * There is NO data phase for FLUSH_RX — 0 data bytes per datasheet.
   */
   HAL_StatusTypeDef result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_reuse_tx_pl(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
)
{
   /* REUSE_TX_PL = 0b11100011 per datasheet Table 20 */
   uint8_t commandWord = REUSE_TX_PL;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Single-phase transaction: send command byte, receive STATUS register.
   * There is NO data phase for REUSE_TX_PL — 0 data bytes per datasheet.
   */
   HAL_StatusTypeDef result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_read_rx_pl_wid(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   uint8_t           *payloadWidth
)
{
   HAL_StatusTypeDef result;

   /* R_RX_PL_WID = 0b01100000 per datasheet Table 20 */
   uint8_t commandWord = R_RX_PL_WID;

   /* Pre-zero output — ensures *payloadWidth is never left uninitialised
   * on any error path, including the corruption case */
   *payloadWidth = 0;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Receive the 1-byte payload width value from RX FIFO.
   * Only proceed if command phase succeeded.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Receive(
         hspiX,
         payloadWidth,
         1,          /* exactly 1 byte — the payload width value */
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   /*
   * Datasheet corruption check (Table 20, Section 7.3.4):
   * "If its width is longer than 32 bytes then the packet contains
   *  errors and must be discarded. Discard the packet by using the
   *  Flush_RX command."
   *
   * Surface this as HAL_ERROR so the caller cannot silently proceed
   * to nrf24_read_rx_payload() with a corrupt width value.
   * Zero out payloadWidth to prevent accidental use of the bad value.
   */
   if (result == HAL_OK && *payloadWidth > NRF24_MAX_PAYLOAD_WIDTH) {
      *payloadWidth = 0;
      return HAL_ERROR;
   }

   return result;
}


HAL_StatusTypeDef nrf24_write_ack_payload(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t      pipeNumber,
   const uint8_t     *payload,
   const uint8_t      payloadSize
)
{
   HAL_StatusTypeDef result;

   /*
   * Pipe validation: datasheet Table 20 states PPP valid only 000–101 (0–5).
   * Pipes 6 and 7 are not addressable — reject immediately before touching
   * the SPI bus so CSN is never incorrectly asserted.
   */
   if (pipeNumber > NRF24_ACK_PAYLOAD_PIPE_MAX) {
      return HAL_ERROR;
   }

   /*
   * Command byte: W_ACK_PAYLOAD = 0b10101PPP
   * Pipe number is OR'd into the lower 3 bits.
   * Mask with 0x07 defensively even though validation above already
   * guarantees pipeNumber <= 5 and fits in 3 bits.
   */
   uint8_t commandWord = W_ACK_PAYLOAD | (pipeNumber & 0x07);

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Write ACK payload bytes.
   * LSByte first per datasheet Section 8.3.1.
   * Only transmit if command phase succeeded.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Transmit(
         hspiX,
         (uint8_t *)payload,     /* cast away const: HAL param is non-const but won't modify */
         payloadSize,
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_write_tx_no_ack(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status,
   const uint8_t     *payload,
   const uint8_t      payloadSize
)
{
   HAL_StatusTypeDef result;

   /* W_TX_PAYLOAD_NOACK = 0b10110000 per datasheet Table 20 */
   uint8_t commandWord = W_TX_PAYLOAD_NOACK;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Phase 1: Send command byte, simultaneously receive STATUS register.
   * STATUS is always shifted out on MISO during the command byte.
   */
   result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   /*
   * Phase 2: Write payload bytes into TX FIFO.
   * LSByte first per datasheet Section 8.3.1.
   * Only transmit if command phase succeeded.
   */
   if (result == HAL_OK) {
      result = HAL_SPI_Transmit(
         hspiX,
         (uint8_t *)payload,     /* cast away const: HAL param is non-const but won't modify */
         payloadSize,
         HAL_MAX_DELAY
      );
   }

   nrf24_end_spi_command();    /* CSN high — always end transaction, even on error */

   return result;
}

HAL_StatusTypeDef nrf24_nop(
   SPI_HandleTypeDef *hspiX,
   uint8_t           *status
)
{
   /* NOP = 0b11111111 per datasheet Table 20 */
   uint8_t commandWord = NOP;

   nrf24_start_spi_command();  /* CSN low — begins SPI transaction */

   /*
   * Single-phase transaction: send NOP command byte, receive STATUS register.
   * There is NO data phase — 0 data bytes per datasheet.
   * No state is modified on the chip.
   */
   HAL_StatusTypeDef result = HAL_SPI_TransmitReceive(
      hspiX,
      &commandWord,
      status,
      1,              /* command word is always exactly 1 byte */
      HAL_MAX_DELAY
   );

   nrf24_end_spi_command();    /* CSN high — ends SPI transaction */

   return result;
}