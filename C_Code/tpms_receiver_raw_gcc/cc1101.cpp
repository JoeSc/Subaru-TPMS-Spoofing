/**
 * Copyright (c) 2011 panStamp <contact@panstamp.com>
 * 
 * This file is part of the panStamp project.
 * 
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 * 
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: 03/03/2011
 */
#include <stdio.h>
#include "cc1101.h"
#include <util/delay.h>

typedef uint8_t byte;

/**
 * Macros
 */
// Select (SPI) CC1101
#define cc1101_Select()  PORT_SPI_SS &= ~_BV(BIT_SPI_SS)
// Deselect (SPI) CC1101
#define cc1101_Deselect()  PORT_SPI_SS |= _BV(BIT_SPI_SS)
// Wait until SPI MISO line goes low
#define wait_Miso()  loop_until_bit_is_clear(PIN_SPI_MISO, BIT_SPI_MISO)
// Get GDO0 pin state
#define getGDO0state()  bitRead(PIN_GDO0, BIT_GDO0)
// Wait until GDO0 line goes high
#define wait_GDO0_high()  loop_until_bit_is_set( PIN_GDO0, BIT_GDO0)
// Wait until GDO0 line goes low
#define wait_GDO0_low()  loop_until_bit_is_clear( PIN_GDO0, BIT_GDO0)

 /**
  * PATABLE
  */

/**
 * CC1101
 * 
 * Class constructor
 */
CC1101::CC1101(void)
{
}

/**
 * wakeUp
 * 
 * Wake up CC1101 from Power Down state
 */
void CC1101::wakeUp(void)
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeReg
 * 
 * Write single register into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'value'	Value to be writen
 */
