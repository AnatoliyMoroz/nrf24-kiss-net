#ifndef __LISNET_H__
#define __LISNET_H__

#include "RF24.h"

#define PK_RX_UID_LEN 10

#define PK_BROADCAST    0x01
#define PK_STREAM       0x02
#define PK_COM          0x03
#define PK_CK          0x04



/*
broadcast
0  Protocol
1  N
2  s. sub net
3  s. ip
4  d. port

stream
0  Protocol
1  N
2  s. ip
3  d. port

com
0  Protocol
1  s. ip
2  d. port

on RX ( ip, port )
*/

struct headerBroadcast_t{
    uint8_t     n;
    uint8_t     srcIp;
    uint8_t     srcSub;
    uint8_t     port;
};


struct headerStream_t{
    uint8_t     n;
    uint8_t     srcIp;
    uint8_t     port;
};

struct headerPacket_t{
    uint8_t     n;
    uint8_t     srcIp;
    uint8_t     port;
};

struct headerCheck_t{
    uint8_t     n;
    uint8_t     srcIp;
    //uint8_t     port;
};

typedef void ( *onReceiveBroadcast_p )  ( headerBroadcast_t *header, uint8_t pipe_num, uint8_t len );
typedef void ( *onReceiveStream_p )     ( headerStream_t    *header, uint8_t pipe_num, uint8_t len );
typedef void ( *onReceivePacket_p )     ( headerPacket_t    *header, uint8_t pipe_num, uint8_t len );

typedef void ( *onSuccess_p )       ( uint8_t retry );
typedef void ( *onFail_p )          (  );

class LisNet{

    public:

        LisNet( RF24 *r );
        void    setNetwork  ( const char * domain, uint8_t sub, uint8_t ip );


        void        broadcast   ( uint8_t port );
        uint8_t     stream      ( uint8_t ip, uint8_t port );
        uint8_t     send        ( uint8_t ip, uint8_t port );
        void        response    ( uint8_t ip, uint8_t n );

        void success( onSuccess_p f ){
            onSuccess = f;
        }

        void fail( onFail_p f ){
            onFail = f;
        }

        uint8_t     isBusy      (){
            return ( nowWriting || waitConfirm );
        }

        void        stopRepeat(){
            repeatTx = 0;
        }
        uint8_t    transmit( uint8_t rp =0 ){
            if( tx_len ){
                if( rp )repeatTx = rp ;
                if( dst_confirm ) waitConfirm = true;
                radio->openWritingPipe( getAddr( dst_sub, dst_ip ) );
                radio->stopListening();
                radio->powerUp() ;

                //printf("(TX L=%i)",tx_len);

                nowWriting = true;
                radio->write( tx_buffer, tx_len);
                nowWriting = false;

                _delay_ms(1);
                radio->startListening();
                return true;
            }else{
                return false;
            }
        };

        void openPipe( uint8_t p, uint8_t ip ){
            if(p>1&&p<=5)radio->openReadingPipe( p, getAddr( 0, ip ) );
        }

        uint64_t getAddr( uint8_t sub, uint8_t ip );

        void    onReceiveBroadcast( onReceiveBroadcast_p f ){
            onReceiveBroadcastFunction = f;
        }

        void    onReceiveStream( onReceiveStream_p f ){
            onReceiveStreamFunction = f;
        }

        void    onReceivePacket( onReceivePacket_p f ){
            onReceivePacketFunction = f;
        }

