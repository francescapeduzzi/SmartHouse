#include "packet_handler.h"
#include "packet_header.h"
#include "serial_linux.h"
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
int interrupt_occured = 0;
char nomeStanza1[20]; 
char nomeStanza2[20]; 
char nomeStanza3[20]; 
char nomeStanza4[20]; 

//static struct UART* uart;
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
	float temp_val;
	uint8_t chi;	
} TempPacket;
#pragma pack(pop) 

#define TEMP_PACKET_TYPE 1
#define TEMP_PACKET_SIZE (sizeof(TempPacket)) 

TempPacket temp_packet_buffer;

PacketHeader* TempPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != TEMP_PACKET_TYPE || size != TEMP_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &temp_packet_buffer;
}

PacketStatus TempPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	
	TempPacket* te= (TempPacket*) header;
	if(te->chi==1){
		printf("La temperatura è di: %.1f°C\n", te->temp_val);
	}
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

//valore pin digitale
#pragma pack(push,1)
typedef struct DigitalStatusPacket {
	PacketHeader header;
	uint8_t pin;
	uint8_t status;	
} DigitalStatusPacket;
#pragma pack(pop) 

#define DIGITALSTATUS_PACKET_TYPE 2
#define DIGITALSTATUS_PACKET_SIZE (sizeof(DigitalStatusPacket))

DigitalStatusPacket digital_buffer;

PacketHeader* DigitalStatusPacket_initializeBuffer(PacketType type, PacketSize size, void* args __attribute__((unused))) {
	if(type != DIGITALSTATUS_PACKET_TYPE || size != DIGITALSTATUS_PACKET_SIZE)
		return 0;
	return (PacketHeader*) &digital_buffer;
}

PacketStatus DigitalStatusPacket_onReceive(PacketHeader* header, void* args __attribute__((unused))){
	DigitalStatusPacket* d = (DigitalStatusPacket*) header;
	char camera[20];
	if (d->pin == 13 ) { 
		strcpy(camera, nomeStanza1);
	} 
	if (d->pin == 10 ) { 
		strcpy(camera, nomeStanza2);
	} 
	if (d->pin == 9 ) { 
		strcpy(camera, nomeStanza3);
	} 
	if (d->pin == 6 ) { 
		strcpy(camera, nomeStanza4);
	} 
	if (d->status == 1 ) {
		printf("Luce %s accesa!\n", camera);
	}
	else {
		printf("Luce %s spenta!\n", camera);
	}
	return Success;

}

PacketOperations DigitalStatusPacket_ops = {
	DIGITALSTATUS_PACKET_TYPE,
	sizeof(DigitalStatusPacket),
	DigitalStatusPacket_initializeBuffer,
	0,
	DigitalStatusPacket_onReceive,
	0
}; 
//fine pacchetto DigitalStatus


void flushOutputBuffer(int fd) {
	while(packet_handler.tx_size){
		uint8_t c = PacketHandler_txByte(&packet_handler);
		write(fd, &c, 1);
		usleep(1000);
	}
}

void flushInputBuffer( int fd) {
	volatile int packet_complete =0;
		while ( !packet_complete ) 
		{
			uint8_t c;
			int n=read (fd, &c, 1);
			if (n) 
			{
				PacketStatus status = PacketHandler_rxByte(&packet_handler, c);
				#ifdef DEBUG_ERR_COMUNICATION
				if (status<0)
				{	printf("%d",status);
					fflush(stdout);
				}
				#endif
				packet_complete = (status==SyncChecksum);
			}
		}		
}


void configura (void) {
	printf("Inizio configurazione:\n");
	printf("Inserisci il nome della prima stanza\n");
	scanf("%s", nomeStanza1);
	printf("Inserisci il nome della seconda stanza\n");
	scanf("%s", nomeStanza2);
	printf("Inserisci il nome della terza stanza\n");
	scanf("%s", nomeStanza3);
	printf("Inserisci il nome della quarta stanza\n");
	scanf("%s", nomeStanza4);
	printf("Le tue stanze sono >> 1°:%s, 2°:%s, 3°:%s, 4°:%s\n", nomeStanza1, nomeStanza2, nomeStanza3, nomeStanza4);
}

