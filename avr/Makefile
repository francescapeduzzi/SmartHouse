
PREFIX=../
ARCH=arch/atmega2560/
CC=avr-gcc
INCLUDE_DIRS=-I$(PREFIX)/src/common -I$(PREFIX)/src/arch/include -I$(PREFIX)/src/host
CC_OPTS=-Wall -std=gnu99 -DF_CPU=16000000UL    -O3 -funsigned-char -funsigned-bitfields  -fshort-enums -Wall -Wstrict-prototypes -mmcu=atmega2560 -D_ARDUINO_MEGA_2560_ $(INCLUDE_DIRS) 


AVRDUDE=avrdude

# com1 = serial port. Use lpt1 to connect to parallel port.
AVRDUDE_PORT = /dev/ttyACM0    # programmer connected to serial device

AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET):i
AVRDUDE_FLAGS = -p m2560 -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -b 19200
AVRDUDE_FLAGS += $(AVRDUDE_NO_VERIFY)
AVRDUDE_FLAGS += $(AVRDUDE_VERBOSE)
AVRDUDE_FLAGS += $(AVRDUDE_ERASE_COUNTER)
AVRDUDE_FLAGS += -D -q -V -C /usr/share/arduino/hardware/tools/avr/../avrdude.conf
AVRDUDE_FLAGS += -c wiring


OBJS=	pins.o\
    	uart.o\
    	eeprom.o\
     	delay.o\
     	digio.o\
	pwm.o\
	packet_handler.o\
	adc.o\
    
HEADERS= packet_handler.h\
	 packet_header.h\
	 packet_operations.h\
	 delay.h\
	 digio.h\
	 eeprom.h\
	 pins.h\
	 uart.h\
	 pwm.h\
	 adc.h\
	 
BINS= main.elf\

.phony:	clean all

all:	$(BINS)

#common objects
%.o:	$(PREFIX)/src/common/%.c 
	$(CC) $(CC_OPTS) -c  $<

%.o:	$(PREFIX)/src/$(ARCH)/%.c 
	$(CC) $(CC_OPTS) -c  $<

#orazio server objects
%.o:	$(PREFIX)/src/main/%.c 
	$(CC) $(CC_OPTS) -c  $<

%.elf:	%.o $(OBJS)
	$(CC) $(CC_OPTS) -o $@ $< $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) $(BINS) *.hex *~ *.o

%.hex:	%.elf
	avr-objcopy -O ihex -R .eeprom $< $@
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$@:i 
	rm $@
