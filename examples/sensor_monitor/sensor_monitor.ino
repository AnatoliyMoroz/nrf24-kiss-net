// Display ILI9341_DRIVER
// Edit Pin & font lib/TFT_eSPI/User-setup.h 
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

#include "LisNet.h"
RF24 radio(7,8);
LisNet r(&radio);

#include "tools.h"




Channel_t channels[LOG_CHANNELS];

Graph_t tmp[2],tmp2[1];


void onReadPacket( headerPacket_t * h, uint8_t p );
void check_radio(void){ r.int0(); }

void setup() {
  // ===============  Init program  ===============================================================
  tmp[0].ch = 0;
  tmp[0].color = TFT_ORANGE;
  tmp[1].ch = 1;
  tmp[1].color = TFT_SKYBLUE;
  tmp2[0].ch = 2;
  tmp2[0].color = TFT_GREEN;
  // put your setup code here, to run once:
  Serial.begin(115200);

   for( byte n = 0; n < LOG_LEN ; n++ ){
    log_data[0][n]= -32768;//80+ random(20);
    log_data[1][n]=  -32768;//random(20);
    log_data[2][n]= -32768;//250+ random(20);// ;
   }
  
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);

  r.setNetwork( "LIS", 10, 1 );
  r.onReceivePacket(onReadPacket);

  radio.printDetails();
  attachInterrupt(0, check_radio, FALLING);
  radio.startListening();

}

uint8_t ns=0;
void loop() {
  // ===============  Main program loop  ==========================================================
  if( ns != log_pos ){

    cli();
    drawGraph(40,120,LOG_LEN*3,100,tmp,2);
    
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    drawFloat( channels[0].vmax / 100.0 , 1,2, 30, 8, 0, 120 );
    drawFloat( channels[0].vmin / 100.0 , 1,2, 30, 8, 0, 206 );

    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    drawFloat( channels[1].vmax / 100.0 , 1,2, 30, 8, 290, 120 );
    drawFloat( channels[1].vmin / 100.0 , 1,2, 30, 8, 290, 206 );
    
    drawGraph(40,50,LOG_LEN*3,50,tmp2,1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    drawInt( channels[2].vmax , 1,2, 30, 8, 290, 50 );
    drawInt( channels[2].vmin , 1,2, 30, 8, 290, 86 );
    
    sei();
    
    ns = log_pos;
  }
   delay(100);
   
}


void onReadPacket( headerPacket_t * h, uint8_t p ){
  // ===============  Network read int  ===========================================================
  if(  h->port == 100  ){
    Pack_t msg;
    if( !r.get( msg ) ) return;
    
    log_data[0][log_pos] = msg.temperature * 100;
    log_data[1][log_pos] = msg.humidity * 100;
    
    log_data[2][log_pos] = msg.pressure - 100000;

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    drawFloat( msg.temperature, 1,6, 100, 48, 0, 0 );
    drawFloat( msg.humidity, 1,6, 100, 48, 130, 0 );
    drawFloat( msg.pressure - 100000, 0,2, 30, 48, 260, 26 );

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    drawFloat( msg.vBat, 2,2, 30, 48, 260, 10 );
    
    log_pos++;
    if( log_pos >= LOG_LEN ) log_pos = 0;
    
    return;
  }
}





void drawGraph( uint16_t x,uint16_t y,uint16_t w,uint16_t h, Graph_t g[], uint8_t c){
  int16_t val;
  for( uint8_t i=0; i<c ; i++ ){
    uint8_t ch = g[i].ch;
    
    int16_t vmax = -32768+1;
    int16_t vmin = 32767;
    int16_t diff;

    for( byte n = 0; n < LOG_LEN ; n++ ){
      val = log_data[ch][n];
      if( val == -32768 ) continue;
      if( val > vmax ) vmax = val;
      if( val < vmin ) vmin = val;
    }
    channels[ch].vmax = vmax;
    channels[ch].vmin = vmin;
    
    diff= vmax - vmin;
    if( diff==0 ) diff = 1;
    g[i].vh = h-2;
    g[i].vh /= diff ;
  }
  
  
  tft.drawFastHLine(x,y,w,TFT_DARKGREY);
  tft.drawFastHLine(x,y+h,w,TFT_DARKGREY);

  h--;
  //y++;

  uint8_t y0,y1; 
  
  for( byte r = 0; r < LOG_LEN ; r++ ){
    byte n = (r+log_pos)%LOG_LEN;
    for(byte q=0;q<3;q++)tft.drawFastVLine(x+r*3+q,y+1,h,TFT_BLACK);   
    for( uint8_t i=0; i<c ; i++ ){
      uint8_t ch = g[i].ch;
      val = log_data[ch][n];
      if( val == -32768 ){ g[i].y1=0; continue;}
     
      val -= channels[ch].vmin;
      val *= g[i].vh;
      y0 = y+h-val;
      y1 = g[i].y1;
      
      if( y1 ){ 
      tft.drawLine( x + r*3-3, y1, x+r*3, y0, g[i].color);
      }
      g[i].y1 = y0;
    }    
  }
  
}

void drawFloat( float val, uint8_t num, uint8_t s,uint8_t w,uint8_t h,uint16_t x,uint8_t y ){
  char buff[11];
  dFloat(val, num, buff );
  uint8_t i = tft.textWidth(buff,s);
  //tft.fillRect(x+i,y,x+w,y,TFT_BLACK );

  tft.drawString(buff,x,y,s);
}

void drawInt( int val, uint8_t num, uint8_t s,uint8_t w,uint8_t h,uint16_t x,uint8_t y ){
  //char buff[11];
  //dFloat(val, num, buff );
  //uint8_t i = tft.textWidth(buff,s);
  //tft.fillRect(x+i,y,x+w,y,TFT_BLACK );

  tft.drawNumber(val,x,y,s);
}