int main(int argc, char** argv){
	if (argc!=2){
		return -1;
	}
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
	PacketHandler_installPacket(&packet_handler, &DigitalStatusPacket_ops);
	
	configura();

	while (1) {
			printf("Cosa vuoi controllare? [luce, gradi, digital], [esc] per uscire\n");
			scanf("%s", &cmd);
			if (!strcmp(cmd, "gradi")){
				TempPacket t={ {TEMP_PACKET_TYPE, TEMP_PACKET_SIZE, 0}, 0, 0};
				PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &t);
				
				flushOutputBuffer(fd);
				usleep(50000);
				flushInputBuffer(fd);
			}
			else if (!strcmp(cmd, "luce")){
				printf("Quale stanza vuoi controllare? [%s, %s, %s, %s]\n",nomeStanza1, nomeStanza2, nomeStanza3, nomeStanza4);
				scanf("%s", &cmd); 
				uint8_t stanza=0;
				int continua = 1;
				if (!strcmp(cmd, nomeStanza1)){
					stanza = 13;	
				} else if (!strcmp(cmd, nomeStanza2)){
					stanza = 10;	
				} else if (!strcmp(cmd, nomeStanza3)){
					stanza = 9;	
				} else if (!strcmp(cmd, nomeStanza4)){
					stanza = 6; 
				}
				else {
					printf("Stanza non trovata!\n");
					continua = 0;
				}
				if (continua) {
					printf("Vuoi accendere o spegnere la luce in %s? [accendi/spegni]\n", cmd);
					scanf("%s", &cmd1);
					uint8_t on_off;
					int ok=0;
					if (!strcmp(cmd1, "spegni")){
						printf("Mi hai detto: %s luce in %s\n", cmd1, cmd);
						on_off = ledOff;
						ok=1;
					}
					else if(!strcmp(cmd1, "accendi")) {
						printf("Mi hai detto: %s luce in %s\n", cmd1, cmd);
						on_off = ledOn;
						ok=1;
					}
					else {
						printf("comando inserito sbagliato!!\n");
					}
					if(ok==1){
					LedPacket p0 = { {LED_PACKET_TYPE, LED_PACKET_SIZE, 0}, stanza, on_off};
					PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &p0);
					flushOutputBuffer(fd);
					}
				}	
			}	
			else if (!strcmp(cmd, "digital")){
				printf("Di quale stanza vuoi sapere lo stato? [%s,%s,%s,%s]\n", nomeStanza1, nomeStanza2, nomeStanza3, nomeStanza4);
				scanf("%s", &cmd);
				uint8_t pinDigitale; 
				if (!strcmp(cmd, nomeStanza1)){
					pinDigitale = 13;	
				} else if (!strcmp(cmd, nomeStanza2)){
					pinDigitale = 10;
				} else if (!strcmp(cmd, nomeStanza3)){
					pinDigitale = 9;
				} else if (!strcmp(cmd, nomeStanza4)){
					pinDigitale = 6;	
				} else {
					return;
				}
				uint8_t statusPin = 0;
				DigitalStatusPacket d0 = { {DIGITALSTATUS_PACKET_TYPE, DIGITALSTATUS_PACKET_SIZE, 0}, pinDigitale, statusPin};
				printf("pinDigitale richiesto: %d\n", pinDigitale);
				PacketHandler_sendPacket(&packet_handler, (PacketHeader*) &d0);
				flushOutputBuffer(fd);
				usleep(50000);
				flushInputBuffer(fd);
			}
			else if (!strcmp(cmd, "esc")){
				break;
			}
			else {
				printf("Comando non trovato, dimmi cosa vuoi fare...\n");	
			}
	}		
	close(fd);	
	return 0;
}
