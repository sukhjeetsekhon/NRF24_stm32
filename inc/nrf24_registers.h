/**
   @file nrf24_registers.h

   @brief nRF24L01+ registers, addresses, and bit positions

   You can configure and control the radio by accessing the register map through the SPI.

   @note Addresses 18 to 1B are reserved for test purposes, altering them makes the chip malfunction.

   @see docs\nRF24L01P_Product_Specification_1_0.pdf

   Pages: 57-63

*/

#ifndef NRF24_REGISTERS_H
#define NRF24_REGISTERS_H

/***********************************CONFIG*************************************/
#define CONFIG 0x00 // Configuration Register

/**
   Mask interrupt caused by RX_DR
   1: Interrupt not reflected on the IRQ pin
   0: Reflect RX_DR as active low interrupt on the IRQ pin
*/
#define MASK_RX_DR 6

/**
   Mask interrupt caused by TX_DS
   1: Interrupt not reflected on the IRQ pin
   0: Reflect TX_DS as active low interrupt on the IRQ pin
*/
#define MASK_TX_DS 5

/**
   Mask interrupt caused by MAX_RT
   1: Interrupt not reflected on the IRQ pin
   0: Reflect MAX_RT as active low interrupt on the IRQ pin
*/
#define MASK_MAX_RT 4

/**
   Enable CRC. Forced high if one of the bits in the EN_AA is high
*/
#define EN_CRC 3

/**
   CRC encoding scheme
   '0' - 1 byte
   '1' – 2 bytes
*/
#define CRCO 2

/**
   1: POWER UP
   0: POWER DOWN
*/
#define PWR_UP 1

/**
   RX/TX control
   1: PRX
   0: PTX
*/
#define PRIM_RX 0

/************************************EN_AA*************************************/
/**
   Enhanced ShockBurst™
   Enable ‘Auto Acknowledgment’ Function
   Disable this functionality to be compatible with nRF2401,
   see page 75
*/
#define EN_AA 0x01

/**
   Enable auto acknowledgement data pipe 5
*/
#define ENAA_P5 5

/**
   Enable auto acknowledgement data pipe 4
*/
#define ENAA_P4 4

/**
   Enable auto acknowledgement data pipe 3
*/
#define ENAA_P3 3

/**
   Enable auto acknowledgement data pipe 2
*/
#define ENAA_P2 2

/**
   Enable auto acknowledgement data pipe 1
*/
#define ENAA_P1 1

/**
   Enable auto acknowledgement data pipe 0
*/
#define ENAA_P0 0

/**********************************EN_RXADDR***********************************/
#define EN_RXADDR 0x02 // Enabled RX Addresses

/**
   Enable data pipe 5.
*/
#define ERX_P5 5

/**
   Enable data pipe 4.
*/
#define ERX_P4 4

/**
   Enable data pipe 3.
*/
#define ERX_P3 3

/**
   Enable data pipe 2.
*/
#define ERX_P2 2

/**
   Enable data pipe 1.
*/
#define ERX_P1 1

/**
   Enable data pipe 0.
*/
#define ERX_P0 0

/***********************************SETUP_AW***********************************/
/**
   Setup of Address Widths
   (common for all data pipes)
*/
#define SETUP_AW 0x03

/**
   Bits: [1:0]

   RX/TX Address field width
   '00' - Illegal
   '01' - 3 bytes
   '10' - 4 bytes
   '11' – 5 bytes
   LSByte is used if address width is below 5 bytes
*/
#define AW 0

/*********************************SETUP_RETR***********************************/
#define SETUP_RETR 0x04 // Setup of Automatic Retransmission

