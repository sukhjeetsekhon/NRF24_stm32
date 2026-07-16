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


void write_csn_high(void){

	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_SET
	);
}

void write_csn_low(void){

	HAL_GPIO_WritePin(
		CSN_GPIO_PORT,
		CSN_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void write_ce_high(void){

	HAL_GPIO_WritePin(
		CE_GPIO_PORT,
		CE_GPIO_PIN,
		GPIO_PIN_SET
	);
}

void write_ce_low(void){

	HAL_GPIO_WritePin(
		CE_GPIO_PORT,
		CE_GPIO_PIN,
		GPIO_PIN_RESET
	);
}

void nrf24_write_reg(uint8_t reg, uint8_t *data, uint8_t size){


	uint8_t cmd = W_REGISTER | reg;

	write_csn_low();

	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);

	write_csn_high();
}

uint8_t nrf24_read_reg(uint8_t reg, uint8_t size){

	uint8_t cmd = R_REGISTER | reg;
	uint8_t data = 0;

	write_csn_low();

	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Receive(&hspiX, &data, size, spi_r_timeout);

	write_csn_high();

	return data;
}

void nrf24_write_special_command(uint8_t cmd){

	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
}

void nrf24_write_special_reg(uint8_t *data, uint8_t size){

	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
}

void nrf24_read_special_reg(uint8_t *data, uint8_t size){

	HAL_SPI_Receive(&hspiX, data, size, spi_r_timeout);
}

void nrf24_power_up(void){
	uint8_t data = 0;

	data = nrf24_read_reg(CONFIG, 1);

	data |= (1 << PWR_UP);

	nrf24_write_reg(CONFIG, &data, 1);
}

void nrf24_power_down(void){
	uint8_t data = 0;

	data = nrf24_read_reg(CONFIG, 1);

	data &= ~(1 << PWR_UP);

	nrf24_write_reg(CONFIG, &data, 1);
}

void nrf24_set_tx_power(uint8_t pwr){
	uint8_t data = 0;

	data = nrf24_read_reg(RF_SETUP, 1);

	data &= 184;

	data |= (pwr << RF_PWR);

	nrf24_write_reg(RF_SETUP, &data, 1);
}

void nrf24_set_data_rate(uint8_t bps){
	uint8_t data = 0;

	data = nrf24_read_reg(RF_SETUP, 1);

	data &= ~(1 << RF_DR_LOW) & ~(1 << RF_DR_HIGH);

	if(bps == _2mbps){
		data |= (1 << RF_DR_HIGH);
	}else if(bps == _250kbps){
		data |= (1 << RF_DR_LOW);
	}

	nrf24_write_reg(RF_SETUP, &data, 1);
}

void nrf24_set_channel(uint8_t ch){
	nrf24_write_reg(RF_CH, &ch, 1);
}

void nrf24_open_tx_pipe(uint8_t *addr){
	nrf24_write_reg(TX_ADDR, addr, 5);
}

void nrf24_set_pipe_payload_size(uint8_t pipe, uint8_t size){
	if(size > 32){
		size = 32;
	}

	switch(pipe){
	case 0:
		nrf24_write_reg(RX_PW_P0, &size, 1);

		break;
	case 1:
		nrf24_write_reg(RX_PW_P1, &size, 1);

		break;
	case 2:
		nrf24_write_reg(RX_PW_P2, &size, 1);

		break;
	case 3:
		nrf24_write_reg(RX_PW_P3, &size, 1);

		break;
	case 4:
		nrf24_write_reg(RX_PW_P4, &size, 1);

		break;
	case 5:
		nrf24_write_reg(RX_PW_P5, &size, 1);

		break;
	}
}

void nrf24_open_rx_pipe(uint8_t pipe, uint8_t *addr){

	uint8_t data = 0;

	data = nrf24_read_reg(EN_RXADDR, 1);

	switch(pipe){
	case 0:
		nrf24_write_reg(RX_ADDR_P0, addr, 5);

		data |= (1 << ERX_P0);
		break;
	case 1:
		nrf24_write_reg(RX_ADDR_P1, addr, 5);

		data |= (1 << ERX_P1);
		break;
	case 2:
		nrf24_write_reg(RX_ADDR_P2, addr, 1);

		data |= (1 << ERX_P2);
		break;
	case 3:
		nrf24_write_reg(RX_ADDR_P3, addr, 1);

		data |= (1 << ERX_P3);
		break;
	case 4:
		nrf24_write_reg(RX_ADDR_P4, addr, 1);

		data |= (1 << ERX_P4);
		break;
	case 5:
		nrf24_write_reg(RX_ADDR_P5, addr, 1);

		data |= (1 << ERX_P5);
		break;
	}

	nrf24_write_reg(EN_RXADDR, &data, 1);
}

void nrf24_close_rx_pipe(uint8_t pipe){
	uint8_t data = nrf24_read_reg(EN_RXADDR, 1);

	data &= ~(1 << pipe);

	nrf24_write_reg(EN_RXADDR, &data, 1);
}

void nrf24_set_crc(uint8_t en_crc, uint8_t crc0){
	uint8_t data = nrf24_read_reg(CONFIG, 1);

	data &= ~(1 << EN_CRC) & ~(1 << CRCO);

	data |= (en_crc << EN_CRC) | (crc0 << CRCO);

	nrf24_write_reg(CONFIG, &data, 1);
}

void nrf24_set_address_width(uint8_t bytes){
	bytes -= 2;
	nrf24_write_reg(SETUP_AW, &bytes, 1);
}

void nrf24_flush_tx(void){
	write_csn_low();
	nrf24_write_special_command(FLUSH_TX);
	write_csn_high();
}

void nrf24_flush_rx(void){
	write_csn_low();
	nrf24_write_special_command(FLUSH_RX);
	write_csn_high();
}

uint8_t nrf24_read_status_reg(void){
	uint8_t data = 0;
	uint8_t cmd = NOP;

	write_csn_low();
	HAL_SPI_TransmitReceive(&hspiX, &cmd, &data, 1, spi_rw_timeout);
	write_csn_high();

	return data;
}

void nrf24_clear_rx_dr(void){
	uint8_t data = 0;

	data = nrf24_read_status_reg();

	data |= (1 << RX_DR);

	nrf24_write_reg(STATUS, &data, 1);
}

void nrf24_clear_tx_ds(void){
	uint8_t data = 0;

	data = nrf24_read_status_reg();

	data |= (1 << TX_DS);

    nrf24_write_reg(STATUS, &data, 1);
}

void nrf24_clear_max_rt(void){
	uint8_t data = 0;

	data = nrf24_read_status_reg();

	data |= (1 << MAX_RT);

    nrf24_write_reg(STATUS, &data, 1);
}

uint8_t nrf24_read_bit(uint8_t reg, uint8_t bit){

	if(nrf24_read_reg(reg, 1) & (1 << bit)){
		return 1;
	}

	return 0;
}

void nrf24_set_bit(uint8_t reg, uint8_t bit, uint8_t val){
	uint8_t data = 0;

	data = nrf24_read_reg(reg, 1);

	if(val){
		data |= (1 << bit);
	}else{
		data &= ~(1 << bit);
	}

    nrf24_write_reg(reg, &data, 1);
}

uint8_t nrf24_read_payload_width(void){
	uint8_t width = 0;

	write_csn_low();
	nrf24_write_special_command(R_RX_PL_WID);
	nrf24_read_special_reg(&width, 1);
	write_csn_high();

	return width;
}

void nrf24_switch_to_rx(void){
	uint8_t data = 0;

	data = nrf24_read_reg(CONFIG, 1);

	data |= (1 << PRIM_RX);

	nrf24_write_reg(CONFIG, &data, 1);

	nrf24_write_ce_high();
}

void nrf24_switch_to_tx(void){
	uint8_t data = 0;

	data = nrf24_read_reg(CONFIG, 1);

	data &= ~(1 << PRIM_RX);

	nrf24_write_reg(CONFIG, &data, 1);
}

void nrf24_enable_dynamic_payload(uint8_t en){
	uint8_t feature = nrf24_read_reg(FEATURE, 1);

	if(en == enable){
		feature |= (1 << EN_DPL);
	}else{
		feature &= ~(1 << EN_DPL);
	}

	nrf24_write_reg(FEATURE, &feature, 1);
}

void nrf24_set_rx_dynamic_payload_pipe(uint8_t pipe, uint8_t en){

	uint8_t dynpd = nrf24_read_reg(DYNPD, 1);

	if(pipe > 5){
		pipe = 5;
	}

	if(en){
		dynpd |= (1 << pipe);
	}else{
		dynpd &= ~(1 << pipe);
	}

	nrf24_write_reg(DYNPD, &dynpd, 1);
}

void nrf24_enable_auto_ack(uint8_t pipe, uint8_t ack){

	if(pipe > 5){
		pipe = 5;
	}

	uint8_t enaa = nrf24_read_reg(EN_AA, 1);

	if(ack){
		enaa |= (1 << pipe);
	}else{
		enaa &= ~(1 << pipe);
	}

	nrf24_write_reg(EN_AA, &enaa, 1);
}

void nrf24_enable_auto_ack_on_all_pipes(uint8_t ack){
	uint8_t enaa = nrf24_read_reg(EN_AA, 1);

	if(ack){
		enaa = 63;
	}else{
		enaa = 0;
	}

	nrf24_write_reg(EN_AA, &enaa, 1);
}

void nrf24_enable_ack_payload(uint8_t en){
	uint8_t feature = nrf24_read_reg(FEATURE, 1);

	if(en){
		feature |= (1 << EN_ACK_PAY);
	}else{
		feature &= ~(1 << EN_ACK_PAY);
	}

	nrf24_write_reg(FEATURE, &feature, 1);
}

void nrf24_enable_transmit_no_ack(uint8_t en){
	uint8_t feature = nrf24_read_reg(FEATURE, 1);

	if(en){
		feature |= (1 << EN_DYN_ACK);
	}else{
		feature &= ~(1 << EN_DYN_ACK);
	}

	nrf24_write_reg(FEATURE, &feature, 1);
}

void nrf24_set_auto_retransmission_delay(uint8_t delay){
	uint8_t data = nrf24_read_reg(SETUP_RETR, 1);

	data &= 15;

	data |= (delay << ARD);

	nrf24_write_reg(SETUP_RETR, &data, 1);
}

void nrf24_set_auto_retransmission_limit(uint8_t limit){
	uint8_t data = nrf24_read_reg(SETUP_RETR, 1);

	data &= 240;

	data |= (limit << ARC);

	nrf24_write_reg(SETUP_RETR, &data, 1);
}

void nrf24_convert_type_to_uint8_array(size_t in, uint8_t* out, uint16_t size){
	for(uint16_t i = 0; i < size; i++){
		out[i] = (((in & (255 << (i*8)))) >> (i*8));
	}
}

size_t nrf24_convert_uint8_array_to_type(uint8_t* in, uint16_t size){
	size_t out = 0;

	for(uint16_t i = 0; i < size; i++){
		out |= (in[i] << (8*i));
	}

	return out;
}


uint8_t nrf24_transmit_data(uint8_t *data, uint8_t size){

	nrf24_write_ce_low();

	uint8_t cmd = W_TX_PAYLOAD;

	write_csn_low();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	write_csn_high();

	nrf24_write_ce_high();
	nrf24_delay_microseconds(20);
	nrf24_write_ce_low();

	if(nrf24_read_status_reg() & (1 << MAX_RT)){
		nrf24_clear_max_rt();
		nrf24_flush_tx();
		return 1;
	}

	return 0;
}

uint8_t nrf24_transmit_data_no_ack(uint8_t *data, uint8_t size){

	nrf24_write_ce_low();

	uint8_t cmd = W_TX_PAYLOAD_NOACK;

	write_csn_low();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	write_csn_high();

	nrf24_write_ce_high();
	nrf24_delay_microseconds(20);
	nrf24_write_ce_low();

	if(nrf24_read_status_reg() & (1 << MAX_RT)){
		nrf24_clear_max_rt();
		nrf24_flush_tx();
		return 1;
	}

	return 0;
}

void nrf24_transmit_rx_payload_as_rx(uint8_t pipe, uint8_t *data, uint8_t size){

	if(pipe > 5){
		pipe = 5;
	}

	uint8_t cmd = (W_ACK_PAYLOAD | pipe);

	write_csn_low();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Transmit(&hspiX, data, size, spi_w_timeout);
	write_csn_high();

}

uint8_t nrf24_detect_signal_on_channel(void){
	return nrf24_read_reg(RPD, 1);
}

uint8_t nrf24_data_available(void){

 	uint8_t reg_dt = nrf24_read_reg(FIFO_STATUS, 1);

	if(!(reg_dt & (1 << RX_EMPTY))){
		return 1;
	}

	return 0;
}

void nrf24_receive_data(uint8_t *data, uint8_t size){
	uint8_t cmd = R_RX_PAYLOAD;

	write_csn_low();
	HAL_SPI_Transmit(&hspiX, &cmd, 1, spi_w_timeout);
	HAL_SPI_Receive(&hspiX, data, size, spi_r_timeout);
	write_csn_high();

	if(!nrf24ReadBit(FIFO_STATUS, RX_EMPTY)){
		nrf24_clear_rx_dr();
	}
}

void nrf24_delay_microseconds(uint16_t del_time){
	__HAL_TIM_SET_COUNTER(&htimX, 0);
	uint16_t tmp_t = __HAL_TIM_GET_COUNTER(&htimX);
	while((__HAL_TIM_GET_COUNTER(&htimX)-tmp_t) < del_time){
		;
	}
}

void nrf24_start_const_carrier(void){
	// TODO: implement whatever this was supposed to be
}


void nrf24_stop_const_carrier(void){
	// TODO: implement whatever this was supposed to be
}

void nrf24_set_defaults(void){
	nrf24_write_ce_low();


	nrf24_power_down();

	nrf24_set_tx_power(3);
	nrf24_set_data_rate(_1mbps);
	nrf24_set_channel(2);
	nrf24_set_crc(no_crc, _1byte);
	nrf24_set_address_width(5);
	nrf24_flush_tx();
	nrf24_flush_rx();
	nrf24_clear_rx_dr();
	nrf24_clear_tx_dr();
	nrf24_clear_max_rt();
	nrf24_switch_to_tx();
	nrf24_enable_dynamic_payload(disable);
	nrf24_enable_ack_payload(disable);
	nrf24_enable_transmit_no_ack(disable);
	nrf24_set_auto_retransmission_delay(0);
	nrf24_set_auto_retransmission_limit(3);


	for(uint8_t i = 0; i < 5; i++){
		nrf24_set_pipe_payload_size(i, 0);
		nrf24_close_rx_pipe(i);
		nrf24_set_rx_dynamic_payload_pipe(i, disable);
		nrf24_enable_auto_ack(i, enable);
	}

	nrf24_write_ce_high();
}

void nrf24_init(void){

	if(HAL_TIM_Base_Start(&htimX) != HAL_OK){
		// Error_Handler(); // TODO: add this when used in main.c
	}

	nrf24_power_up();

	nrf24_flush_tx();
	nrf24_flush_rx();

	nrf24_clear_rx_dr();
	nrf24_clear_tx_dr();
	nrf24_clear_max_rt();
}