        void    int0(){
            if( !nowWriting ){
                uint8_t pipe_num;
                if ( radio->available( &pipe_num ) ){

                bool done = false;

                    while ( !done )
                    {
                        rx_offset = 0;
                        rx_len  = radio-> getDynamicPayloadSize();
                        done    = radio-> read( rx_buffer, rx_len );


                        uint8_t protocol = 0;


                        if( rx_len ){
                            protocol = rx_buffer[0];
                            rx_offset++;
                        }

                        switch( protocol ){
                            case PK_BROADCAST:
                                //printf("(RXbrc%i %i)",pipe_num,rx_len);
                                if( onReceiveBroadcastFunction ){
                                    headerBroadcast_t hd;

                                    if( get(hd)  ){
                                        if( !isReceived( hd.srcIp, hd.n ) ){
                                            uint8_t len = rx_len-rx_offset;
                                            received( hd.srcIp, hd.n);
                                            if( hd.port == 255 ){
                                                if( ! isBusy() && repeatTx == 0 ){
                                                    stream( hd.srcIp, 255 );
                                                    transmit(3);
                                                }
                                            }else onReceiveBroadcastFunction( &hd,pipe_num, len  );
                                        }
                                    }//else printf("(erBr)");
                                }
                            break;
                            case PK_STREAM:
                                //printf("(RXstr%i %i)",pipe_num,rx_len);
                                if( onReceiveStreamFunction ){
                                    headerStream_t hd;
                                    if( get(hd)  ){
                                        uint8_t len = rx_len-rx_offset;
                                        if( !isReceived( hd.srcIp, hd.n ) ){
                                            received( hd.srcIp, hd.n );
                                            onReceiveStreamFunction( &hd,pipe_num, len );
                                        }
                                    }//else printf("(erBr)");
                                }
                            break;
                            case PK_COM:
                                //printf("(RXcom%i %i)",pipe_num,rx_len);
                                headerPacket_t hd;
                                if( get(hd)  ){
                                    uint8_t len = rx_len-rx_offset;
                                    //printf("\n\rresponse  %i.%i:%i" ,addr_sub, hd.srcIp, hd.n );

                                    response( hd.srcIp, hd.n );
                                    transmit();

                                    if( !isReceived( hd.srcIp, hd.n ) ){
                                        received( hd.srcIp, hd.n);
                                        printf("\n\rRX com [%i] %i)",pipe_num,rx_len);
                                        if( onReceivePacketFunction )onReceivePacketFunction(&hd,pipe_num, len);
                                    }
                                }
                            break;
                            case PK_CK:
                                //printf("(RXack%i %i)",pipe_num,rx_len);
                                if( waitConfirm ){
                                    headerPacket_t hd;
                                    if( get(hd)  ){
                                        if( dst_ip == hd.srcIp && dst_n == hd.n ){
                                            waitConfirm = false;
                                            if( onSuccess ) onSuccess( sendRetry-dst_retry );
                                        }else{
                                            //printf("(err982)");
                                        }
                                    }
                                }else{

                                }
                            break;
                        }
                    }

                    radio->startListening();
                }


            }else{

                //needCheck = true;
            }
        }


        void    service(){
            if( repeatTx > 0 ){
                repeatTx--;
                transmit();
            }
            if( waitConfirm ){
                if( dst_retry ){
                    dst_retry--;
                    printf("(RETR%i)",dst_retry);
                    transmit();
                }else{
                    waitConfirm = false;
                    //dst_confirm = 0;
                    if( onFail ) onFail();
                }
            }
        }





        uint8_t getFreeTx(){
            return 32 - tx_len;
        }

        void    reset_tx(){
            tx_len = 0;
        }

        void    clear_tx(){
            reset_tx();
            memset( &tx_buffer, 0, sizeof( tx_buffer ) );
        }



        uint8_t put( uint8_t v );
        uint8_t put(const uint8_t *data, uint8_t len);
        template <typename T> uint8_t put( const T& blob ){
            uint8_t len = sizeof( blob );
            if( tx_len+len > 32 ) return 0;
            uint8_t * data = ( uint8_t*) &blob;
            return put(data,len);
        }



        template <typename T> uint8_t get( T& blob ){
            uint8_t len = sizeof( blob );
            uint8_t * data = ( uint8_t*) &blob;
            return this->get( data , len );
        }
        uint8_t get( uint8_t *data, uint8_t len);

    private:
        uint8_t     net[3];// = {12,3,42};
        RF24        *radio;

        uint8_t     nowWriting;
        uint8_t     needCheck;
        uint8_t     powerOff;

        uint8_t     n2n; // listen 0- broadcast  1-node
        uint8_t     addr_sub;
        uint8_t     addr_ip;

        uint8_t     dst_sub;
        uint8_t     dst_ip;
        uint8_t     dst_n; // number of send packet
        uint8_t     dst_confirm;
        uint8_t     dst_retry;
        uint8_t     sendRetry;

        uint8_t     repeatTx;
        uint8_t waitConfirm;
        uint8_t txPacketN;

        uint8_t tx_buffer[32];
        uint8_t tx_len;

        uint8_t rx_offset;
        uint8_t rx_buffer[32];
        uint8_t rx_len;

        uint16_t rxPacketUid[PK_RX_UID_LEN];
        uint8_t  rxPacketUidN;

        void        received    (  uint8_t ip, uint8_t n );
        bool        isReceived  (  uint8_t ip, uint8_t n );

        void        response    ( uint8_t ip, uint8_t port , uint8_t n);

        onReceiveBroadcast_p    onReceiveBroadcastFunction;
        onReceiveStream_p       onReceiveStreamFunction;
        onReceivePacket_p       onReceivePacketFunction;

        onSuccess_p             onSuccess;
        onFail_p                onFail;

};

#endif // __

