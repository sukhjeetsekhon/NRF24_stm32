/*
 * NRF24_Control.c
 *
 *  Created on: Nov 11, 2026
 *      Author: bjsek
 */

#include "NRF24_Control.h"

#define CMD_WORD_SIZE (uint8_t) 1 // in bytes

void NRF24_selectChip(void) {
    HAL_GPIO_WritePin(RF_CSN_GPIO_Port, RF_CSN_Pin, GPIO_PIN_RESET);
}

void NRF24_deselectChip(void) {
    HAL_GPIO_WritePin(RF_CSN_GPIO_Port, RF_CSN_Pin, GPIO_PIN_SET);
}

void NRF24_sendCommand(uint8_t cmd) {
    NRF24_selectChip();
    HAL_SPI_Transmit(&hspi1, &cmd, CMD_WORD_SIZE, NO_TIMEOUT);
    NRF24_deselectChip();
}

void readRegister(const uint8_t reg, const uint8_t size, uint8_t rxData[]) {
    uint8_t cmd = R_REGISTER | (reg & 0x1F);

    NRF24_selectChip();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, NO_TIMEOUT);
    HAL_SPI_Receive(&hspi1, rxData, size, NO_TIMEOUT);
    NRF24_deselectChip();
}

void writeRegister(const uint8_t reg, const uint8_t size, const uint8_t txData[]) {
    uint8_t cmd = W_REGISTER | (reg & REG_ADDR_MASK);

    NRF24_selectChip();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, NO_TIMEOUT);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)txData, size, NO_TIMEOUT);
    NRF24_deselectChip();
}


uint8_t readStatusRegister(void) {
    const uint8_t cmd = NOP;
    uint8_t status = 0x00;

    NRF24_selectChip();
    HAL_SPI_TransmitReceive(&hspi1, &cmd, &status, 1, NO_TIMEOUT);
    NRF24_deselectChip();

    return status;
}

void writePayload(const uint8_t size, const uint8_t txData[]) {
    const uint8_t cmd = W_TX_PAYLOAD; // 0xA0 (standalone command, not OR'd with W_REGISTER)

    NRF24_selectChip();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, NO_TIMEOUT);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)txData, size, NO_TIMEOUT);
    NRF24_deselectChip();
}

void powerUp(void) {
	uint8_t configValues = 0;

	readRegister(CONFIG, 1, &configValues);

	// preserve config values while writing a 1 in PWR_UP
	uint8_t configPowerUp = configValues | 0x02;

	writeRegister(CONFIG, 1, &configPowerUp);
}

void powerDown(void) {
	uint8_t configValues = 0;

	readRegister(CONFIG, 1, &configValues);

	// preserve config values while writing a 0 in PWR_UP
	uint8_t configPowerDown = configValues & 0xFD;

	writeRegister(CONFIG, 1, &configPowerDown);
}

void setRFChannelFreq(const uint8_t channel) {
	uint8_t rfChannel = channel & 0x7F;

	writeRegister(RF_CH, 1, &rfChannel);
}

void setRFDataRate(const uint8_t dataRate) {
	if (dataRate >= 3 || dataRate < 0) {
		return;
	}

	uint8_t rfSetupValues = 0;
	readRegister(RF_SETUP, 1, &rfSetupValues);

	uint8_t rfConfig = rfSetupValues & 0xD7;

	writeRegister(RF_SETUP, 1, &rfConfig); // set dataRate bits to 00 or MBPS_1

	if (MBPS_2 == dataRate) {
		rfConfig |= 0x08;
		writeRegister(RF_SETUP, 1, &rfConfig);
	} else if (KBPS_250 == dataRate) {
		rfConfig |= 0x20;
		writeRegister(RF_SETUP, 1, &rfConfig);
	}
}

void setTXMode() {
	uint8_t configValues = 0;
	readRegister(CONFIG, 1, &configValues);

	const uint8_t configTXMode = configValues & 0xFE;
	writeRegister(CONFIG, 1, &configTXMode);
}

void setRXMode() {
	uint8_t configValues = 0;
	readRegister(CONFIG, 1, &configValues);

	const uint8_t configRXMode = configValues | 0x01;
	writeRegister(CONFIG, 1, &configRXMode);
}

void setRFPower(uint8_t pwr) {
	if (pwr >= 4 || pwr < 0) {
		return;
	}

	pwr <<= 1; // shift to the left to align to bits 2:1

	uint8_t rfSetupValues = 0;
	readRegister(RF_SETUP, 1, &rfSetupValues);

	rfSetupValues = (rfSetupValues & 0xF9) | pwr;
	writeRegister(RF_SETUP, 1, &rfSetupValues);
}

void setTXAddr(const uint8_t addr[TX_ADDR_SIZE]) {
	writeRegister(TX_ADDR, TX_ADDR_SIZE, addr);
}

void setRXPipeAddr(const uint8_t pipe, const uint8_t size, const uint8_t addr[size]) {
	const uint8_t maxTXAddrSize = 5;
	const uint8_t minTXAddrSize = 1;

	if (size < minTXAddrSize || size > maxTXAddrSize) {
		return;
	}

	switch(pipe) {
		case 0:
			if (size != maxTXAddrSize) {return;}
			writeRegister(RX_ADDR_P0, size, addr);
			break;
		case 1:
			if (size != maxTXAddrSize) {return;}
			writeRegister(RX_ADDR_P1, size, addr);
			break;
		case 2:
			if (size != minTXAddrSize) {return;}
			writeRegister(RX_ADDR_P2, size, addr);
			break;
		case 3:
			if (size != minTXAddrSize) {return;}
			writeRegister(RX_ADDR_P3, size, addr);
			break;
		case 4:
			if (size != minTXAddrSize) {return;}
			writeRegister(RX_ADDR_P4, size, addr);
			break;
		case 5:
			if (size != minTXAddrSize) {return;}
			writeRegister(RX_ADDR_P5, size, addr);
			break;
		default:
			return;
	}
}

void setRXPipe(const uint8_t pipe, uint8_t en) {
	const uint8_t maxPipe = 5;
	const uint8_t minPipe = 0;
	if (pipe > maxPipe || pipe < minPipe) {
		return;
	}

	en <<= pipe; // align value of enable to pipe bit

	uint8_t enableRXAddrMask = 0;

	switch(pipe) {
		case 0:
			enableRXAddrMask = 0x3E | en;
			break;
		case 1:
			enableRXAddrMask = 0x3D | en;
			break;
		case 2:
			enableRXAddrMask = 0x3B | en;
			break;
		case 3:
			enableRXAddrMask = 0x37 | en;
			break;
		case 4:
			enableRXAddrMask = 0x2F | en;
			break;
		case 5:
			enableRXAddrMask = 0x1F | en;
			break;
	}
	writeRegister(EN_RXADDR, 1, &enableRXAddrMask);
}

void clearIRQ() {
	const uint8_t clear_irq  = 0x70;
	writeRegister(STATUS, 1, &clear_irq);
}

void readPayload(const uint8_t size, uint8_t rxData[]) {
    uint8_t cmd = R_RX_PAYLOAD; // 0x61

    NRF24_selectChip();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, NO_TIMEOUT);
    HAL_SPI_Receive(&hspi1, rxData, size, NO_TIMEOUT);
    NRF24_deselectChip();
}
