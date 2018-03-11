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

#include "simplespi.h"

//#define SPI_SS CC1101_SPI_SS     // PB2 = SPI_SS
//#define SPI_MOSI MOSI            // PB3 = MOSI
//#define SPI_MISO MISO            // PB4 = MISO
//#define SPI_SCK  SCK             // PB5 = SCK
//#define GDO0 CC1101_GDO0         // PD2 = INT0
//
//#define PORT_SPI_MISO  PINB
//#define BIT_SPI_MISO  4
//
//#define PORT_SPI_SS  PORTB
//#define BIT_SPI_SS   2
//
//#define PORT_GDO0  PIND
//#define BIT_GDO0  2

/**
 * init
 * 
 * SPI initialization
 */
void SIMPLESPI::init() 
{
  //digitalWrite(SPI_SS, HIGH);
  PORTB |= _BV(BIT_SPI_SS);


  
  // Configure SPI pins
  //pinMode(SPI_SS, OUTPUT);
  DDR_SPI_SS |= _BV(BIT_SPI_SS);
  //pinMode(SPI_MOSI, OUTPUT);
  DDR_SPI_MOSI |= _BV(BIT_SPI_MOSI);
  //pinMode(SPI_MISO, INPUT);
  DDR_SPI_MISO &= ~_BV(BIT_SPI_MISO);
  //pinMode(SPI_SCK, OUTPUT);
  DDR_SPI_SCK |= _BV(BIT_SPI_SCK);

  //digitalWrite(SPI_SCK, HIGH);
  PORT_SPI_SCK |= _BV(BIT_SPI_SCK);
  //digitalWrite(SPI_MOSI, LOW);
  PORT_SPI_MOSI |= _BV(BIT_SPI_MOSI);

  // SPI speed = clk/4
  SPCR = _BV(SPE) | _BV(MSTR);
}

/**
 * send
 * 
 * Send uint8_t via SPI
 * 
 * 'value'	Value to be sent
 * 
 * Return:
 * 	Response received from SPI slave
 */
uint8_t SIMPLESPI::send(uint8_t value) 
{
  SPDR = value;                          // Transfer uint8_t via SPI
  wait_Spi();                            // Wait until SPI operation is terminated

  return SPDR;
}