/**
   Bits: [7:4]

   Auto Retransmit Delay
   ‘0000’ – Wait 250µS
   ‘0001’ – Wait 500µS
   ‘0010’ – Wait 750µS
   ...
   ‘1111’ – Wait 4000µS
   (Delay defined from end of transmission to start of next transmission)

   Please take care when setting this parameter.
   If the ACK payload is more than 15 byte in 2Mbps mode the ARD must be 500µS or more.
   If the ACK payload is more than 5 byte in 1Mbps mode the ARD must be 500µS or more.
   In 250kbps mode (even when the payload is not in ACK) the ARD must be 500µS or more.
   Please see section 7.4.2 on page 33 for more information.

   [(Delay defined from end of transmission to start of next transmission)] is the time the PTX is waiting for an ACK packet before a retransmit is made. The PTX is in RX mode for 250µS (500µS in 250kbps mode) to wait for address match.
   If the address match is detected, it stays in RX mode to the end of the packet, unless ARD elapses.
   Then it goes to standby-II mode for the rest of the specified ARD.
   After the ARD it goes to TX mode and then retransmits the packet.
*/
#define ARD 4

/**
   Bits: [0:3]

   Auto Retransmit Count
   ‘0000’ –Re-Transmit disabled
   ‘0001’ – Up to 1 Re-Transmit on fail of AA
   ...
   ‘1111’ – Up to 15 Re-Transmit on fail of AA
*/
#define ARC 0

/********************************RF_CH_ADDRESS*********************************/
#define RF_CH_ADDRESS 0x05 // RF Channel

/**
   Bits: [6:0]

   Sets the frequency channel nRF24L01+ operates on
*/
#define RF_CH 0

/**********************************RF_SETUP************************************/
#define RF_SETUP 0x06 // RF Setup Register

/**
   Enables continuous carrier transmit when high.
*/
#define CONT_WAVE 7

/**
   Set RF Data Rate to 250kbps. See RF_DR_HIGH for encoding.
*/
#define RF_DR_LOW 5

/**
   Force PLL lock signal. Only used in test
*/
#define PLL_LOCK 4

/**
   Select between the high speed data rates.
   This bit is don’t care if RF_DR_LOW is set.
   Encoding:
   [RF_DR_LOW, RF_DR_HIGH]:
   ‘00’ – 1Mbps
   ‘01’ – 2Mbps
   ‘10’ – 250kbps
   ‘11’ – Reserved
*/
#define RF_DR_HIGH 3

/**
   Bits: [2:1]

   Set RF output power in TX mode
   '00' – -18dBm
   '01' – -12dBm
   '10' – -6dBm
   '11' – 0dBm
*/
#define RF_PWR 1

/***********************************STATUS*************************************/
/**
   Status Register
   (In parallel to the SPI command word applied on the MOSI pin, the STATUS register is shifted serially out on the MISO pin)
*/
#define STATUS 0x07

/**
   Data Ready RX FIFO interrupt.
   Asserted when new data arrives RX FIFOc.
   Write 1 to clear bit.

   The RX_DR IRQ is asserted by a new packet arrival event.
   The procedure for handling this interrupt should be:
      1) read payload through SPI
      2) clear RX_DR IRQ
      3) read FIFO_STATUS to check if there are more payloads available in RX FIFO
      4) if there are more data in RX FIFO repeat from step 1).
*/
#define RX_DR 6

/**
   Data Sent TX FIFO interrupt.
   Asserted when packet transmitted on TX.
   If AUTO_ACK is activated, this bit is set high only when ACK is received.
   Write 1 to clear bit.
*/
#define TX_DS 5

/**
   Maximum number of TX retransmits interrupt
   Write 1 to clear bit.
   If MAX_RT is asserted it must be cleared to enable further communication.
*/
#define MAX_RT 4

/**
   Bits: [3:1]

   Data pipe number for the payload available for reading from RX_FIFO
   000-101: Data Pipe Number
   110: Not Used
   111: RX FIFO Empty
*/
#define RX_P_NO 1

/**
   TX FIFO full flag.
   1: TX FIFO full.
   0: Available locations in TX FIFO.
*/
#define TX_FULL 0

