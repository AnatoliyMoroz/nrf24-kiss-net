#include "RF24.h"

#define PK_RX_UID_LEN 10
struct pkHeader_t{
    uint8_t     type;
    uint16_t    uid;
    //uint8_t nodeId;
};
struct pkReceived_t{
    // resend header
};

struct pkError{
  uint8_t code;
};

#define PK_HEADER 255


#define PK_SIZE_HEADER sizeof(pkHeader_t)
#define PK_MAX_SIZE 32

typedef enum{
  COM_SENDING=1,
  COM_LISTEN,
  COM_READY,
}ComState_e;

typedef void ( *onReceiveFunction_p )( pkHeader_t header, uint8_t pipe_num );




class RfCom{
  public:
    //RfCom();
    RfCom(RF24 *r);
    void begin();
    //bool isReadyWrite();
    void int0();
    bool        transmit(uint8_t resp=0);

    uint8_t rxSize(){return rx_len> PK_SIZE_HEADER ? rx_len - PK_SIZE_HEADER:0;}

    bool write(uint8_t type, uint8_t *data , uint8_t len, uint8_t resp=0);


    template <typename T> bool write(uint8_t type, T& blob, uint8_t resp=0){
        uint8_t len = sizeof( blob );
        uint8_t * data = ( uint8_t*) &blob;
        return this->write( type,  data , len, resp);
    };

    bool response();

    template <typename T> bool response(uint8_t type, T& blob){
            return write( type, blob, 1);
        }

    template <typename T> bool getBlob( T& blob ){
        uint8_t len = sizeof( blob );
        uint8_t * data = ( uint8_t*) &blob;
        return this->get_rx( PK_SIZE_HEADER, data , len);
    }

    void setOnReceive(onReceiveFunction_p f){
        onReceiveFunction = f;
    }
    //void autoListen( bool x){disableAutoListen=x;}
  private:
    void        received(uint16_t pkUid);
    bool        isReceived(uint16_t pkUid);
    //bool        disableAutoListen;

    onReceiveFunction_p     onReceiveFunction;
    unsigned long started_waiting_at;
    //static RfCom *instance;

    //static 
    RF24        *radio;
    bool        nowWriting;
    bool        enabled;
    bool        busyISR;
    ComState_e  state;
    bool        waitISR;

    uint16_t rxPacketUid[PK_RX_UID_LEN];
    uint8_t rxPacketUidN;

    uint8_t rx_buffer[PK_MAX_SIZE];
    uint8_t rx_len;

    uint16_t txPacketN;
    uint8_t tx_buffer[PK_MAX_SIZE];
    uint8_t tx_len;

    void clear_tx();
    void clear_rx();
    bool put_tx(uint8_t *data, uint8_t len);
    template <typename T> bool put_tx( T& blob ){
        uint8_t len = sizeof( blob );
        if( tx_len+len > 32 ) return false;
        uint8_t * data = (byte*) &blob;
        return put_tx(data,len);
    }
    bool get_rx(uint8_t offset, uint8_t *data, uint8_t len);
    template <typename T> bool get_rx(uint8_t offset, T& blob){
        uint8_t len = sizeof( blob );
        uint8_t * data = ( byte*) &blob;
        return get_rx(offset, data, len );
    }
};

/*
template <typename T> uint8_t CRC8(uint8_t crc, const T& value) {
  uint16_t len = sizeof( value );
  const uint8_t * data = (const uint8_t*) &value;
  return CRC8(crc, data, len);
}

template <typename T> uint8_t CRC8(const T& value) {
  uint8_t crc = 0;
  return CRC8(crc, value);
}

uint8_t CRC8(uint8_t crc, const uint8_t *data, uint16_t len) {
  //byte crc = 0x00;
  Serial.print(" CRC|");

  while (len--) {
    uint8_t extract = *data++;
    Serial.print(extract);
    Serial.print(' ');
    for (uint8_t tempI = 8; tempI; tempI--) {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  Serial.print(" | ");
  return crc;
}
*/




