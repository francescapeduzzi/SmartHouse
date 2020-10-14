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
 
TempPacket t;
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

void temp(void) {
	uint8_t valoretemp= adc_read();
	float voltage= (valoretemp/1024.0f)*5.0f;
  float temperature=(voltage-0.5f)*100.0f;
  temperatura= (round(temperature * 10) / 10.0f);
	//temperatura = adc_read();
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
	DigIO_init();
	PWM_init();

	adc_init();
	uart= UART_init(0, 115200);
	PacketHandler_initialize(&packet_handler);
	PacketHandler_installPacket(&packet_handler, &ledPacket_ops);
	//temperatura
	PacketHandler_installPacket(&packet_handler, &tempPacket_ops);
	
	
	while(1){
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
	}
	PacketHandler_uninstallPacket(&packet_handler, LED_PACKET_TYPE);
	PacketHandler_uninstallPacket(&packet_handler, TEMP_PACKET_TYPE);
	
	return 0;
}	
