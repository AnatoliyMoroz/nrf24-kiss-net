typedef struct Pack_t{
  float     temperature;
  float     humidity;  
  int32_t   pressure;
  float     vBat;
  uint8_t   fail;
};
Pack_t msg;


#define LOG_CHANNELS  3
#define LOG_LEN       80

int16_t log_data[LOG_CHANNELS][LOG_LEN];
uint8_t log_pos=00;

typedef struct Channel_t{
  int16_t vmax,vmin,rt;
};

typedef struct Graph_t{
  uint8_t ch;
  uint16_t color;
  float   vh;
  uint8_t y1;
};

void dFloat(float floatNumber, int dp, char * str ) //int poX, int poY, int font)
{
  //char str[14];               // Array to contain decimal string
  uint8_t ptr = 0;            // Initialise pointer for array
  int8_t  digits = 1;         // Count the digits to avoid array overflow
  float rounding = 0.5;       // Round up down delta

  if (dp > 7) dp = 7; // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding)    // add sign, avoid adding - sign to 0.0!
  {
    *(str + ptr++ ) = '-'; // Negative number
    *(str + ptr ) = 0; // Put a null in the array as a precaution
    digits = 0;   // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber; // Make positive
  }

  floatNumber += rounding; // Round up or down

  // For error put ... in string and return (all TFT_ILI9341 library fonts contain . character)
  if (floatNumber >= 2147483647) {
    for(byte n=0;n<3;n++){ 
      *(str +n) = '.';
    }
    *(str +3)=0;
    return;
    //return drawString(str, poX, poY, font);
  }
  // No chance of overflow from here on

  // Get integer part
  unsigned long temp = (unsigned long)floatNumber;

  // Put integer part into array
  ltoa(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)*(str+ptr) != 0) ptr++; // Move the pointer along
  digits += ptr;                  // Count the digits

  *(str +ptr++) = '.'; // Add decimal point
  *(str +ptr) = '0';   // Add a dummy zero
  *(str +ptr + 1) = 0; // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) // while (i < dp) for no limit but array size must be increased
  {
    i++;
    floatNumber *= 10;       // for the next decimal
    temp = floatNumber;      // get the decimal
    ltoa(temp, str + ptr, 10);
    ptr++; digits++;         // Increment pointer and digits count
    floatNumber -= temp;     // Remove that digit
  }
  
  // Finally we can plot the string and return pixel length
  return ;//drawString(str, poX, poY, font);
}
