#include <avr/sleep.h>

#include "LisNet.h"
RF24 radio(7,8);
LisNet r(&radio);
void check_radio(void){ r.int0(); }

void onReadPacket( headerPacket_t * h, uint8_t p );




int serial_putc( char c, FILE * ) {Serial.write( c );return c;} 
void printf_begin(void){fdevopen( &serial_putc, 0 );}



void setup() {
  
  Serial.begin(115200);
  Serial.print("\n\nKotCHIP writed : ");Serial.print(F(__DATE__));Serial.print("  ");Serial.print(F(__TIME__));Serial.print("  ");Serial.println(F(__VERSION__));

  printf_begin();


  randomSeed(analogRead(0));
  radio.begin();

  radio.setPALevel(RF24_PA_MAX);

  r.setNetwork( "LIS", 10, 1 );
  r.onReceivePacket(onReadPacket);

  radio.printDetails();
  attachInterrupt(0, check_radio, FALLING);
  radio.startListening();
}




typedef struct{
  float     temperature;
  float     humidity;  
  int32_t   pressure;
  float     vBat;
  uint8_t   fail;
}Pack_t;



Pack_t msg;

void loop() {
  _delay_ms( 10 );
  
  r.service();
}

void onReadPacket( headerPacket_t * h, uint8_t p ){
  if(  h->port == 100  ){
    Pack_t msg;
    memset( &msg, 0, sizeof( msg ) );
    
    r.get( msg );

    Serial.print("v ");
    Serial.println(msg.vBat);
    Serial.print("t ");
    Serial.print(msg.temperature);Serial.println(" C");
    Serial.print("h ");
    Serial.print(msg.humidity);Serial.println(" %");
    Serial.print("p ");
    Serial.print(msg.pressure);Serial.println(" Pa");
    
    return;
  }
}




uint8_t sleep_cycles_remaining;
void sleepDelay(){
  
  
  ADCSRA &= ~( 1<<7 );// disable ADC
  
  
  sleep_cycles_remaining = 1;

  

  setup_watchdog(9);
  while(sleep_cycles_remaining)system_sleep();
  stop_watchdog();
  
  ADCSRA |=  1<<7 ;// enable ADC
  
}

void system_sleep() {
/*
SLEEP_MODE_IDLE: 15 mA
SLEEP_MODE_ADC: 6.5 mA
SLEEP_MODE_PWR_SAVE: 1.62 mA
SLEEP_MODE_EXT_STANDBY: 1.62 mA
SLEEP_MODE_STANDBY : 0.84 mA
SLEEP_MODE_PWR_DOWN : 0.36 mA
*/
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System actually sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out
}


// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

void stop_watchdog(){
  WDTCSR&= ~_BV(WDCE);
  WDTCSR&= ~_BV(WDE);
}

ISR(WDT_vect) {
  --sleep_cycles_remaining;
}
