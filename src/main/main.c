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
#include "adc.h"
#include <math.h>

static struct UART* uart;
static PacketHandler packet_handler; 
float temperatura;
uint8_t pinDigitale;
uint8_t statusPin;
int controllo= 0;
int controltemp=0;
int controlloLed =0;
int eeprom_control =0;
volatile uint8_t interrupt_occurred;
volatile int reset=1;
int button = 21;
int piezo = 4;
char nomeStanza1[8]; 
char nomeStanza2[8]; 
char nomeStanza3[8]; 
char nomeStanza4[8];
int tmpStanza1[8];
int tmpStanza2[8];
int tmpStanza3[8];
int tmpStanza4[8];


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
  DigIO_setDirection(pin, Output);
  DigIO_setValue(pin, 1);
}

void ledOff(uint8_t pin){
  DigIO_setDirection(pin, Output);
  DigIO_setValue(pin, 0);
}

#pragma pack(push,1)
typedef struct LedPacket {
	PacketHeader header;
	uint8_t stanza;	
	uint8_t on_off;	
} LedPacket;
#pragma pack(pop) 

#define LED_PACKET_TYPE 0
#define LED_PACKET_SIZE (sizeof(LedPacket))


//temperatura
#pragma pack(push,1)
typedef struct TempPacket {
	PacketHeader header;
	float temp_val;
	uint8_t chi;	
} TempPacket;
#pragma pack(pop) 

#define TEMP_PACKET_TYPE 1
#define TEMP_PACKET_SIZE (sizeof(TempPacket)) 


#pragma pack(push,1)
typedef struct DigitalStatusPacket {
	PacketHeader header;
	uint8_t pin;
	uint8_t status;	
} DigitalStatusPacket;
#pragma pack(pop) 

#define DIGITALSTATUS_PACKET_TYPE 3
#define DIGITALSTATUS_PACKET_SIZE (sizeof(DigitalStatusPacket))

#pragma pack(push,1)
typedef struct EepromPacket {
	PacketHeader header;
	uint8_t mitt;	
	uint8_t stanza1[8];
	uint8_t stanza2[8];
	uint8_t stanza3[8];
	uint8_t stanza4[8];
} EepromPacket;
#pragma pack(pop) 

#define EEPROM_PACKET_TYPE 2
#define EEPROM_PACKET_SIZE (sizeof(EepromPacket))

DigitalStatusPacket digital_buffer;
LedPacket led_packet_buffer;
//temperatura 
TempPacket temp_packet_buffer;
EepromPacket eeprom_packet_buffer;

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

PacketHeader* EepromPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != EEPROM_PACKET_TYPE || size != EEPROM_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &eeprom_packet_buffer;
}

void val (int pinD){
	pinDigitale = pinD;
  statusPin = DigIO_getValue(pinDigitale);
}

PacketStatus DigitalStatusPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	controllo = 1;
	DigitalStatusPacket* d = (DigitalStatusPacket*) header;	
	val(d->pin);
	return Success;
}

PacketStatus TempPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	TempPacket* te= (TempPacket*) header;
	controltemp=1;
	return Success;
}


PacketStatus LedPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	LedPacket* l = (LedPacket*) header;
	int stanza = l->stanza;
	int on_off = l->on_off;
	if (on_off == 1){
		ledOn(stanza);
		if((DigIO_getValue(13)==1)&&(DigIO_getValue(10)==1)&&(DigIO_getValue(9)==1)&&(DigIO_getValue(6)==1)){
			controlloLed = 1;
		}
	}
	else if (on_off == 0){
		ledOff(l->stanza);
	}
	else {
		printf("Error\n");
	}
	
	return Success;
}

