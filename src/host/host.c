#include "packet_handler.h"
#include "packet_header.h"
#include "serial_linux.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

char cmd[20];
char cmd1[20];
PacketHandler packet_handler;
int ledOff = 0;
int ledOn  = 1;
char temperatura[5];
static struct UART* uart;
/*------------------- LedPacket ------------*/

#pragma pack(push,1)
typedef struct LedPacket {
	PacketHeader header;
	uint8_t stanza;	
	uint8_t on_off;	
} LedPacket;
#pragma pack(pop) 

#define LED_PACKET_TYPE 0
#define LED_PACKET_SIZE (sizeof(LedPacket))

LedPacket led_packet_buffer;

PacketHeader* LedPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != LED_PACKET_TYPE || size != LED_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &led_packet_buffer;
}

PacketOperations ledPacket_ops = {
	LED_PACKET_TYPE,
	sizeof(LedPacket),
	LedPacket_initializeBuffer,
	0,
	0,
	0
};


// pacchetto temperatura

#pragma pack(push,1)
typedef struct TempPacket {
	PacketHeader header;
	uint8_t temp_val;	
} TempPacket;
#pragma pack(pop) 

#define TEMP_PACKET_TYPE 0
#define TEMP_PACKET_SIZE (sizeof(TempPacket)) 

TempPacket temp_packet_buffer;
PacketHeader* TempPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != TEMP_PACKET_TYPE || size != TEMP_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &temp_packet_buffer;
}

PacketStatus TempPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	++header->seq;
	TempPacket* t = (TempPacket*) header;
	memcpy(&t, header, header->size);
	int x = 1;
	return Success;
}

PacketOperations tempPacket_ops = {
	TEMP_PACKET_TYPE,
	sizeof(TempPacket),
	TempPacket_initializeBuffer,
	0,
	TempPacket_onReceive,
	0
};

// fine pacchetto temperatura

void flushOutputBuffer(int fd) {
	while(packet_handler.tx_size){
		uint8_t c = PacketHandler_txByte(&packet_handler);
		write(fd, &c, 1);
		usleep(1000);
	}
}

void flushInputBuffer( int fd) {
	
	for(int i=0; i<sizeof(temperatura); i++){
		read(fd,temperatura+i,1);	
		printf("temperatura_c[%d]= %c\n",i, temperatura[i]);	
  }	
}



int main(int argc, char** argv){
	assert(argc>1);
	int fd = serial_open(argv[1]);
	if(fd<0)
		return 0;
	if(serial_set_interface_attribs(fd, 115200, 0)<0)
		return 0;
	serial_set_blocking(fd, 1);
	if(!fd)
		return 0;

	PacketHandler_initialize(&packet_handler);
	PacketHandler_installPacket(&packet_handler, &ledPacket_ops);

	PacketHandler_installPacket(&packet_handler, &tempPacket_ops);
	TempPacket t;

	while (1) {
		printf("Cosa vuoi controllare? [luce, gradi]\n");
		scanf("%s", &cmd);
		if (!strcmp(cmd, "luce")){
			printf("Quale stanza vuoi controllare?\n");
			scanf("%s", &cmd); 
			uint8_t stanza=0;
			if (!strcmp(cmd, "cucina")){
				stanza = 13;	//int pin_13 =  cucina;
			} else if (!strcmp(cmd, "bagno")){
				stanza = 10;	//int pin_10 =  bagno;
			} else if (!strcmp(cmd, "salone")){
				stanza = 9;	//int pin_9 =  salone;
			} else {
				stanza = 6; //int pin_6 =  altro;
			}
			printf("Vuoi accendere o spegnere la luce in %s? [accendi/spegni]\n", cmd);
			scanf("%s", &cmd1);
			uint8_t on_off;
			if (!strcmp(cmd1, "spegni")){
				printf("Mi hai detto: %s luce in %s\n", cmd1, cmd);
				on_off = ledOff;
			}
			else if(!strcmp(cmd1, "accendi")) {
				printf("Mi hai detto: %s luce in %s\n", cmd1, cmd);
				on_off = ledOn;
			}
			else {
				on_off=0;
			}
			LedPacket p0 = { {LED_PACKET_TYPE, LED_PACKET_SIZE, 0}, stanza, on_off};
			printf("p0.stanza = %d\n", stanza);
			printf("p0.on_off = %d\n", on_off);
			PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p0);
			flushOutputBuffer(fd);
		}
		if (!strcmp(cmd, "gradi")){
			char x;
			tcflush(fd, TCIFLUSH);
			PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &t);
			flushOutputBuffer(fd);
			usleep(50000);
			//read(fd,&x,1);
			int ok =1;
			while ( ok){	
				read(fd,&x,1);	
				if (x == 'I'){
					ok=0;
					for(int i=0; i<5; i++){
						read(fd,&x,1);	
						if (x != "%-19s"){
							temperatura[i] = x;
						}
					}
				}
			}
			printf("La temperatura è di: %s°C\n", temperatura);	
			printf("\n");
		}
	}
		//printf("Comando non trovato, dimmi cosa vuoi fare...");	}		
		return 0;
}
