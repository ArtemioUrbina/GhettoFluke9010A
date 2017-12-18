#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

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
