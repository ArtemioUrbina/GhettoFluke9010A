#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Increments for address space
#define ADDR_16 2
#define ADDR_8  1

#define NORMHEX   1

//  ----------------------------------------------------------------------------------------------------------------------------
//  Interface defines
//  ----------------------------------------------------------------------------------------------------------------------------

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5


int lcd_key       = btnNONE;
int adc_key_in    = 0;

void initLCD()
{
  lcd.begin(16, 2);
}

int read_LCD_buttons()
{
  adc_key_in = analogRead(0);   
  delay(5);
  int k = (analogRead(0) - adc_key_in); 
  if (5 < abs(k)) return btnNONE;  
  
  if (adc_key_in > 1000) return btnNONE; 
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 195)  return btnUP; 
  if (adc_key_in < 380)  return btnDOWN; 
  if (adc_key_in < 555)  return btnLEFT; 
  if (adc_key_in < 790)  return btnSELECT;   
  
  return btnNONE;  
}  

void DisplayTop(const char *text)
{
  int len = 0;
  char buffer[17];

  len = strlen(text);
  if(len < 16)
  {
    strncpy(buffer, (char*)text, len);
    for(int i = len; i < 16; i++)
      buffer[i] = ' ';
  }
  else
    strncpy(buffer, (char*)text, 16);
  buffer[16] = '\0';

  lcd.setCursor(0,0);
  lcd.print(buffer);
}

void DisplayBottom(const char *text)
{
  int len = 0;
  char buffer[17];

  len = strlen(text);
  if(len < 16)
  {
    strncpy(buffer, (char*)text, len);
    for(int i = len; i < 16; i++)
      buffer[i] = ' ';
  }
  else
    strncpy(buffer, (char*)text, 16);
  buffer[16] = '\0';

  lcd.setCursor(0,1);
  lcd.print(buffer);
}

void Display(const char *text1, const char *text2)
{
  DisplayTop(text1);
  DisplayBottom(text2);
}

int WaitKey()
{
  lcd_key = btnNONE;
  do
  {
    lcd_key = read_LCD_buttons(); 
  }while(lcd_key == btnNONE);
  
  delay(200);
  return lcd_key;
}

uint32_t SelectHex(uint32_t startVal, uint32_t endVal, uint8_t charCount, uint8_t increment, int posy, const char *msg)
{
  char      buffer[24], chardefine[24];
  uint32_t  value = startVal, mask = 0, 
            bytepos = charCount - 1, byteval = 0;
  uint8_t   x = 0, show = 0, posx = 0;

  posx = strlen(msg) + 3;
  x = posx;
  
  sprintf(chardefine, "%s 0x%%0.%dlX      ", msg, charCount);
  sprintf(buffer, chardefine, value);

  lcd.blink();
  do
  {
    do
    {
      sprintf(buffer, chardefine, value);
      lcd.setCursor(0, posy);
      lcd.print(buffer);
      lcd_key = read_LCD_buttons(); 
      lcd.setCursor(x, posy);
      if(show = 1)
        lcd.blink();
      else
        lcd.noBlink();
      show = !show;
      delay(150);
    }while(lcd_key == btnNONE);

    switch(lcd_key)
    {
      case btnUP:
        mask = ((uint32_t)0xF << (bytepos*4));
        byteval = (value & mask) >> (bytepos*4);
        if(bytepos == 0)
          byteval += increment;
        else
          byteval ++;
        byteval &= 0xF;
        byteval = byteval << (bytepos*4);
        mask = ~mask;
        value = (value & mask) | byteval;
        break;
      case btnDOWN:
        mask = ((uint32_t)0xF << (bytepos*4));
        byteval = (value & mask) >> (bytepos*4);
        if(bytepos == 0)
          byteval -= increment;
        else
          byteval --;
        byteval &= 0xF;
        byteval = byteval << (bytepos*4);
        mask = ~mask;
        value = (value & mask) | byteval;
        break;
      case btnLEFT:
        x = x - 1;
        if(x < posx)
          x = posx + charCount - 1;
        if(bytepos == charCount - 1)
          bytepos = 0;
        else
          bytepos++;
        break;
      case btnRIGHT:
        x = x + 1;
        if(x > posx + charCount - 1)
          x = posx;
        if(bytepos == 0)
          bytepos = charCount - 1;
        else
          bytepos--;
      break;
    }    
    if(value < startVal)
      value = startVal;
    if(value > endVal)
      value = endVal;
  }while(lcd_key != btnSELECT);

  lcd.noBlink();
  
  return value;
}

#define StartProgress(startval, endval) \  
    uint32_t __unit, __amount, __last; \  
    DisplayBottom("[--------------]"); __unit = (endval - startval)/(uint32_t)15; __amount = 0; __last = (uint32_t)0xFFFF;

#define DisplayProgress(pos) \  
    __amount = pos / __unit;  \  
    if(__amount != __last) { lcd.setCursor((int)__amount, 1); lcd.print("*"); __last = __amount; }