/**********************************OBSERVE_TX**********************************/
#define OBSERVE_TX 0x08 // Transmit observe register

/**
   Bits: [7:4]

   Count lost packets.
   The counter is overflow protected to 15, and discontinues at max until reset.
   The counter is reset by writing to RF_CH.
   See page 75.
*/
#define PLOS_CNT 4

/**
   Bits: [3:0]

   Count retransmitted packets.
   The counter is reset when transmission of a new packet starts.
   See page 75.
*/
#define ARC_CNT 0

/*********************************RPD_ADDRESS**********************************/
#define RPD_ADDRESS 0x09 // Received Power Detector Register

/**
   Received Power Detector.
   This register is called CD (Carrier Detect) in the nRF24L01.
   The name is different in nRF24L01+ due to the different input power level threshold for this bit.
   See section 6.4 on page 25.
*/
#define RPD 0

/*********************************RX_ADDR_P0***********************************/

/**
   Bits: [39:0]
   Receive address data pipe 0.
   5 Bytes maximum length.
   (LSByte is written first. Write the number of bytes defined by SETUP_AW)
*/
#define RX_ADDR_P0 0x0A

/*********************************RX_ADDR_P1***********************************/

/**
   Bits: [39:0]

   Receive address data pipe 1.
   5 Bytes maximum length.
   (LSByte is written first. Write the number of bytes defined by SETUP_AW)
*/
#define RX_ADDR_P1 0x0B

/*********************************RX_ADDR_P2***********************************/

/**
   Bits: [7:0]

   Receive address data pipe 2.
   Only LSB.
   MSBytes are equal to RX_ADDR_P1[39:8]
*/
#define RX_ADDR_P2 0x0C

/*********************************RX_ADDR_P3***********************************/

/**
   Bits: [7:0]

   Receive address data pipe 3.
   Only LSB.
   MSBytes are equal to RX_ADDR_P1[39:8]
*/
#define RX_ADDR_P3 0x0D

/*********************************RX_ADDR_P3***********************************/

/**
   Bits: [7:0]

   Receive address data pipe 4.
   Only LSB.
   MSBytes are equal to RX_ADDR_P1[39:8]
*/
#define RX_ADDR_P4 0x0E

/*********************************RX_ADDR_P5***********************************/

/**
   Bits: [7:0]

   Receive address data pipe 5.
   Only LSB.
   MSBytes are equal to RX_ADDR_P1[39:8]
*/
#define RX_ADDR_P5 0x0F

/***********************************TX_ADDR************************************/

/**
   Bits: [39:0]

   Transmit address.
   Used for a PTX device only.
   (LSByte is written first)
   Set RX_ADDR_P0 equal to this address to handle automatic acknowledge if this is a PTX device with Enhanced ShockBurst™ enabled.
   See page 75.
*/
#define TX_ADDR 0x10

/******************************RX_PW_P0_ADDRESS********************************/

#define RX_PW_P0_ADDRESS 0x11

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 0 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P0 0

/******************************RX_PW_P1_ADDRESS********************************/

#define RX_PW_P1_ADDRESS 0x12

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 1 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P1 0

/******************************RX_PW_P2_ADDRESS********************************/

#define RX_PW_P2_ADDRESS 0x13

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 2 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P2 0

/******************************RX_PW_P3_ADDRESS********************************/

#define RX_PW_P3_ADDRESS 0x14

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 3 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P3 0

/******************************RX_PW_P4_ADDRESS********************************/

#define RX_PW_P4_ADDRESS 0x15

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 4 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P4 0

/******************************RX_PW_P5_ADDRESS********************************/

#define RX_PW_P5_ADDRESS 0x16

/**
   Bits: [5:0]

   Number of bytes in RX payload in data pipe 5 (1 to 32 bytes).
   0 Pipe not used
   1 = 1 byte
   ...
   32 = 32 bytes
*/
#define RX_PW_P5 0

