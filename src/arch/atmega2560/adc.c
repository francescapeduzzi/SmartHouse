#include "adc.h"
#include <avr/io.h>


void adc_init(void) {
	ADMUX = (1<<REFS0); //default Ch-0; Vref = 5V
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);//ADCStatusRegister = Scelgo la porta da leggere
	ADCSRB = 0x00;
}

uint16_t adc_read(void){
  ADMUX &= ~(1 << MUX4) & ~(1 << MUX3);
  ADCSRA |= (1 << ADEN); //Enable la conversione
  ADCSRB &= ~(1 << MUX5);
  ADMUX &= ~(1 << MUX2) & ~(1 << MUX1) & ~(1 << MUX0);
  uint8_t mask = 0;
  mask |= (1 << ADSC); //inizia la conversione
  ADCSRA |= (1 << ADSC); 
  while (ADCSRA & mask); //while la conversione è terminata
  const uint8_t low = ADCL;
  const uint8_t high = ADCH;
  return (high << 8) | low;
}
