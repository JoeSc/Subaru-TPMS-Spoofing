extern "C"
{
	#include "uart.h"
}
#include <stdio.h>
#include "cc1101.h"
#include <avr/io.h>
#include <util/delay.h>


/* iterate through prepend list stopping on the last element */
#define PREPEND_LIST_LEN (2)
uint8_t prepend_list[PREPEND_LIST_LEN] = {5, 7};
uint8_t prepend_idx = 0;



#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED      PINB5

CC1101 radio;
CCPACKET tx_packet;

#define PKTLEN (10)
#define START_OF_DATA_OFFSET (1)
#define LEN_OF_DATA (74)

uint32_t winter_addrs[] = {0x6CE1E6, 0x6CC3ED, 0x6CDCE5, 0x6CC2DF};
//uint32_t summer_addrs[] = {0x795aab, ?0x799484?, ?0x79B788?, ?0x79B7B3?};

struct tpms_packets {
   	uint8_t prepend;
	uint32_t address;
   	uint16_t pressure;
}; 

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









void loop() {
	int i;

	for (i=0; i<4; i++){
		printf("Sending packets to 0x%lx\n", winter_addrs[i]);
		struct tpms_packets tpms_packet;
		tpms_packet.prepend=prepend_list[prepend_idx];
		tpms_packet.address=winter_addrs[i];
		//tpms_packet.pressure=718;
		tpms_packet.pressure=22 * 20; // Transmit 22 PSI
	
		uint8_t *txp = struct_to_raw_packet(tpms_packet);

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
		LED_PORT |= _BV(LED);
		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(185);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		LED_PORT &= ~_BV(LED);

		/* Give some delay between different tires */
		_delay_ms(2000);
	}
	_delay_ms(28000);

	/* Increment the prepend list if we are not on the last item */
	if( prepend_idx < (PREPEND_LIST_LEN - 1))
		prepend_idx++;
}

int main(void) {
	uart_init();
	stdin = stdout = fdevopen(uart_putchar, uart_getchar);

	/* set LED pin as output */
	LED_DDR |= _BV(LED);
	LED_PORT &= ~_BV(LED);

	puts("Hello world!\n");

	radio.init();
	puts("Initializing Radio\n");
	while(!uart_get_available() ) {
		puts("Starting on button press");
		_delay_ms(100);
	}
	getchar();

	while (1)
		loop();
	return 0;
}
