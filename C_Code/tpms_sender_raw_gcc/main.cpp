extern "C"
{
	#include "uart.h"
}
#include <stdio.h>
#include "cc1101.h"
#include <avr/io.h>
#include <util/delay.h>

#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED      PINB5
//LED_PORT ^= _BV(LED);

CC1101 radio;
CCPACKET rx_packet;
CCPACKET tx_packet;


#define LEN_OF_PACKET (10)
#define START_OF_DATA_OFFSET (1)
#define LEN_OF_DATA (74)

uint32_t fake_addrs[] = {0x931e19, 0x933c12, 0x93231a, 0x933d20};  // Winter
//uint32_t actual_addrs[] = {0x6484c, 0x64877, 0x66b7b, 0x6a554}; // Summer


/* Take two bits and convert it to a manchester value */
uint8_t man_data_to_bin( uint8_t v0, uint8_t v1)
{
	if (( v0 == 1) && ( v1 == 0)) {
		return 0;
	}
	else if (( v0 == 0) && ( v1 == 1)) {
		return 1;
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

uint8_t * hex_to_packetdata(uint64_t hexp)
{
	uint8_t v0, v1, k, d = 0;
	int i;
	static uint8_t data[10] = {0};

	for (i = 0; i < 10; i++)
		data[i] = 0;

	/* 73 should be a div round down or something */
	for (i = 73; i > 0; i-= 2) {
		k = i;
		d = hexp & 1;
		hexp = hexp >> 1;

		if (d) {
			v0 = 1;
			v1 = 0;
		} else {
			v0 = 0;
			v1 = 1;
		}

		data[k/8] |= (v1 << (7 - (k % 8)));
		k+=1;
		data[k/8] |= (v0 << (7 - (k % 8)));
	}

	return data;
}


uint8_t * struct_to_raw_packet( struct tpms_packets tppkt)
{
	uint64_t hex_pkt = 0;
	hex_pkt |= (uint64_t)(tppkt.prepend) << 34;
	hex_pkt |= (uint64_t)(tppkt.address) << 10;
	hex_pkt |= tppkt.pressure;


	return hex_to_packetdata(hex_pkt);
}









void loop() {

	int i, j;
	uint8_t preps[9] = {2, 0,0,0,0,0,0,0};

	for (j=0; j<8; j++){

	for (i=0; i<4; i++){
		//puts("Push key to send packets...\n");
		//getchar();
		printf("Sending packets to 0x%lx\n", fake_addrs[i]);
		struct tpms_packets tpms_packet;
		tpms_packet.prepend=preps[j];
		tpms_packet.address=fake_addrs[i];
		tpms_packet.pressure=507;
		//tpms_packet.pressure=305;
	
		uint8_t *txp = struct_to_raw_packet(tpms_packet);

		/* Shut off sending of the preamble and syncwords, just send them by "hand" */
		tx_packet.data[0] = 0x07;
		tx_packet.data[1] = 0xAA;
		tx_packet.data[2] = 0xAA;
		tx_packet.data[3] = 0xAA;
		tx_packet.data[4] = 0xAF;

		for ( int k = 0; k < 10; k++)
			tx_packet.data[5+k] = *txp++;
		tx_packet.length = 15;

		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(186);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(135);
		radio.sendData(tx_packet);
		_delay_ms(85);
		radio.sendData(tx_packet);
		_delay_ms(200);
	}
	_delay_ms(500);
	}
}

int main(void) {
	uart_init();
	stdin = stdout = fdevopen(uart_putchar, uart_getchar);

	/* set LED pin as output */
	LED_DDR |= _BV(LED);
	LED_PORT &= ~_BV(LED);

	puts("Hello world!\n");
	//getchar();

	radio.init();
	puts("Initializing Radio\n");
	puts("Starting on button press");

	getchar();

	while (1)
		loop();
	return 0;
}
