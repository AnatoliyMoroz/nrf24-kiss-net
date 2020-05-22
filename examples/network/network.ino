#include "printf.h"

#include "LisNet.h"


RF24 radio(7,8);
LisNet r(&radio);

void success( uint8_t retryN ){
  printf("\n\rTX OK[ r %i]" , retryN );
 // r.clsSuccess();
}

void fail(){
  printf("\n\rTX fail " );
  //r.clsSuccess();
}

void onReadBroadcast( headerBroadcast_t * h, uint8_t p ){
  
  printf("\n\rRX cast[%i] %i.%i:%i .%i " , p, h->srcSub, h->srcIp, h->port, h->n );


  if(  r.isBusy() ){
    printf("\n\r _BUSY_" );
    return;
  }
  
  if(  h->port == 100  ){
    
    printf("\n\rRX 255 []" );
    
    r.stream( h->srcIp, 100 );
    r.transmit(5);    

    r.success( success );
    r.fail( fail );
    return;
  }
}

void onReadStream( headerStream_t * h, uint8_t p ){
  printf("\n\rRX strm[%i] xx.%i:%i .%i " , p, h->srcIp, h->port, h->n );
  if(  h->port == 255  ){
    return;
  }
}

void onReadPacket( headerPacket_t * h, uint8_t p ){
  
}

void check_radio(void){ r.int0(); }


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.print("\n\nKotCHIP writed : ");Serial.print(F(__DATE__));Serial.print("  ");Serial.print(F(__TIME__));Serial.print("  ");Serial.println(F(__VERSION__));

  randomSeed(analogRead(0));
  printf_begin();

  //======================================================
  //
  //  RF
  //------------------------------------------------------
  radio.begin();

  radio.setPALevel(RF24_PA_MAX);
  radio.enableDynamicPayloads();

  r.setNetwork( "LIS", 10, 10+random(64) );
  
  radio.printDetails();
  
  r.openPipe(2, 100 );
  
  r.onReceiveBroadcast( onReadBroadcast );
  r.onReceiveStream   ( onReadStream );
  
  attachInterrupt(0, check_radio, FALLING);
  //======================================================  
  r.broadcast( 255 );
  
  r.transmit();
  r.transmit();

}






unsigned long timeValue;
int tLen;
int dl=0;
void loop() {

  _delay_ms( 10 );
  r.service();
  if( dl < 1 ){
    dl = 250 + random(350);
    if( ! r.isBusy() ){
      r.broadcast( 50 );
      r.transmit();
      //r.transmit();
    }
  }else dl--;
  
  
  /*if( ! r.isBusy() ){
    r.stream( 100, 10 );
    r.transmit();
  }*/
  
  //timeValue = millis();
  
  //r.transmit();

  //tLen = millis( )- timeValue;
  //printf("(Tms=%i)",tLen);
}











