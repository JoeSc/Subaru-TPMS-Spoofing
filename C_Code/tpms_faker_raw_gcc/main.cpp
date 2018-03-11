extern "C"
{
	#include "uart.h"
}
#include <stdio.h>
#include <stdlib.h>
#include "cc1101.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED      PINB1

CC1101 radio;
CCPACKET rx_packet;
CCPACKET tx_packet;
bool new_rx_packet = false;


#define PKTLEN (10)
#define START_OF_DATA_OFFSET (1)
#define LEN_OF_DATA (74)

//#define enableIRQ_GDO0()          ::attachInterrupt(0, radioISR, FALLING);
//#define disableIRQ_GDO0()         ::detachInterrupt(0);

#define enableIRQ_GDO0()          EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (2 << ISC00); EIMSK |= (1 << INT0);
#define disableIRQ_GDO0()         EIMSK &= ~(1 << INT0);

ISR(INT0_vect)
{
  // Disable interrupt
  disableIRQ_GDO0();

  if (radio.rfState == RFSTATE_RX)
  {

    // Any packet waiting to be read?
    if (radio.receiveData(&rx_packet) > 0)
    {
		new_rx_packet = true;
		//printf(" GOT A PACKET!!!!!!!!!!!!!!!!!!\n");
    }
  }

  // Enable interrupt
  enableIRQ_GDO0();
}



/* Take two bits and convert it to a manchester value */
uint8_t man_data_to_bin( uint8_t v0, uint8_t v1)
{
	if (( v0 == 1) && ( v1 == 0)) {
		return 1;
	}
	else if (( v0 == 0) && ( v1 == 1)) {
		return 0;
	}

	return 0xff;
}

uint64_t raw_packet_to_hex(uint8_t *packet)
{
	uint8_t v0, v1, i;
	uint64_t hex_packet = 0;

	for (i = START_OF_DATA_OFFSET; i < LEN_OF_DATA + START_OF_DATA_OFFSET; i+= 1) {
		v0 = (packet[i/8] >> (7 - (i % 8 ))) & 1;
		i += 1;
		v1 = (packet[i/8] >> (7 - (i % 8 ))) & 1;
		hex_packet = (hex_packet << 1) | man_data_to_bin(v0, v1);
	}

	return hex_packet;
}

struct tpms_packets {
   	uint8_t prepend;
	uint32_t address;
   	uint16_t pressure;
}; 

struct tpms_packets raw_packet_to_struct(uint8_t *packet)
{
	struct tpms_packets ret;
	uint64_t hex_pkt;

	//printf("packet = ");
	//for (int i = 0; i < 10 ; i+= 1) 
	//	printf("%02x ", packet[i]);
	//printf("\n");

	hex_pkt = raw_packet_to_hex(packet);

	ret.prepend = hex_pkt >> 34;
	ret.address = ((hex_pkt >> 10) & 0xffffff);
	ret.pressure = (hex_pkt & 0x3ff);

	printf("Got packet from addr = 0x%lx   prepend = %d,   pressure = %d\n", ret.address, ret.prepend, ret.pressure/2);

	return ret;
}

/* Turn a 64-bit value of the packet into an array of ints
 * This functions handles converting the binary data to manchester encoded
 * data.
 */
uint8_t * hex_to_packetdata(uint64_t hexp)
{
	uint8_t man_bit_0, man_bit_1, k, data_bit = 0;
	int i;
	static uint8_t data[PKTLEN] = {0};

	for (i = 0; i < PKTLEN; i++)
		data[i] = 0;

	/* 73 should be a div round down or something */
	for (i = 73; i > 0; i-= 2) {
		k = i;
		data_bit = hexp & 1;
		hexp = hexp >> 1;

		if (data_bit) {
			man_bit_0 = 0;
			man_bit_1 = 1;
		} else {
			man_bit_0 = 1;
			man_bit_1 = 0;
		}

		data[k/8] |= (man_bit_1 << (7 - (k % 8)));
		k+=1;
		data[k/8] |= (man_bit_0 << (7 - (k % 8)));
	}

	return data;
}


/* Convert a tpms_packet into an array of ints */
uint8_t * struct_to_raw_packet( struct tpms_packets tppkt)
{
	uint64_t hex_pkt = 0;
	hex_pkt |= (uint64_t)(tppkt.prepend) << 34;
	hex_pkt |= (uint64_t)(tppkt.address) << 10;
	hex_pkt |= tppkt.pressure;
	return hex_to_packetdata(hex_pkt);
}




void transmit_packet(struct tpms_packets packet)
{
	printf("Sending fake transmission to %lx\n", packet.address);
	uint8_t *txp = struct_to_raw_packet(packet);

	/* Fill in the wake pulse + timing pulses + wake pulse by hand */
	tx_packet.data[0] = 0x07;
	tx_packet.data[1] = 0xAA;
	tx_packet.data[2] = 0xAA;
	tx_packet.data[3] = 0xAA;
	tx_packet.data[4] = 0xAF;

	for ( int k = 0; k < PKTLEN; k++)
		tx_packet.data[5+k] = *txp++;
	tx_packet.length = 15;

	/* When I recorded some data I found that it sends 8 packets in this pattern
	 * pkt,85ms,pkt,135ms,pkt,185ms,pkt,135ms,pkt,135ms,pkt,85ms,pkt,85ms,pkt
	 * So just replicate that
	 */
	//LED_PORT |= _BV(LED);
	radio.sendData(tx_packet);
//	_delay_ms(85);
//	radio.sendData(tx_packet);
//	_delay_ms(135);
//	radio.sendData(tx_packet);
//	_delay_ms(185);
//	radio.sendData(tx_packet);
//	_delay_ms(135);
//	radio.sendData(tx_packet);
//	_delay_ms(135);
//	radio.sendData(tx_packet);
//	_delay_ms(85);
//	radio.sendData(tx_packet);
//	_delay_ms(85);
//	radio.sendData(tx_packet);
}



uint32_t car_programmed_addrs[] = {0x6CE1E6, 0x6CC3ED, 0x6CDCE5, 0x6CC2DF};
//uint32_t summer_addrs[] = {0x6484c, 0x64877, 0x66b7b, 0x6a554};
uint32_t summer_addrs[] =         {0x000000, 0x000000, 0x795AAB, 0x000000};

void loop() {

	if (new_rx_packet) {
		new_rx_packet = false;
		LED_PORT ^= _BV(LED);
		struct tpms_packets tpms_packet = raw_packet_to_struct(rx_packet.data);
		int i;
		for (i = 0; i<4; i++) {
			if (tpms_packet.address == summer_addrs[i])
			{
				/* Send a spoofed packet out */
				tpms_packet.address = car_programmed_addrs[i];
				tpms_packet.pressure = 20*20;
				transmit_packet(tpms_packet);
				break;
			}
		}

		if( i == 4 )
			printf("Got Bogus packet from 0x%lx\n", tpms_packet.address);
	}

}

int main(void) {
	uart_init();
	stdin = stdout = fdevopen(uart_putchar, uart_getchar);

	/* set LED pin as output */
	LED_DDR |= _BV(LED);
	LED_PORT &= ~_BV(LED);

	radio.init();
	printf("Initializing Radio\n");

	_delay_us(50);
	radio.setRxState();

	enableIRQ_GDO0();
	sei();

	struct tpms_packets tpms_packet;
	tpms_packet.address = car_programmed_addrs[2];
	tpms_packet.prepend = 5;
	tpms_packet.pressure = 36*20;
	transmit_packet(tpms_packet);

	while (1)
	{};
		loop();
	return 0;
}
