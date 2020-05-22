#include "LisNet.h"

LisNet::    LisNet  ( RF24 *r ){
    radio = r;
    sendRetry = 4;
    clear_tx();
}

void LisNet::    setNetwork  ( const char * domain, uint8_t sub, uint8_t ip ){

    addr_sub = sub;
    addr_ip  = ip;

    for( byte q = 0 ; q < 3; q++){
        net[q] = *domain++;

    }

    radio->openReadingPipe( 0, getAddr( 0, 0 ) );
    radio->openReadingPipe( 1, getAddr( sub, 0 ) );
    radio->openReadingPipe( 2, getAddr( sub, ip ) );
}


void LisNet::    broadcast( uint8_t port ) {
    repeatTx = 0;
    txPacketN++;
    reset_tx();

    put( (uint8_t)PK_BROADCAST );
    put( txPacketN );
    put( addr_ip );
    put( addr_sub );
    put( port );

    dst_sub     = 0;
    dst_ip      = 0;
    dst_confirm = 0;
}



uint8_t LisNet::    stream  ( uint8_t ip, uint8_t port ){
    repeatTx = 0;
    txPacketN++;
    reset_tx();

    put( (uint8_t)PK_STREAM );
    put( txPacketN );
    put( addr_ip );
    put( port );

    dst_sub     = addr_sub;
    dst_ip      = ip;
    dst_confirm = 0;

    return 0;
}




uint8_t LisNet::    send    ( uint8_t ip, uint8_t port ){


    if( waitConfirm ) return 0;
    repeatTx = 0;
    txPacketN++;
    reset_tx();

    put( (uint8_t)PK_COM );
    put( txPacketN );
    put( addr_ip );
    //put( ip );
    put( port );

    dst_sub     = addr_sub;
    dst_ip      = ip;
    dst_confirm = 1;
    dst_n       = txPacketN;
    dst_retry   = sendRetry;

    return 0;
}

void LisNet::    response    ( uint8_t ip, uint8_t n ){
    txPacketN++;
    repeatTx = 0;
    reset_tx();

    put( (uint8_t)PK_CK );
    put( n );
    put( addr_ip );
    put( 88 );

    dst_sub     = addr_sub;
    dst_ip      = ip;
    dst_confirm = 0;

    return ;
}

















        uint8_t  LisNet::    put( uint8_t v ){
            if( tx_len+1 >= 32 ) return false;
            tx_buffer[tx_len] = v;
            tx_len++;
            return true;
        }

        uint8_t  LisNet::    put(const uint8_t *data, uint8_t len){
            if( tx_len+len >= 32 ) return false;
            while (len--) {
                tx_buffer[tx_len] = *data++;
                tx_len++;
            }
            return true;
        }


        uint8_t  LisNet::     get( uint8_t *data, uint8_t len) {
            uint8_t * p;
            if( rx_offset + len > rx_len )return false;
            p = &rx_buffer[rx_offset];

            while (len--) {
                *data++ = *p++;
                rx_offset++;
            }
            return true;
        }

























uint64_t LisNet::   getAddr( uint8_t sub, uint8_t ip){
            uint64_t addr = 0;
            uint8_t * n = ( uint8_t*) &addr;
            *n++ = ip;
            *n++ = sub;
            for( byte q = 0 ; q < 3; q++){
                *n++ = net[q];
            }
    return addr;
}



void LisNet::   received( uint8_t ip, uint8_t n  ){
    uint16_t pkUid = ip;
    pkUid <<= 8;
    pkUid += n;
    if(pkUid){
        rxPacketUid[rxPacketUidN] = pkUid;

        rxPacketUidN++;
        if( rxPacketUidN >= PK_RX_UID_LEN )rxPacketUidN=0;
    }
}
bool LisNet::   isReceived( uint8_t ip, uint8_t n  ){
    uint16_t pkUid = ip;
    pkUid <<= 8;
    pkUid += n;
    if(pkUid){
        uint8_t len = PK_RX_UID_LEN;

        while( len-- )
            if( rxPacketUid[len] == pkUid ){
                printf("{HavePK}");
                return true;
            }


        printf("{NewPK}");

    }
    return false;
}