/*******************************FIFO_STATUS************************************/

#define FIFO_STATUS 0x17 // FIFO Status Register

/**
   Used for a PTX device
   Pulse the rfce high for at least 10µs to Reuse last transmitted payload.
   TX payload reuse is active until W_TX_PAYLOAD or FLUSH TX is executed.
   TX_REUSE is set by the SPI command REUSE_TX_PL, and is reset by the SPI commands W_TX_PAYLOAD or FLUSH TX
*/
#define TX_REUSE 6

/**
   TX FIFO full flag.
   1: TX FIFO full.
   0: Available locations in TX FIFO.
*/
#define TX_FULL 5

/**
   TX FIFO empty flag.
   1: TX FIFO empty.
   0: Data in TX FIFO.
*/
#define TX_EMPTY 4

/**
   RX FIFO full flag.
   1: RX FIFO full.
   0: Available locations in RX FIFO.
*/
#define RX_FULL 1

/**
   RX FIFO empty flag.
   1: RX FIFO empty.
   0: Data in RX FIFO.
*/
#define RX_EMPTY 0
/***********************************ACK_PLD************************************/

/**
   Bits: [255:0]

   Written by separate SPI command
   ACK packet payload to data pipe number PPP given in SPI command.
   Used in RX mode only.
   Maximum three ACK packet payloads can be pending.
   Payloads with same PPP are handled first in first out.
*/
#define ACK_PLD

/***********************************TX_PLD*************************************/

/**
   Bits: [255:0]

   Written by separate SPI command TX data payload register.
   1 - 32 bytes.
   This register is implemented as a FIFO with three levels.
   Used in TX mode only.
*/
#define TX_PLD

/***********************************RX_PLD*************************************/

/**
   Bits: [255:0]

   Read by separate SPI command.
   RX data payload register.
   1 - 32 bytes.
   This register is implemented as a FIFO with three levels.
   All RX channels share the same FIFO.
*/
#define RX_PLD

/***********************************DYNPD**************************************/

#define DYNPD 0x1C // Enable dynamic payload length

/**
   Enable dynamic payload length data pipe 5.
   (Requires EN_DPL and ENAA_P5)
*/
#define DPL_P5 5

/**
   Enable dynamic payload length data pipe 4.
   (Requires EN_DPL and ENAA_P4)
*/
#define DPL_P4 4

/**
   Enable dynamic payload length data pipe 3.
   (Requires EN_DPL and ENAA_P3)
*/
#define DPL_P3 3

/**
   Enable dynamic payload length data pipe 2.
   (Requires EN_DPL and ENAA_P2)
*/
#define DPL_P2 2

/**
   Enable dynamic payload length data pipe 1.
   (Requires EN_DPL and ENAA_P1)
*/
#define DPL_P1 1

/**
   Enable dynamic payload length data pipe 0.
   (Requires EN_DPL and ENAA_P0)
*/
#define DPL_P0 0

/***********************************FEATURE************************************/

#define FEATURE 0x1D // Feature Register

/**
   Enables Dynamic Payload Length
*/
#define EN_DPL 2

/**
   Enables Payload with ACK

   If ACK packet payload is activated, ACK packets have dynamic payload lengths and the Dynamic Payload Length feature should be enabled for pipe 0 on the PTX and PRX.
   This is to ensure that they receive the ACK packets with payloads.

   If the ACK payload is more than 15 byte in 2Mbps mode the ARD must be 500µS or more
   If the ACK payload is more than 5 byte in 1Mbps mode the ARD must be 500µS or more.
   In 250kbps mode (even when the payload is not in ACK) the ARD must be 500µS or more.
*/
#define EN_ACK_PAY 1

/**
   Enables the W_TX_PAYLOAD_NOACK command
*/
#define EN_DYN_ACK 0

#endif /* NRF24_REGISTERS_H */