/*
 * NRF24_SPI_commands.h
 *
 *  Created on: Nov 11, 2026
 *      Author: bjsek
 */

#ifndef INC_NRF24_SPI_COMMANDS_H_
#define INC_NRF24_SPI_COMMANDS_H_

#define R_REGISTER 			(uint8_t) 0x00
#define W_REGISTER 			(uint8_t) 0x20
#define R_RX_PAYLOAD 		(uint8_t) 0x61
#define W_TX_PAYLOAD 		(uint8_t) 0xA0
#define FLUSH_TX 			(uint8_t) 0xE1
#define FLUSH_RX 			(uint8_t) 0xE2
#define REUSE_TX_PL 		(uint8_t) 0xE3
#define R_RX_PL_WID 		(uint8_t) 0x60
#define W_ACK_PAYLOAD 		(uint8_t) 0xA8
#define W_TX_PAYLOAD_NOACK  (uint8_t) 0xB0
#define NOP 				(uint8_t) 0xFF



#endif /* INC_NRF24_SPI_COMMANDS_H_ */