void CC1101::writeReg(byte regAddr, byte value) 
{
	printf("writeReg  0x%X to reg 0x%X\n", value, regAddr);
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(regAddr);                    // Send register address
  spi.send(value);                      // Send value
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeBurstReg
 * 
 * Write multiple registers into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'buffer'	Data to be writen
 * 'len'	Data length
 */
void CC1101::writeBurstReg(byte regAddr, byte* buffer, byte len)
{
  byte addr, i;
	printf("writeBurst  ");
  for(i=0 ; i<len ; i++)
	  printf(" 0x%X", buffer[i]);
  printf("  to reg 0x%X\n", regAddr);
  
  addr = regAddr | WRITE_BURST;         // Enable burst transfer
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  
  for(i=0 ; i<len ; i++)
    spi.send(buffer[i]);                // Send value

  cc1101_Deselect();                    // Deselect CC1101  
}

/**
 * cmdStrobe
 * 
 * Send command strobe to the CC1101 IC via SPI
 * 
 * 'cmd'	Command strobe
 */     
void CC1101::cmdStrobe(byte cmd) 
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(cmd);                        // Send strobe command
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * readReg
 * 
 * Read CC1101 register via SPI
 * 
 * 'regAddr'	Register address
 * 'regType'	Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 * 
 * Return:
 * 	Data byte returned by the CC1101 IC
 */
byte CC1101::readReg(byte regAddr, byte regType)
{
  byte addr, val;

  addr = regAddr | regType;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  val = spi.send(0x00);                 // Read result
  cc1101_Deselect();                    // Deselect CC1101

  return val;
}

/**
 * readBurstReg
 * 
 * Read burst data from CC1101 via SPI
 * 
 * 'buffer'	Buffer where to copy the result to
 * 'regAddr'	Register address
 * 'len'	Data length
 */
void CC1101::readBurstReg(byte * buffer, byte regAddr, byte len) 
{
  byte addr, i;
  
  addr = regAddr | READ_BURST;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  for(i=0 ; i<len ; i++)
    buffer[i] = spi.send(0x00);         // Read result byte by byte
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * reset
 * 
 * Reset CC1101
 */
void CC1101::reset(void) 
{
  cc1101_Deselect();                    // Deselect CC1101
  _delay_us(5);
  cc1101_Select();                      // Select CC1101
  _delay_us(10);
  cc1101_Deselect();                    // Deselect CC1101
  _delay_us(41);
  cc1101_Select();                      // Select CC1101

  wait_Miso();                          // Wait until MISO goes low
  spi.send(CC1101_SRES);                // Send reset command strobe
  wait_Miso();                          // Wait until MISO goes low

  cc1101_Deselect();                    // Deselect CC1101

  setCCregs();                          // Reconfigure CC1101
}

/**
 * setCCregs
 * 
 * Configure CC1101 registers
 */
void CC1101::setCCregs(void) 
{
  writeReg(CC1101_IOCFG2,  CC1101_DEFVAL_IOCFG2);
  writeReg(CC1101_IOCFG1,  CC1101_DEFVAL_IOCFG1);
  writeReg(CC1101_IOCFG0,  CC1101_DEFVAL_IOCFG0);
  writeReg(CC1101_FIFOTHR,  CC1101_DEFVAL_FIFOTHR);
  writeReg(CC1101_PKTLEN,  CC1101_DEFVAL_PKTLEN);
  writeReg(CC1101_PKTCTRL1,  CC1101_DEFVAL_PKTCTRL1);
  writeReg(CC1101_PKTCTRL0,  CC1101_DEFVAL_PKTCTRL0);

  writeReg(CC1101_SYNC1, CC1101_DEFVAL_SYNC1);
  writeReg(CC1101_SYNC0, CC1101_DEFVAL_SYNC0);

  writeReg(CC1101_ADDR, CC1101_DEFVAL_ADDR);

  writeReg(CC1101_CHANNR,  CC1101_DEFVAL_CHANNR);
  
  writeReg(CC1101_FSCTRL1,  CC1101_DEFVAL_FSCTRL1);
  writeReg(CC1101_FSCTRL0,  CC1101_DEFVAL_FSCTRL0);

  writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2);
  writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1);
  writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0);

  // RF speed
  writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4);
    
  writeReg(CC1101_MDMCFG3,  CC1101_DEFVAL_MDMCFG3);
  writeReg(CC1101_MDMCFG2,  CC1101_DEFVAL_MDMCFG2);
  writeReg(CC1101_MDMCFG1,  CC1101_DEFVAL_MDMCFG1);
  writeReg(CC1101_MDMCFG0,  CC1101_DEFVAL_MDMCFG0);
  writeReg(CC1101_DEVIATN,  CC1101_DEFVAL_DEVIATN);
  writeReg(CC1101_MCSM2,  CC1101_DEFVAL_MCSM2);
  writeReg(CC1101_MCSM1,  CC1101_DEFVAL_MCSM1);
  writeReg(CC1101_MCSM0,  CC1101_DEFVAL_MCSM0);
  writeReg(CC1101_FOCCFG,  CC1101_DEFVAL_FOCCFG);
  writeReg(CC1101_BSCFG,  CC1101_DEFVAL_BSCFG);
  writeReg(CC1101_AGCCTRL2,  CC1101_DEFVAL_AGCCTRL2);
  writeReg(CC1101_AGCCTRL1,  CC1101_DEFVAL_AGCCTRL1);
  writeReg(CC1101_AGCCTRL0,  CC1101_DEFVAL_AGCCTRL0);
  writeReg(CC1101_WOREVT1,  CC1101_DEFVAL_WOREVT1);
  writeReg(CC1101_WOREVT0,  CC1101_DEFVAL_WOREVT0);
  writeReg(CC1101_WORCTRL,  CC1101_DEFVAL_WORCTRL);
  writeReg(CC1101_FREND1,  CC1101_DEFVAL_FREND1);
  writeReg(CC1101_FREND0,  CC1101_DEFVAL_FREND0);
  writeReg(CC1101_FSCAL3,  CC1101_DEFVAL_FSCAL3);
  writeReg(CC1101_FSCAL2,  CC1101_DEFVAL_FSCAL2);
  writeReg(CC1101_FSCAL1,  CC1101_DEFVAL_FSCAL1);
  writeReg(CC1101_FSCAL0,  CC1101_DEFVAL_FSCAL0);
  writeReg(CC1101_RCCTRL1,  CC1101_DEFVAL_RCCTRL1);
  writeReg(CC1101_RCCTRL0,  CC1101_DEFVAL_RCCTRL0);
  writeReg(CC1101_FSTEST,  CC1101_DEFVAL_FSTEST);
  writeReg(CC1101_PTEST,  CC1101_DEFVAL_PTEST);
  writeReg(CC1101_AGCTEST,  CC1101_DEFVAL_AGCTEST);
  writeReg(CC1101_TEST2,  CC1101_DEFVAL_TEST2);
  writeReg(CC1101_TEST1,  CC1101_DEFVAL_TEST1);
  writeReg(CC1101_TEST0,  CC1101_DEFVAL_TEST0);
  
  // Send empty packet
  CCPACKET packet;
  packet.length = 0;
  sendData(packet);
}

/**
 * init
 * 
 * Initialize CC1101 radio
 *
 * @param freq Carrier frequency
 * @param mode Working mode (speed, ...)
 */
