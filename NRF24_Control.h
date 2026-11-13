/*
 * NRF24_Control.h
 *
 *  Created on: Nov 11, 2026
 *      Author: bjsek
 */

#ifndef INC_NRF24_CONTROL_H_
#define INC_NRF24_CONTROL_H_

#include "NRF24_Config.h"
#include "NRF24_SPI_commands.h"
#include "NRF24_Register_Address.h"

#define NO_TIMEOUT 	(uint32_t) 100

// air data rates
#define MBPS_1 		(uint8_t) 0
#define MBPS_2 		(uint8_t) 1
#define KBPS_250 	(uint8_t) 2

// long-range data rate
#define LORA_DR KBPS_250

// transmit power
#define DBM_18 	(uint8_t) 0
#define DBM_12 	(uint8_t) 1
#define DBM_6 	(uint8_t) 2
#define DBM_0 	(uint8_t) 3
#define MAX_TX_PWR DBM_0

// Channel frequency
#define MAX_CH_FREQ (uint8_t) 0x7D

#define TX_ADDR_SIZE 5 // in bytes

/**
 * @brief activate the chip select pin
 */
void NRF24_selectChip();

/*
 * @brief deactivate the chip select pin
 */
void NRF24_deselectChip();


uint8_t readStatusRegister();

/**
  *
  *	@param size size of register in bytes
  */
void readRegister(const uint8_t reg, const uint8_t size, uint8_t rxData[]);

/**
 * @brief write data to a register
 * @param reg register to write to
 * @param size size of data to write
 * @param txData data to write into register (up to 5 bytes)
 */
void writeRegister(const uint8_t reg, const uint8_t size, const uint8_t txData[]);

/**
 * @brief send a command word
 * @param cmd 1 byte command word
 * @note cmd should not require any additional register address or data bits
 */
void NRF24_sendCommand(uint8_t cmd);

/**
 * @brief read STATUS register by sending a NOP command
 */
uint8_t readStatusRegister(void);

/**
 * @brief write to TX payload for transmission
 * @param size size of payload in bytes
 * @param txData data to transmit
 */
void writePayload(const uint8_t size, const uint8_t txData[]);

/**
 * @brief power up radio while preserving CONFIG values
 */
void powerUp(void);

/**
 * @brief power down radio while preserving CONFIG values
 */
void powerDown(void);

void setRFChannelFreq(const uint8_t channel);

void setRFDataRate(const uint8_t dataRate);

void setTXMode();

void setRXMode();

void setRFPower(const uint8_t pwr);

/**
 * @brief sets the address of the transmitter radio
 * @brief addr the 5 byte address of the transmitter
 *
 * @note LSByte is sent first
 */
void setTXAddr(const uint8_t addr[TX_ADDR_SIZE]);

/**
 * @brief set the receiver pipe address
 * @param pipe pipe number 0 to 5
 * @param size size of the address in bytes
 * @param addr the address to set the receiver pipe's address
 * @note pipe 0-1 are 5 bytes wide. pipe 2-5 are 1 byte wide
 */
void setRXPipeAddr(const uint8_t pipe, const uint8_t size, const uint8_t addr[size]);

/**
 * @brief enable or disable receiver pipe address
 * @param pipe pipe number 0 to 5
 * @param en 1 for enable pipe or 0 for disable pipe
 */
void setRXPipe(const uint8_t pipe, uint8_t en);

/**
 * @brief clear IRQ flags
 */
void clearIRQ();

void readPayload(const uint8_t size, uint8_t rxData[]);
#endif /* INC_NRF24_CONTROL_H_ */
