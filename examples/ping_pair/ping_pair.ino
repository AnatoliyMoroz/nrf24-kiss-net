#include "LisNet.h"
RF24 radio(7,8);
LisNet r(&radio);

unsigned long en = 0;

void check_radio(void){r.int0();}

void onReadStream( headerStream_t * h, uint8_t p );

byte role;


int serial_putc( char c, FILE * ) { Serial.write( c );return c;} 
void printf_begin(void){fdevopen( &serial_putc, 0 );}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\nKotCHIP writed : ");Serial.print(F(__DATE__));Serial.print("  ");Serial.print(F(__TIME__));Serial.print("  ");Serial.println(F(__VERSION__));

  printf_begin();
  radio.begin();

  role = 0; // 1 transmit    0 recive

  radio.setPALevel(RF24_PA_MAX);


  r.setNetwork( "LIS", 10, 100+role );
  r.onReceiveStream   ( onReadStream );
  r.onReceivePacket   ( onReadPacket );
  
  attachInterrupt(0, check_radio, FALLING);


}


void onReadPacket( headerPacket_t * h, uint8_t p ){
  if(  h->port == 100  ){
    Serial.println("PKG");
//    Serial.println( t );
    //r.response( h->srcIp, h->n );
    //r.stream( h->srcIp, 201 );
    //r.transmit();
    return;
  }
}


void onReadStream( headerStream_t * h, uint8_t p ){
  
  //printf("\n\rRX strm[%i] xx.%i:%i .%i " , p, h->srcIp, h->port, h->n );
  if(  h->port == 100  ){
    unsigned long t=0;
    r.get(t);

    Serial.print("Read: ");
    Serial.println( t );

    //delay(100);
    r.stream( h->srcIp, 200 );
    r.put(t);
    
    r.transmit();
    return;
  }
  if(  h->port == 200  ){
    unsigned long t=0;
    Serial.print("TTL: ");
    if(r.get(t)){
      
      Serial.println( micros()-t );
    }
    return;
  }

}

void loop() {
  Serial.println('.');
  
  if(role){
    unsigned long time = micros();
    Serial.print("Send: ");
    Serial.println( time );
    delay(1);
    
    r.stream( 100, 100 );
    r.put(time);
    r.transmit();
    //r.waitTransmit();

    /*
    r.send( 100, 201 );
    r.put(time);

    r.transmit();
    r.waitTransmit();
    
    if(r.isConfirmed())Serial.print("@");else Serial.print("_");
    */
  }
  
  delay(1000);
}