void CC1101::init()
{
  spi.init();                           // Initialize SPI interface
  //pinMode(GDO0, INPUT);                 // Config GDO0 as input
  DDR_GDO0 &= ~_BV(BIT_GDO0);
  PORT_GDO0 |= (1 << _BV(BIT_GDO0));

  reset();                              // Reset CC1101

  // Configure PATABLE
  setTxPowerAmp(PA_LowPower);

  ////uint8_t paTable[8];
  ////paTable[0] =0;// = {0x00, 0x51, 0, 0, 0, 0, 0, 0};
  //uint8_t foo[8];
  //foo[0] = 0;
  //foo[1] = 0x51;
  //writeBurstReg(CC1101_PATABLE, foo, 2);
}

/**
 * sendData
 * 
 * Send data packet via RF
 * 
 * 'packet'	Packet to be transmitted. First byte is the destination address
 *
 *  Return:
 *    True if the transmission succeeds
 *    False otherwise
 */
bool CC1101::sendData(CCPACKET packet)
{
  byte marcState;
  bool res = false;
uint8_t mdmcfg2;
 
  // Declare to be in Tx state. This will avoid receiving packets whilst
  // transmitting
  rfState = RFSTATE_TX;

  // Enter RX state
  setRxState();

//  uint8_t foo[8];
//  foo[0] = 0x00;
//  foo[1] = 0xC0;
//  writeBurstReg(CC1101_PATABLE, foo, 2);


  // Check that the RX state has been entered
  while (((marcState = readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x0D)
  {
    if (marcState == 0x11)        // RX_OVERFLOW
      flushRxFifo();              // flush receive queue
  }

  _delay_us(500);

//mdmcfg2 = readStatusReg(CC1101_MDMCFG2);
  //writeReg(CC1101_MDMCFG2,  mdmcfg2 & 0xf8);

  if (packet.length > 0)
  {
    // Write data into the TX FIFO
    writeBurstReg(CC1101_TXFIFO, packet.data, packet.length);

    // CCA enabled: will enter TX state only if the channel is clear
    setTxState();
  }

  // Check that TX state is being entered (state = RXTX_SETTLING)
  marcState = readStatusReg(CC1101_MARCSTATE) & 0x1F;
  if((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15))
  {
    setIdleState();       // Enter IDLE state
    flushTxFifo();        // Flush Tx FIFO
    setRxState();         // Back to RX state

    // Declare to be in Rx state
    rfState = RFSTATE_RX;
    return false;
  }

  // Wait for the sync word to be transmitted
  wait_GDO0_high();

  // Wait until the end of the packet transmission
  wait_GDO0_low();

  //writeReg(CC1101_MDMCFG2,  mdmcfg2);

  // Check that the TX FIFO is empty
  if((readStatusReg(CC1101_TXBYTES) & 0x7F) == 0)
    res = true;

  setIdleState();       // Enter IDLE state
  flushTxFifo();        // Flush Tx FIFO

  // Enter back into RX state
  setRxState();

  // Declare to be in Rx state
  rfState = RFSTATE_RX;

  return res;
}

/**
 * receiveData
 * 
 * Read data packet from RX FIFO
 *
 * 'packet'	Container for the packet received
 * 
 * Return:
 * 	Amount of bytes received
 */
byte CC1101::receiveData(CCPACKET * packet)
{
  byte val;
  byte rxBytes = readStatusReg(CC1101_RXBYTES);

  // Any byte waiting to be read and no overflow?
  if (rxBytes & 0x7F && !(rxBytes & 0x80))
  {
    // Read data length
    if (rxBytes > CCPACKET_DATA_LEN)
      packet->length = 0;   // Discard packet
    else
    {
		packet->length = rxBytes;
      // Read data packet
      readBurstReg(packet->data, CC1101_RXFIFO, rxBytes);
      // Read RSSI
      packet->rssi = readConfigReg(CC1101_RXFIFO);
      // Read LQI and CRC_OK
      val = readConfigReg(CC1101_RXFIFO);
      packet->lqi = val & 0x7F;
      packet->crc_ok = val & _BV(7);
    }
  }
  else
    packet->length = 0;

  setIdleState();       // Enter IDLE state
  flushRxFifo();        // Flush Rx FIFO
  //cmdStrobe(CC1101_SCAL);

  // Back to RX state
  setRxState();

  return packet->length;
}

/**
 * setRxState
 * 
 * Enter Rx state
 */
void CC1101::setRxState(void)
{
  cmdStrobe(CC1101_SRX);
  rfState = RFSTATE_RX;

}

/**
 * setTxState
 * 
 * Enter Tx state
 */
void CC1101::setTxState(void)
{

  cmdStrobe(CC1101_STX);
  rfState = RFSTATE_TX;
}
