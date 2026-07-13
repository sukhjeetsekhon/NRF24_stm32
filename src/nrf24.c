/**

   @file nrf24.c

   @brief define functions to operate the NRF24L01 radio module

   @see docs\nRF24L01P_Product_Specification_1_0.pdf

   Pages: 21-27

*/
#define STM32H7xx // TODO: this is for development. delete this later
#include "nrf24.h"

extern SPI_HandleTypeDef hspiX;
extern TIM_HandleTypeDef htimX;


void writeCSNHIGH(void){
	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_SET
	);
}

void writeCSNLOW(void){
	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void writeCEHIGH(void){
	HAL_GPIO_WritePin(
		CE_GPIO_PORT,
		CE_GPIO_PIN,
		GPIO_PIN_SET
	);
}

void writeCELOW(void){
	HAL_GPIO_WritePin(
		CE_GPIO_PORT,
		CE_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void nrf24WriteReg(uint8_t reg, uint8_t *data, uint8_t size){

	uint8_t cmd = W_REGISTER | reg;

	writeCSNLOW();

	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);

	writeCSNHIGH();
}

uint8_t nrf24ReadReg(uint8_t reg, uint8_t size){
	uint8_t cmd = R_REGISTER | reg;
	uint8_t data = 0;

	writeCSNLOW();

	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Receive(&hspiX, &data, size, spi_r_timeout);

	writeCSNHIGH();

	return data;
}

void nrf24WriteSpecialCommand(uint8_t cmd){
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
}

void nrf24WriteSpecialReg(uint8_t *data, uint8_t size){
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
}

void nrf24ReadSpecialReg(uint8_t *data, uint8_t size){
	HAL_SPI_Receive(&hspiX, data, size, spi_r_timeout);
}

void nrf24PowerUp(void){
	uint8_t data = 0;

	data = nrf24ReadReg(CONFIG, 1);

	data |= (1 << PWR_UP);

	nrf24WriteReg(CONFIG, &data, 1);
}

void nrf24PowerDown(void){
	uint8_t data = 0;

	data = nrf24ReadReg(CONFIG, 1);

	data &= ~(1 << PWR_UP);

	nrf24WriteReg(CONFIG, &data, 1);
}

void nrf24SetTXPower(uint8_t pwr){
	uint8_t data = 0;

	data = nrf24ReadReg(RF_SETUP, 1);

	data &= 184;

	data |= (pwr << RF_PWR);

	nrf24WriteReg(RF_SETUP, &data, 1);
}

void nrf24SetDataRate(uint8_t bps){
	uint8_t data = 0;

	data = nrf24ReadReg(RF_SETUP, 1);

	data &= ~(1 << RF_DR_LOW) & ~(1 << RF_DR_HIGH);

	if(bps == _2mbps){
		data |= (1 << RF_DR_HIGH);
	}else if(bps == _250kbps){
		data |= (1 << RF_DR_LOW);
	}

	nrf24WriteReg(RF_SETUP, &data, 1);
}

void nrf24SetChannel(uint8_t ch){
	nrf24WriteReg(RF_CH, &ch, 1);
}

void nrf24OpenTXPipe(uint8_t *addr){
	nrf24WriteReg(TX_ADDR, addr, 5);
}

void nrf24SetPipePayloadSize(uint8_t pipe, uint8_t size){
	if(size > 32){
		size = 32;
	}

	switch(pipe){
	case 0:
		nrf24WriteReg(RX_PW_P0, &size, 1);

		break;
	case 1:
		nrf24WriteReg(RX_PW_P1, &size, 1);

		break;
	case 2:
		nrf24WriteReg(RX_PW_P2, &size, 1);

		break;
	case 3:
		nrf24WriteReg(RX_PW_P3, &size, 1);

		break;
	case 4:
		nrf24WriteReg(RX_PW_P4, &size, 1);

		break;
	case 5:
		nrf24WriteReg(RX_PW_P5, &size, 1);

		break;
	}
}

void nrf24OpenRXPipe(uint8_t pipe, uint8_t *addr){

	uint8_t data = 0;

	data = nrf24ReadReg(EN_RXADDR, 1);

	switch(pipe){
	case 0:
		nrf24WriteReg(RX_ADDR_P0, addr, 5);

		data |= (1 << ERX_P0);
		break;
	case 1:
		nrf24WriteReg(RX_ADDR_P1, addr, 5);

		data |= (1 << ERX_P1);
		break;
	case 2:
		nrf24WriteReg(RX_ADDR_P2, addr, 1);

		data |= (1 << ERX_P2);
		break;
	case 3:
		nrf24WriteReg(RX_ADDR_P3, addr, 1);

		data |= (1 << ERX_P3);
		break;
	case 4:
		nrf24WriteReg(RX_ADDR_P4, addr, 1);

		data |= (1 << ERX_P4);
		break;
	case 5:
		nrf24WriteReg(RX_ADDR_P5, addr, 1);

		data |= (1 << ERX_P5);
		break;
	}

	nrf24WriteReg(EN_RXADDR, &data, 1);
}

void nrf24CloseRXPipe(uint8_t pipe){
	uint8_t data = nrf24ReadReg(EN_RXADDR, 1);

	data &= ~(1 << pipe);

	nrf24WriteReg(EN_RXADDR, &data, 1);
}

void nrf24SetCRC(uint8_t en_crc, uint8_t crc0){
	uint8_t data = nrf24ReadReg(CONFIG, 1);

	data &= ~(1 << EN_CRC) & ~(1 << CRCO);

	data |= (en_crc << EN_CRC) | (crc0 << CRCO);

	nrf24WriteReg(CONFIG, &data, 1);
}

void nrf24SetAddressWidth(uint8_t bytes){
	bytes -= 2;
	nrf24WriteReg(SETUP_AW, &bytes, 1);
}

void nrf24FlushTX(void){
	writeCSNLOW();
	nrf24WriteSpecialCommand(FLUSH_TX);
	writeCSNHIGH();
}

void nrf24FlushRX(void){
	writeCSNLOW();
	nrf24WriteSpecialCommand(FLUSH_RX);
	writeCSNHIGH();
}

uint8_t nrf24ReadStatusReg(void){
	uint8_t data = 0;
	uint8_t cmd = NOP;

	writeCSNLOW();
	HAL_SPI_TransmitReceive(&hspiX, &cmd, &data, 1, spi_rw_timeout);
	writeCSNHIGH();

	return data;
}

void nrf24ClearRX_DR(void){
	uint8_t data = 0;

	data = nrf24ReadStatusReg();

	data |= (1 << RX_DR);

	nrf24WriteReg(STATUS, &data, 1);
}

void nrf24ClearTX_DS(void){
	uint8_t data = 0;

	data = nrf24ReadStatusReg();

	data |= (1 << TX_DS);

    nrf24WriteReg(STATUS, &data, 1);
}

void nrf24ClearMAX_RT(void){
	uint8_t data = 0;

	data = nrf24ReadStatusReg();

	data |= (1 << MAX_RT);

    nrf24WriteReg(STATUS, &data, 1);
}

uint8_t nrf24ReadBit(uint8_t reg, uint8_t bit){

	if(nrf24ReadReg(reg, 1) & (1 << bit)){
		return 1;
	}

	return 0;
}

void nrf24SetBit(uint8_t reg, uint8_t bit, uint8_t val){
	uint8_t data = 0;

	data = nrf24ReadReg(reg, 1);

	if(val){
		data |= (1 << bit);
	}else{
		data &= ~(1 << bit);
	}

    nrf24WriteReg(reg, &data, 1);
}

uint8_t nrf24ReadPayloadWidth(void){
	uint8_t width = 0;

	writeCSNLOW();
	nrf24WriteSpecialCommand(R_RX_PL_WID);
	nrf24ReadSpecialReg(&width, 1);
	writeCSNHIGH();

	return width;
}

void nrf24SwitchToRX(void){
	uint8_t data = 0;

	data = nrf24ReadReg(CONFIG, 1);

	data |= (1 << PRIM_RX);

	nrf24WriteReg(CONFIG, &data, 1);

	writeCEHIGH();
}

void nrf24SwitchToTX(void){
	uint8_t data = 0;

	data = nrf24ReadReg(CONFIG, 1);

	data &= ~(1 << PRIM_RX);

	nrf24WriteReg(CONFIG, &data, 1);
}

void nrf24EnableDynamicPayload(uint8_t en){
	uint8_t feature = nrf24ReadReg(FEATURE, 1);

	if(en == enable){
		feature |= (1 << EN_DPL);
	}else{
		feature &= ~(1 << EN_DPL);
	}

	nrf24WriteReg(FEATURE, &feature, 1);
}

void nrf24SetRXDynamicPayloadPipe(uint8_t pipe, uint8_t en){

	uint8_t dynpd = nrf24ReadReg(DYNPD, 1);

	if(pipe > 5){
		pipe = 5;
	}

	if(en){
		dynpd |= (1 << pipe);
	}else{
		dynpd &= ~(1 << pipe);
	}

	nrf24WriteReg(DYNPD, &dynpd, 1);
}

void nrf24EnableAutoAck(uint8_t pipe, uint8_t ack){

	if(pipe > 5){
		pipe = 5;
	}

	uint8_t enaa = nrf24ReadReg(EN_AA, 1);

	if(ack){
		enaa |= (1 << pipe);
	}else{
		enaa &= ~(1 << pipe);
	}

	nrf24WriteReg(EN_AA, &enaa, 1);
}

void nrf24EnableAutoAckOnAllPipes(uint8_t ack){
	uint8_t enaa = nrf24ReadReg(EN_AA, 1);

	if(ack){
		enaa = 63;
	}else{
		enaa = 0;
	}

	nrf24WriteReg(EN_AA, &enaa, 1);
}

void nrf24EnableAckPayload(uint8_t en){
	uint8_t feature = nrf24ReadReg(FEATURE, 1);

	if(en){
		feature |= (1 << EN_ACK_PAY);
	}else{
		feature &= ~(1 << EN_ACK_PAY);
	}

	nrf24WriteReg(FEATURE, &feature, 1);
}

void nrf24EnableTransmitNoAck(uint8_t en){
	uint8_t feature = nrf24ReadReg(FEATURE, 1);

	if(en){
		feature |= (1 << EN_DYN_ACK);
	}else{
		feature &= ~(1 << EN_DYN_ACK);
	}

	nrf24WriteReg(FEATURE, &feature, 1);
}

void nrf24SetAutoRetransmissionDelay(uint8_t delay){
	uint8_t data = nrf24ReadReg(SETUP_RETR, 1);

	data &= 15;

	data |= (delay << ARD);

	nrf24WriteReg(SETUP_RETR, &data, 1);
}

void nrf24SetAutoRetransmissionLimit(uint8_t limit){
	uint8_t data = nrf24ReadReg(SETUP_RETR, 1);

	data &= 240;

	data |= (limit << ARC);

	nrf24WriteReg(SETUP_RETR, &data, 1);
}

void nrf24ConvertTypeToUint8Array(size_t in, uint8_t* out, uint16_t size){
	for(uint16_t i = 0; i < size; i++){
		out[i] = (((in & (255 << (i*8)))) >> (i*8));
	}
}

size_t nrf24ConvertUint8ArrayToType(uint8_t* in, uint16_t size){
	size_t out = 0;

	for(uint16_t i = 0; i < size; i++){
		out |= (in[i] << (8*i));
	}

	return out;
}


uint8_t nrf24TransmitData(uint8_t *data, uint8_t size){

	writeCELOW();

	uint8_t cmd = W_TX_PAYLOAD;

	writeCSNLOW();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	writeCSNHIGH();

	writeCEHIGH();
	delayMicroseconds(20);
	writeCELOW();

	if(nrf24ReadStatusReg() & (1 << MAX_RT)){
		nrf24ClearMAX_RT();
		nrf24FlushTX();
		return 1;
	}

	return 0;
}

uint8_t nrf24TransmitDataNoAck(uint8_t *data, uint8_t size){

	writeCELOW();

	uint8_t cmd = W_TX_PAYLOAD_NOACK;

	writeCSNLOW();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	writeCSNHIGH();

	writeCEHIGH();
	delayMicroseconds(20);
	writeCELOW();

	if(nrf24ReadStatusReg() & (1 << MAX_RT)){
		nrf24ClearMAX_RT();
		nrf24FlushTX();
		return 1;
	}

	return 0;
}

void nrf24TransmitRXPayloadAsRX(uint8_t pipe, uint8_t *data, uint8_t size){

	if(pipe > 5){
		pipe = 5;
	}

	uint8_t cmd = (W_ACK_PAYLOAD | pipe);

	writeCSNLOW();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	writeCSNHIGH();

}

uint8_t nrf24DetectSignalOnChannel(void){
	return nrf24ReadReg(RPD, 1);
}

uint8_t nrf24DataAvailable(void){

 	uint8_t reg_dt = nrf24ReadReg(FIFO_STATUS, 1);

	if(!(reg_dt & (1 << RX_EMPTY))){
		return 1;
	}

	return 0;
}

void nrf24ReceiveData(uint8_t *data, uint8_t size){
	uint8_t cmd = R_RX_PAYLOAD;

	writeCSNLOW();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Receive(&hspiX, data, size, spi_r_timeout);
	writeCSNHIGH();

	if(!nrf24ReadBit(FIFO_STATUS, RX_EMPTY)){
		nrf24ClearRX_DR();
	}
}

void delayMicroseconds(uint16_t del_time){
	__HAL_TIM_SET_COUNTER(&htimX, 0);
	uint16_t tmp_t = __HAL_TIM_GET_COUNTER(&htimX);
	while((__HAL_TIM_GET_COUNTER(&htimX)-tmp_t) < del_time){
		;
	}
}

void nrf24StartConstCarrier(){
	// TODO: implement whatever this was supposed to be
}

void nrf24StopConstCarrier(){
	// TODO: implement whatever this was supposed to be
}

void nrf24SetDefaults(void){
	writeCELOW();

	nrf24PowerDown();
	nrf24SetTXPower(3);
	nrf24SetDataRate(_1mbps);
	nrf24SetChannel(2);
	nrf24SetCRC(no_crc, _1byte);
	nrf24SetAddressWidth(5);
	nrf24FlushTX();
	nrf24FlushRX();
	nrf24ClearRX_DR();
	nrf24ClearTX_DS();
	nrf24ClearMAX_RT();
	nrf24SwitchToTX();
	nrf24EnableDynamicPayload(disable);
	nrf24EnableAckPayload(disable);
	nrf24EnableTransmitNoAck(disable);
	nrf24SetAutoRetransmissionDelay(0);
	nrf24SetAutoRetransmissionLimit(3);


	for(uint8_t i = 0; i < 5; i++){
		nrf24SetPipePayloadSize(i, 0);
		nrf24CloseRXPipe(i);
		nrf24SetRXDynamicPayloadPipe(i, disable);
		nrf24EnableAutoAck(i, enable);
	}

	writeCEHIGH();
}

void nrf24Init(void){

	if(HAL_TIM_Base_Start(&htimX) != HAL_OK){
		// Error_Handler(); // TODO: add this when used in main.c
	}

	nrf24PowerUp();

	nrf24FlushTX();
	nrf24FlushRX();

	nrf24ClearRX_DR();
	nrf24ClearTX_DS();
	nrf24ClearMAX_RT();
}
