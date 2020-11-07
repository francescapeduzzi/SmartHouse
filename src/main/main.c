#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "digio.h"
#include "eeprom.h"
#include "packet_handler.h"
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "delay.h"
#include "pwm.h"
#include "adc.h"
#include <math.h>

//#include "serial_linux.h"
static struct UART* uart;
static PacketHandler packet_handler; 
float temperatura;
uint8_t pinDigitale;
uint8_t statusPin;
int controllo= 0;
volatile uint8_t interrupt_occurred=0;
volatile int reset=1;
int button = 21;
int piezo = 4;
int valo=0;

ISR(INT0_vect){
		interrupt_occurred=1;
}

void flushInputBuffer(void){
	while(UART_rxBufferFull(uart)){
		uint8_t c = UART_getChar(uart);
		PacketHandler_rxByte(&packet_handler, c);
	}
}

int flushOutputBuffers(void)
{
	while (packet_handler.tx_size)
		UART_putChar(uart, PacketHandler_txByte(&packet_handler));
	return packet_handler.tx_size;
}

void ledOn(uint8_t pin){
  PWM_enable(pin, 0);
  DigIO_setDirection(pin, Output);
  DigIO_setValue(pin, 1);
}

void ledOff(uint8_t pin){
  PWM_enable(pin, 0);
  DigIO_setDirection(pin, Output);
  DigIO_setValue(pin, 0);
}

#pragma pack(push,1)
typedef struct LedPacket {
	PacketHeader header;
	uint8_t stanza;	//uint8_t o uint16_t
	uint8_t on_off;	//uint8_t o uint16_t
} LedPacket;
#pragma pack(pop) 

#define LED_PACKET_TYPE 0
#define LED_PACKET_SIZE (sizeof(LedPacket))


//temperatura
#pragma pack(push,1)
typedef struct TempPacket {
	PacketHeader header;
	uint8_t temp_val;	
} TempPacket;
#pragma pack(pop) 

#define TEMP_PACKET_TYPE 1
#define TEMP_PACKET_SIZE (sizeof(TempPacket)) 

//valore pin digitale
#pragma pack(push,1)
typedef struct DigitalStatusPacket {
	PacketHeader header;
	uint8_t pin;
	uint8_t status;	//uint8_t o uint16_t
} DigitalStatusPacket;
#pragma pack(pop) 

#define DIGITALSTATUS_PACKET_TYPE 2
#define DIGITALSTATUS_PACKET_SIZE (sizeof(DigitalStatusPacket))

//TempPacket t;
DigitalStatusPacket digital_buffer;
LedPacket led_packet_buffer;
//temperatura 
TempPacket temp_packet_buffer;

PacketHeader* LedPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != LED_PACKET_TYPE || size != LED_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &led_packet_buffer;
}
PacketHeader* TempPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != TEMP_PACKET_TYPE || size != TEMP_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &temp_packet_buffer;
}

PacketHeader* DigitalStatusPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != DIGITALSTATUS_PACKET_TYPE || size != DIGITALSTATUS_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &digital_buffer;
}

void val (int pinD){
	pinDigitale = pinD;
  statusPin = DigIO_getValue(pinDigitale);
}

PacketStatus DigitalStatusPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	controllo = 1;
	++header->seq;
	DigitalStatusPacket* d = (DigitalStatusPacket*) header;	
	val(d->pin);
	return Success;

}

PacketStatus LedPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	LedPacket* l = (LedPacket*) header;
	int stanza = l->stanza;
	int on_off = l->on_off;
	if (l->on_off == 1){
		ledOn(stanza);
	}
	else if (l->on_off == 0){
		ledOff(l->stanza);
	}
	else {
		printf("Error\n");
	}
	//set_EEPROM_stanza(stanza);
	char s[2];
	sprintf(s, "%d", stanza);
	EEPROM_write(0, s, 1);
	char c[1];
	sprintf(c, "%d", on_off);
	EEPROM_write(16, c, 1);
	if (l->on_off == 1){
		ledOn(l->stanza);
	}
	else if (l->on_off == 0){
		ledOff(l->stanza);
	}
	else {
		printf("Error\n");
	}
	return Success;
}

