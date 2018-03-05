TARGET = atmega328
MCU_TARGET = atmega328p
AVR_FREQ = 8000000L 

COMP_PATH =

CC         = $(COMP_PATH)avr-gcc
CXX        = $(COMP_PATH)avr-gcc
OBJCOPY        = $(COMP_PATH)avr-objcopy
OBJDUMP        = $(COMP_PATH)avr-objdump
SIZE           = $(COMP_PATH)avr-size

OPTIMIZE   = -Os
CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)
CFLAGS += -fno-common -ffunction-sections -fdata-sections
CPPFLAGS = $(CFLAGS)
LDFLAGS       = -Wl,--gc-sections

SRCS ?= main.o uart.o simplespi.o cc1101.o
PROG := $(firstword $(SRCS:.c=))
OBJ := $(SRCS:.c=.o) $(SRCS:.cpp=.o)


all: $(PROG).elf $(PROG).hex $(PROG).lst

%.o: %.c
		$(CC) $(CPPFLAGS) -c $<

%.o: %.cpp
		$(CXX) $(CPPFLAGS) -c $<

%.elf: $(OBJ)
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
			for x in *.o; do $(SIZE) $$x | tail -1; done
	$(SIZE) $@

clean:
		rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex

%.lst: %.elf
		$(OBJDUMP) -h -S $< > $@

%.hex: %.elf
		$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
		$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
		$(OBJCOPY) -j .text -j .data -O binary $< $@

prog: all
		avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -b 57600 -v -Uflash:w:$(PROG).hex:i

win_prog: all
	  wrun /mnt/c/Program\ Files\ \(x86\)/Arduino/hardware/tools/avr/bin/avrdude.exe '-C "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -c arduino -p atmega328p -P COM4 -b 57600 -v -Uflash:w:$(PROG).hex:i'


terminal:
	screen /dev/ttyUSB0 57600
