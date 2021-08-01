#ifndef __PRINTF_H__
#define __PRINTF_H__



int serial_putc( char c, FILE * ) 
{
  Serial.write( c );

  return c;
} 

void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}

#endif // __PRINTF_H__
