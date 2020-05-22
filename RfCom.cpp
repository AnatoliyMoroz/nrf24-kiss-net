
#include "RfCom.h"

//RfCom::RfCom(){

//}

RfCom::RfCom(RF24 *r){
  radio = r;
}

void RfCom::begin(){
    enabled = true;
}


bool RfCom::write(uint8_t type, uint8_t *data , uint8_t len, uint8_t resp=0){
    txPacketN++;
    pkHeader_t header;

    memset(&header, 0,sizeof(header));
    header.type = type;
    header.uid = random(32000);// crc(n, data) <<8 crc(n , group, node)

    clear_tx();
    if( put_tx( header ) ){
        if(! put_tx( data ,len )){
            printf("{blob ERR} \n\r");
        }

        return transmit(resp);
    }else{
        printf("{header ERR} \n\r");
    }


    return false;
}

bool RfCom::response(){
    pkHeader_t header;
    if( get_rx(0, header) ){
        clear_rx();
        return response(PK_HEADER, header);
    }
    return false;
}



bool RfCom::transmit(uint8_t resp=0){

    if( tx_len ){
        printf("(TX L=%i)",tx_len);
        radio->stopListening();

        radio->powerUp() ;
        nowWriting = true;
        radio->write( tx_buffer, tx_len );

        nowWriting = false;
        waitISR = true;
        if(resp)_delay_ms(1);
        radio->startListening();
        return true;

    }
    return false;
}

void RfCom::clear_tx(){
    tx_len = 0;
    memset( &tx_buffer, 0, sizeof(tx_buffer) );
}
void RfCom::clear_rx(){
    rx_len = 0;
    memset( &rx_buffer, 0, sizeof(rx_buffer) );
}

bool RfCom::put_tx( uint8_t *data, uint8_t len){

    if( tx_len+len > 32 ) return false;
    //printf("(put tx L=%i ",len);

    while (len--) {
        tx_buffer[tx_len] = *data++;
        //printf("%c.",tx_buffer[tx_len]);
        tx_len++;
    }
    //printf(" )");
    //printf(" tx_len=%i) ",tx_len);
    return true;
}

bool RfCom::get_rx(uint8_t offset, uint8_t *data, uint8_t len) {

  uint8_t pos;
  if( offset >= len || rx_len < len)return false;
  if( rx_len-offset < len )return false;

  while (len--) {
    *data = rx_buffer[pos+offset];
    *data++;
    pos++;
  }
  return true;
}
void RfCom::received(uint16_t pkUid){
    if(pkUid){
        rxPacketUid[rxPacketUidN] = pkUid;

        rxPacketUidN++;
        if( rxPacketUidN >= PK_RX_UID_LEN )rxPacketUidN=0;
    }
}
bool RfCom::isReceived(uint16_t pkUid){
    if(pkUid){
        uint8_t len = PK_RX_UID_LEN;

        while( len-- )
            if( rxPacketUid[len] == pkUid ){
                printf("{HavePK}");
                return true;
            }


        printf("{NewPK}");
        return false;
    }
}

void RfCom::int0(){
    if( busyISR ) return;
    if( !nowWriting ){
        busyISR = 1;
        uint8_t pipe_num;
        if ( enabled && radio->available(&pipe_num) ){
        //if( waitISR )printf("****BUG732******* ");

        bool done = false;

          while (!done)
          {
            rx_len = radio->getDynamicPayloadSize();
            done = radio->read( rx_buffer, rx_len );

            printf("(RX%i %i)",pipe_num,rx_len);

            pkHeader_t header;
            if(get_rx(0, header)){
                //if( !isReceived( header.uid ) ){
                //    received( header.uid );

                    if(header.type == PK_HEADER){printf("[Resp]");}
                    if(onReceiveFunction)onReceiveFunction(header,pipe_num);
                //}else{
                   // printf("[R]");
                //}
            };
          }
            radio->startListening();
      }else{
        if(waitISR){

            //printf(" (ISR OK) ");
            waitISR = false;

            //    RUN F ON READY

        }else{
            //printf(" *ISR unexpected* ");
        }

      }
    }else{
        if( waitISR ){
                waitISR = false;//
               // printf(" *ISR CLEAR* ");
            }else{
               // printf(" ***BUG454*** ");
            }
    }
    busyISR = 0;
}

