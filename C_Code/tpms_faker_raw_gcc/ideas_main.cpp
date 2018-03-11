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
bool new_rx_packet = false;


#define LEN_OF_PACKET (10)
#define START_OF_DATA_OFFSET (1)
#define LEN_OF_DATA (74)

//#define enableIRQ_GDO0()          ::attachInterrupt(0, radioISR, FALLING);
//#define disableIRQ_GDO0()         ::detachInterrupt(0);


#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( ((a) * 1000L) / (F_CPU / 1000L) )
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)
uint32_t timer0_millis = 0;
uint32_t timer0_fract = 0;
uint32_t timer0_overflow_count = 0;
SIGNAL(TIMER0_OVF_vect)
{
    // copy these to local variables so they can be stored in registers
    // (volatile variables must be read from memory on every access)
    unsigned long m = timer0_millis;
    unsigned char f = timer0_fract;
 
    m += MILLIS_INC;
    f += FRACT_INC;
    if (f >= FRACT_MAX) {
        f -= FRACT_MAX;
        m += 1;
    }
 
    timer0_fract = f;
    timer0_millis = m;
    timer0_overflow_count++;
}
    
unsigned long millis()
{
    unsigned long m;
    uint8_t oldSREG = SREG;
 
    // disable interrupts while we read timer0_millis or we might get an
    // inconsistent value (e.g. in the middle of a write to timer0_millis)
    cli();
    m = timer0_millis;
    SREG = oldSREG;
 
    return m;
}











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

	//printf("Got packet from addr = 0x%lx   prepend = %d,   pressure = %d\n", ret.address, ret.prepend, ret.pressure/2);

	return ret;
}


uint32_t found_addrs[10] = {0};
uint8_t found_cnt[10] = {0};
uint8_t found = 0;

uint32_t last_addr = 0;


void learning() {
	uint32_t loop_cnt = 0;
	for (loop_cnt = 0; loop_cnt < 90000 ; loop_cnt++) {
		if (new_rx_packet) {
			new_rx_packet = false;
	
			LED_PORT ^= _BV(LED);
	
	//		printf(" rx_packet = ");
	//		for (i=0; i<rx_packet.length; i++)
	//			printf("%02x ", rx_packet.data[i]);
	//		printf("\n");
	
			struct tpms_packets tpms_packet = raw_packet_to_struct(rx_packet.data);
			/* This if makes sure we see the same address twice before using it.  This trims down on false packet reception */
			if (last_addr == tpms_packet.address) {
				int i;
				for (i = 0; i< found; i++) {
					if (tpms_packet.address == found_addrs[i]) {
						found_cnt[i] += 1;
						//printf("seen Address %lx   %d\n", tpms_packet.address, found_cnt[i]);
						break;
					}
				}
				if (i == found) {  // Not found in found list
					found_addrs[i] = tpms_packet.address;
					found_cnt[i] = 1;
					found++;
					printf("Adding Address %lx #%d\n", tpms_packet.address, found);
				}
			}
			last_addr = tpms_packet.address;
		}

		if ((loop_cnt % 2000) == 0) {
			printf("millis = %ld\n", millis());
			for(int i =0; i<found; i++)
				printf("%8lx", found_addrs[i]);
			printf("\n");
			for(int i =0; i<found; i++)
				printf("%8d", found_cnt[i]);
			printf("\n");

		}
		_delay_ms(1);
	}

	for (int i = 0; i < found; i++)
	{
		for (int j = 0; j < found; j++)
		{
			if (found_cnt[j] < found_cnt[i])
			{
				uint32_t tmp = found_cnt[i];
				found_cnt[i] = found_cnt[j];
				found_cnt[j] = tmp;
				tmp = found_addrs[i];
				found_addrs[i] = found_addrs[j];
				found_addrs[j] = tmp;
			}
		}
	}

printf("SORTED = \n");
	for(int i =0; i<found; i++)
		printf("%8lx", found_addrs[i]);
	printf("\n");
	for(int i =0; i<found; i++)
		printf("%8d", found_cnt[i]);
	printf("\n");



}

struct learner_struct{
   	uint16_t count;
	uint32_t address;
   	uint64_t last_seen;
}; 
int cnt_compare(const void *s1, const void *s2)
{
struct learner_struct *e1 = (struct learner_struct *)s1;
struct learner_struct *e2 = (struct learner_struct *)s2;
return e2->count - e1->count;
}

struct learner_struct ZZZ[10];
uint8_t ZZZ_cnt = 0;
uint32_t last_print = 0;

void new_learning() {
	uint32_t st = millis();
	while (st + 30000 > millis())
	{
		if (new_rx_packet) {
			new_rx_packet = false;
			LED_PORT ^= _BV(LED);

			struct tpms_packets tpms_packet = raw_packet_to_struct(rx_packet.data);
			/* This if makes sure we see the same address twice before using it.  This trims down on false packet reception */
			if (last_addr == tpms_packet.address) {
				int i;
				for (i = 0; i<ZZZ_cnt; i++) {
					if (tpms_packet.address == ZZZ[i].address) {
						ZZZ[i].count += 1;
						qsort(ZZZ, 10, sizeof(struct learner_struct), cnt_compare);
						//printf("saw Address %lx   %d\n", tpms_packet.address, ZZZ[i].count);
						break;
					}
				}
				if (i == ZZZ_cnt) {  // Not found in found list
					if(i == 10)
						printf("NEED TO HANDLE THIS SITUATION\n");
					ZZZ[i].address = tpms_packet.address;
					ZZZ[i].count = 1;
					ZZZ[i].last_seen = millis();
					ZZZ_cnt += 1;
					qsort(ZZZ, 10, sizeof(struct learner_struct), cnt_compare);
					printf("Adding Address %lx #%d\n", tpms_packet.address, ZZZ_cnt);
				}
			}
			last_addr = tpms_packet.address;


		}

		if ((last_print + 2000) < millis()) {
			printf("millis = %ld\n", millis());
			last_print = millis();
			for(int i =0; i<10; i++)
				printf("%8lx", ZZZ[i].address);
			printf("\n");
			for(int i =0; i<10; i++)
				printf("%8d", ZZZ[i].count);
			printf("\n");
	
		}
	}

}









void loop() {
	new_learning();
	while (1) {};
}

int main(void) {
	uart_init();
	stdin = stdout = fdevopen(uart_putchar, uart_getchar);

	/* set LED pin as output */
	LED_DDR |= _BV(LED);
	LED_PORT &= ~_BV(LED);

	/* Setup millis timer */
	TCCR0B |= _BV(CS01);
	TCCR0B |= _BV(CS00);
	TIMSK0 |= _BV(TOIE0);

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
