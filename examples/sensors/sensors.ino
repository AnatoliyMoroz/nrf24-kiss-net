#include <avr/sleep.h>
#include <avr/power.h>
#include "printf.h"
#include "LisNet.h"
RF24 radio(7,8);
LisNet r(&radio);

/*
  A0 output GND
  A1 vBat
*/

#include "sensors.h"
#include <DHT12.h>
DHT12 dht12;  

#include <BMP180.h>
/*

680 - 810 мм рт.
90644 107973
return (pressure / pow(1.0 - (float)trueAltitude / 44330, 5.255));
BMP180(resolution)

resolution:
BMP180_ULTRALOWPOWER - pressure oversampled 1 time  & power consumption 3μA
BMP180_STANDARD      - pressure oversampled 2 times & power consumption 5μA
BMP180_HIGHRES       - pressure oversampled 4 times & power consumption 7μA
BMP180_ULTRAHIGHRES  - pressure oversampled 8 times & power consumption 12μA, library default
*/
BMP180 myBMP(BMP180_ULTRAHIGHRES);


HTSens_t htSens;


void check_radio(void){ r.int0(); }

void setup() {
   
  Serial.begin(115200);
  Serial.print("\n\nKotCHIP writed : ");Serial.print(F(__DATE__));Serial.print("  ");Serial.print(F(__TIME__));Serial.print("  ");Serial.println(F(__VERSION__));

  printf_begin();
  
  randomSeed(analogRead(0));
  radio.begin();

  radio.setPALevel(RF24_PA_MAX);


  r.setNetwork( "LIS", 10, 51 );
  radio.printDetails();
  attachInterrupt(0, check_radio, FALLING);

  analogReference(INTERNAL);// 1.1 volts

 
  myBMP.begin(); 
}

void powerDown(){
  
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
  uint32_t t = millis();
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  ADCSRA |=  1<<7 ;// enable ADC
  dht12.begin();
  msg.temperature = myBMP.getTemperature();//htSens.temperature;
  
  msg.fail = htSens.fail;
  msg.pressure = myBMP.getPressure();

  msg.vBat = readVBat();
  readDHT();
  if( !htSens.fail ){
    msg.humidity = htSens.humidity;
    //msg.temperature += htSens.temperature;
    //msg.temperature /= 2;
  }
  ADCSRA &= ~( 1<<6 );// disable ADC
  pinMode(A0, INPUT);
  digitalWrite(A0, HIGH);
  t = millis() -t ;
  Serial.print(F("read T: ")); Serial.println(t);

  
  
 
  Serial.print(F("Pressure....: ")); Serial.println( msg.pressure );    

  if( ! htSens.fail ){
    Serial.print("humidity    :");
    Serial.println( htSens.humidity );
  
    Serial.print("temperature A:");
    Serial.println( msg.temperature );
    Serial.print("temperature B:");
    Serial.println( htSens.temperature );
  }
  
  Serial.print("v :");
  Serial.println(msg.vBat);

  r.send( 1, 100 );
  r.put(msg);
  r.transmit();
  r.waitTransmit();
  if(r.isConfirmed())Serial.print("@_");else Serial.print("._");
  
 
  radio.powerDown();
  Serial.flush();
  
  sleepDelay();  
  //delay(2000);
}


float readVBat(){
  int res = 0;

  
  for( byte n = 0 ; n <20 ; n++ ){
    if( n<10 ) analogRead( 1 );else{
      res += analogRead( 1 );
    }
  }

  float r = res/10;
  r /= 200;
  return r;  
}


void initSensors(){
  
}

void readDHT(){
  
  htSens.renew = 1;
  
  float t12 = dht12.readTemperature();
  float h12 = dht12.readHumidity();
  
  if (isnan(h12) || isnan(t12) ){

    htSens.humidity = 0;
    htSens.temperature = 0;
    htSens.fail = 1;
    return;
  }

  htSens.humidity = h12;
  htSens.temperature =  t12;
  htSens.fail = 0;

}



uint8_t sleep_cycles_remaining;

void sleepDelay(){  
  
  sleep_cycles_remaining = 32;//32;

  setup_watchdog(9);
  while(sleep_cycles_remaining){
    system_sleep();
    sleep_cycles_remaining--;
  }
  stop_watchdog();
  
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
  
}