PacketStatus EepromPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	EepromPacket* e= (EepromPacket*) header;
	if(e->mitt== 1){	
		for(int i=0; i<8; i++){
			tmpStanza1[i]= e->stanza1[i];
			tmpStanza2[i]= e->stanza2[i];
			tmpStanza3[i]= e->stanza3[i];
			tmpStanza4[i]= e->stanza4[i];
			EEPROM_write((i*8), &tmpStanza1[i], sizeof(uint8_t));
			EEPROM_write((64+(i*8)), &tmpStanza2[i], sizeof(uint8_t));
			EEPROM_write((128+(i*8)), &tmpStanza3[i], sizeof(uint8_t));
			EEPROM_write((192+(i*8)),&tmpStanza4[i],sizeof(uint8_t));
		}
	}	
	else {
		eeprom_control = 1;
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
uint8_t	leggiDaEeprom(int i, int n){
	uint8_t eeprom_buffer[8];
	memset(eeprom_buffer, 0, 8);
	EEPROM_read(eeprom_buffer+i, (n + (8*i)), sizeof(uint8_t));     
	return eeprom_buffer[i];
}
void temp(void) {
	uint8_t valoretemp= adc_read();
	
	float voltage= (valoretemp/1024.0f)*5.0f;
        float temperature=(voltage-0.5f)*100.0f;
	temperatura= (round(temperature * 10) / 10.0f);
}


PacketOperations tempPacket_ops = {
	TEMP_PACKET_TYPE,
	sizeof(TempPacket),
	TempPacket_initializeBuffer,
	0,
	TempPacket_onReceive,
	0
};

PacketOperations eepromPacket_ops = {
	EEPROM_PACKET_TYPE,
	sizeof(EepromPacket),
	EepromPacket_initializeBuffer,
	0,
	EepromPacket_onReceive,
	0
};
void timerFn(void* args){
	ledOff(10);
	ledOff(13);
	ledOff(9);
	ledOff(6);	
}

int main(void){
	DigIO_init();
	DigIO_setDirection(button, Input); //Pin 21 input
	DigIO_setValue(button, 1);
	
	adc_init();
	uart= UART_init(0, 115200);

  // enable interrupt 0
  EIMSK |=1<<INT0;

  // trigger int0 on rising edge
  EICRA= 1<<ISC01 | 1<<ISC00;
	Timers_init();
  sei();
	DigIO_setDirection(piezo, Output);
	DigIO_setValue(piezo, 0);

	PacketHandler_initialize(&packet_handler);
	PacketHandler_installPacket(&packet_handler, &ledPacket_ops);
	//temperatura
	PacketHandler_installPacket(&packet_handler, &tempPacket_ops);
	PacketHandler_installPacket(&packet_handler, &DigitalStatusPacket_ops);
	PacketHandler_installPacket(&packet_handler, &eepromPacket_ops);
	
	interrupt_occurred=0;
	struct Timer* timerOff = Timer_create("timer_0",1000, timerFn, NULL);
	while(1){
		while (! interrupt_occurred){
			DigIO_setValue(button, 1);	
				
			flushInputBuffer();	
			delayMs(10);
			if(controlloLed){
				Timer_start(timerOff);
				controlloLed=0;
			}
			if(eeprom_control){
				EepromPacket e1 = {{EEPROM_PACKET_TYPE, EEPROM_PACKET_SIZE, 0}, 1, {0}, {0}, {0}, {0}};
				for(int i=0; i<8; i++){
					e1.stanza1[i] = leggiDaEeprom(i, 0);
					e1.stanza2[i] = leggiDaEeprom(i, 64);
					e1.stanza3[i] = leggiDaEeprom(i, 128);
					e1.stanza4[i] = leggiDaEeprom(i, 192);
				}
				PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &e1);
				flushOutputBuffers();
				
				eeprom_control = 0;
			}
			if(controltemp){
				temp();
				TempPacket t={ {TEMP_PACKET_TYPE, TEMP_PACKET_SIZE, 0}, temperatura, 1};
				PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &t);
				flushOutputBuffers();
				controltemp=0;		
			}
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
	PacketHandler_uninstallPacket(&packet_handler, EEPROM_PACKET_TYPE);
	
	return 0;
}	
