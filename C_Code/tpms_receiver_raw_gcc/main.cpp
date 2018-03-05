extern "C"
{
	#include "uart.h"
}
#include <stdio.h>
#include "cc1101.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED      PINB1

CC1101 radio;
CCPACKET rx_packet;
bool new_rx_packet = false;
CCPACKET tx_packet;


#define LEN_OF_PACKET (10)
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
		printf(" GOT A PACKET!!!!!!!!!!!!!!!!!!\n");
    }
  }

  // Enable interrupt
  enableIRQ_GDO0();
}



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

struct tpms_packets raw_packet_to_struct(uint8_t *packet)
{
	struct tpms_packets ret;
	uint64_t hex_pkt;

	printf("packet = ");
	for (int i = 0; i < 10 ; i+= 1) 
		printf("%02x ", packet[i]);
	printf("\n");

	hex_pkt = raw_packet_to_hex(packet);

	ret.prepend = hex_pkt >> 34;
	ret.address = ((hex_pkt >> 10) & 0xffffff);
	ret.pressure = (hex_pkt & 0x3ff);

	printf("Got packet from addr = 0x%lx   prepend = %d,   pressure = %d\n", ret.address, ret.prepend, ret.pressure);

	return ret;
}










void loop() {
	uint8_t i;

	if (new_rx_packet) {
		new_rx_packet = false;

		LED_PORT ^= _BV(LED);

		printf(" rx_packet = ");
		for (i=0; i<rx_packet.length; i++)
			printf("%02x ", rx_packet.data[i]);
		printf("\n");

		struct tpms_packets tpms_packet = raw_packet_to_struct(rx_packet.data);
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


	while (1)
		loop();
	return 0;
}