PacketOperations ledPacket_ops = {
	LED_PACKET_TYPE,
	sizeof(LedPacket),
	LedPacket_initializeBuffer,
	0,
	LedPacket_onReceive,
	0
}; 

PacketOperations DigitalStatusPacket_ops = {
	DIGITALSTATUS_PACKET_TYPE,
	sizeof(DigitalStatusPacket),
	DigitalStatusPacket_initializeBuffer,
	0,
	DigitalStatusPacket_onReceive,
	0
}; 


void temp(void) {
	uint8_t valoretemp= adc_read();
	float voltage= (valoretemp/1024.0f)*5.0f;
  float temperature=(voltage-0.5f)*100.0f;
  temperatura= (round(temperature * 10) / 10.0f);
}
//temperatura 
PacketStatus TempPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
}


PacketOperations tempPacket_ops = {
	TEMP_PACKET_TYPE,
	sizeof(TempPacket),
	TempPacket_initializeBuffer,
	0,
	TempPacket_onReceive,
	0
};

int main(void){
	/*DigIO_init();
	DigIO_setDirection(button, Input); //PD21 input
	DigIO_setValue(button, 1);
	PWM_init();*/

	adc_init();
	uart= UART_init(0, 115200);

	

	DDRD=0x0; // all pins on port b set as input
  PORTD=0x1; // pull_up on port b

  // enable interrupt 0
  EIMSK |=1<<INT0;

  // trigger int0 on rising edge
  EICRA= 1<<ISC01 | 1<<ISC00;
  sei();
	DigIO_setDirection(piezo, Output);
	DigIO_setValue(piezo, 0);

	PacketHandler_initialize(&packet_handler);
	PacketHandler_installPacket(&packet_handler, &ledPacket_ops);
	//temperatura
	PacketHandler_installPacket(&packet_handler, &tempPacket_ops);
	PacketHandler_installPacket(&packet_handler, &DigitalStatusPacket_ops);
	
	while(1){
		while (! interrupt_occurred){
			DigIO_setValue(button, 1);		
			flushInputBuffer();	
			delayMs(10);
		
			temp();
		
			char tempc[5];
			dtostrf(temperatura, 2, 1, tempc );
			UDR0 ='I';
  		for(int i=0; i<5; i++) {
  			while ( !(UCSR0A & (1<<UDRE0)) );
    		UDR0 = tempc[i];
    		delayMs(50);}
			UDR0 ='F';
    	UDR0='\n';
			
			if (controllo){
				DigitalStatusPacket d0 = { {DIGITALSTATUS_PACKET_TYPE, DIGITALSTATUS_PACKET_SIZE, 0}, pinDigitale, statusPin};
				PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &d0);
				flushOutputBuffers();
				controllo = 0 ;
			}		
		}	
		interrupt_occurred=0;
    ledOn(10);
		ledOn(13);
		ledOn(9);
		ledOn(6);	
		delayMs(100);
		ledOff(10);
		ledOff(13);
		ledOff(9);
		ledOff(6);	
		DigIO_setValue(piezo, 1);
		delayMs(100);
		DigIO_setValue(piezo, 0);
		delayMs(50);
		DigIO_setValue(piezo, 1);
		delayMs(100);
		DigIO_setValue(piezo, 0);
	}
	
	PacketHandler_uninstallPacket(&packet_handler, LED_PACKET_TYPE);
	PacketHandler_uninstallPacket(&packet_handler, TEMP_PACKET_TYPE);
	PacketHandler_uninstallPacket(&packet_handler, DIGITALSTATUS_PACKET_TYPE);
	
	return 0;
}	
