/*****************************************************************
 * 
 *   Ghetto Fluke 9010A 68k "pod"
 *       by Artemio Urbina 2017
 *       GNU GPL 2.0
 *   
 * 
 *   This is intended for Arcade PCB repair
 *   will have routines for ROM checksum, 
 *   RAM checking and step by step addressing
 *   of a memory map.
 *   
 *   Version 0.1: Added RAM check routines and basic functions
 *****************************************************************/



//#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//  ----------------------------------------------------------------------------------------------------------------------------
//  M68000 interface pins (Free 0, 1, 10, 13)
//  ----------------------------------------------------------------------------------------------------------------------------
#define A01     46
#define A02     48  
#define A03     50
#define A04     52
#define A05     42
#define A06     40
#define A07     38
#define A08     53
#define A09     51
#define A10     49
#define A11     47
#define A12     45  
#define A13     43
#define A14     41
#define A15     39
#define A16     37
#define A17     35
#define A18     33
#define A19     31
#define A20     29
#define A21     27
#define A22     25  
#define A23     23

#define D00     14 
#define D01     15  
#define D02     16
#define D03     17
#define D04     18
#define D05     19
#define D06     2
#define D07     13
#define D08     22
#define D09     24
#define D10     26
#define D11     28  
#define D12     30
#define D13     32
#define D14     34
#define D15     36

#define RESET   12
#define RW      11
#define AS      44
#define UDS     10
#define LDS     3

int address_bus[] = { A01, A02, A03, A04, A05, A06, A07, A08, A09, A10,
                      A11, A12, A13, A14, A15, A16, A17, A18, A19, A20,
                      A21, A22, A23 };

int data_bus[] = { D00, D01, D02, D03, D04, D05, D06, D07, D08, D09,
                   D10, D11, D12, D13, D14, D15 };



//  ----------------------------------------------------------------------------------------------------------------------------
//  Interface defines
//  ----------------------------------------------------------------------------------------------------------------------------

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// delay amount
#define DELAY_TIME  10

//  ----------------------------------------------------------------------------------------------------------------------------
//  User Interface
//  ----------------------------------------------------------------------------------------------------------------------------

/*
void DisplayIntro()
{
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Ghetto 68000");
  lcd.setCursor(0,1);
  lcd.print("Artemio 2017");
  delay(2500);
}
*/

//  ----------------------------------------------------------------------------------------------------------------------------
//  Backend
//  ----------------------------------------------------------------------------------------------------------------------------

void SetDataWrite()
{
  int count = 0;

  for(count = 0; count < 16; count++)
    pinMode(data_bus[count], OUTPUT); 
}

void SetDataRead()
{
  int count = 0;

  for(count = 0; count < 16; count++)
    pinMode(data_bus[count], INPUT); 
}

void PrepareOutput()
{
  int count = 0;

  for(count = 0; count < 23; count++)
    pinMode(address_bus[count], OUTPUT); 

  SetDataRead(); 

  pinMode(AS, OUTPUT); 
  pinMode(RW, OUTPUT); 
  pinMode(UDS, OUTPUT); 
  pinMode(LDS, OUTPUT); 
  pinMode(RESET, INPUT); 
  
  for(count = 0; count < 23; count++)
    digitalWrite(address_bus[count], LOW);

  digitalWrite(UDS, LOW);
  digitalWrite(LDS, LOW);
  digitalWrite(AS, LOW);
  digitalWrite(RW, HIGH);
}

void SetAddress(long address)
{
  int pos = 0, b = 0, count = 22, start = 0;;
  unsigned char bits[8];
  unsigned char cByte = 0;

  for(pos = 0; pos < 4; pos ++)
  {
    cByte = (address & 0xFF000000L) >> 24;
    address = address << 8;
    for (b = 7; b > -1; b--) 
    {  
      if(start++ > 7) // We start at Address 23
      {
        bits[b] = (cByte & (1 << b)) != 0;
        if(bits[b] == 1)
          digitalWrite(address_bus[count], HIGH);
        else
          digitalWrite(address_bus[count], LOW);
        count --;
      }
    }
  }
}

void RequestAddress()
{
  digitalWrite(AS, LOW);
}

void EndRequestAddress()
{
  digitalWrite(AS, HIGH);
}

void SetReadDataFromBus()
{
  SetDataRead(); 
  digitalWrite(RW, HIGH);
}

void SetWriteDataToBus()
{
  SetDataWrite(); 
  digitalWrite(RW, LOW);
}

void SetData(unsigned int data)
{
  int pos = 0, b = 0, count = 15;
  unsigned char bits[8];
  unsigned char cByte = 0;

  for(pos = 0; pos < 2; pos ++)
  {
    cByte = (data & 0xFF00) >> 8;
    data = data << 8;
    for (b = 7; b > -1; b--) 
    {  
      bits[b] = (cByte & (1 << b)) != 0;
      if(bits[b] == 1)
        digitalWrite(data_bus[count], HIGH);
      else
        digitalWrite(data_bus[count], LOW);
      count --;
    }
  }
}

unsigned int ReadData()
{
  int count = 0;
  unsigned int data = 0;

  for(count = 15; count >= 0; count--)
    data = (data << 1) | digitalRead(data_bus[count]);
  
  return data;
}


void CheckRAM(long startAddress, long endAddress)
{
  long address = 0;
  unsigned int data = 0, error = 0;
  char text[40];

  Serial.print("Writing to RAM: ");
  sprintf(text, "0x%0.6lX", startAddress);
  Serial.print(text);
  sprintf(text, "-0x%0.6lX", endAddress);
  Serial.print(text);
  sprintf(text, " with address & 0x0000FFFF");
  Serial.println(text);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    SetData(address);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
 
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    if(data != (0x0000FFFF & address))
    {
      Serial.println("ERROR IN RAM");
      sprintf(text, "Address: 0x%0.6lX", address);
      Serial.print(text);
      sprintf(text, " Got: 0x%0.4X", data);
      Serial.print(text);
      sprintf(text, " Expected: 0x%0.4X", 0x0000FFFF & address);
      Serial.println(text);
    }
  }
}

void CheckRAMPattern(long startAddress, long endAddress, int pattern)
{
  long address = 0;
  unsigned int data = 0, error = 0;
  char text[40];

  Serial.print("Writing to RAM: ");
  sprintf(text, "0x%0.6lX", startAddress);
  Serial.print(text);
  sprintf(text, "-0x%0.6lX", endAddress);
  Serial.print(text);
  sprintf(text, " with value 0x%0.4X", pattern);
  Serial.println(text);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();

    SetData(pattern);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
 
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    if(data != pattern)
    {
      Serial.println("ERROR IN RAM");
      sprintf(text, "Address: 0x%0.6lX", address);
      Serial.print(text);
      sprintf(text, " Got: 0x%0.4X", data);
      Serial.print(text);
      sprintf(text, " Expected: 0x%0.4X", pattern);
      Serial.println(text);
    }
  }
}

void CheckRAM8bit(long startAddress, long endAddress)
{
  long address = 0;
  unsigned int data = 0, error = 0;
  char text[40];

  Serial.print("Writing to RAM (lower 8 bit): ");
  sprintf(text, "0x%0.6lX", startAddress);
  Serial.print(text);
  sprintf(text, "-0x%0.6lX", endAddress);
  Serial.print(text);
  sprintf(text, " with address & 0x000000FF");
  Serial.println(text);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    SetData(address);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
 
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    if((data & 0x00FF) != (0x000000FF & address))
    {
      Serial.println("ERROR IN RAM");
      sprintf(text, "Address: 0x%0.6lX", address);
      Serial.print(text);
      sprintf(text, " Got: 0x%0.4X", data);
      Serial.print(text);
      sprintf(text, " Expected: 0x%0.4X", 0x000000FF & address);
      Serial.println(text);
    }
  }
}

void CheckRAM8bitPattern(long startAddress, long endAddress, int pattern)
{
  long address = 0;
  unsigned int data = 0, error = 0;
  char text[40];

  Serial.print("Writing to RAM (lower 8 bit): ");
  sprintf(text, "0x%0.6lX", startAddress);
  Serial.print(text);
  sprintf(text, "-0x%0.6lX", endAddress);
  Serial.print(text);
  sprintf(text, " with value 0x%0.4X", 0x00FF & pattern);
  Serial.println(text);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();

    SetData(pattern);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
 
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    if((data & 0x00FF) != (pattern & 0x00FF))
    {
      Serial.println("ERROR IN RAM");
      sprintf(text, "Address: 0x%0.6lX", address);
      Serial.print(text);
      sprintf(text, " Got: 0x%0.4X", data);
      Serial.print(text);
      sprintf(text, " Expected: 0x%0.4X", pattern & 0x00FF);
      Serial.println(text);
    }
  }
}

//  ----------------------------------------------------------------------------------------------------------------------------
//  Main Loop
//  ----------------------------------------------------------------------------------------------------------------------------

void setup() 
{
  PrepareOutput();
  //DisplayIntro();
  
  Serial.begin(115200);
}

void loop() 
{
  //RAM shared with TMS320C10NL-14 protection microcontroller
  CheckRAM(0x30000, 0x33fff);
  //40000-40fff RAM sprite display properties (co-ordinates, character, color - etc)
  CheckRAM(0x40fff, 0x40fff);
  //50000-50dff Palette RAM
  CheckRAM(0x50000, 0x50dff);
  //7a000-7abff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side
  CheckRAM8bit(0x7a000, 0x7abff);

  //RAM shared with TMS320C10NL-14 protection microcontroller
  CheckRAMPattern(0x30000, 0x33fff, 0xABAD);
  //40000-40fff RAM sprite display properties (co-ordinates, character, color - etc)
  CheckRAMPattern(0x40fff, 0x40fff, 0xABAD);
  //50000-50dff Palette RAM
  CheckRAMPattern(0x50000, 0x50dff, 0xABAD);
  //7a000-7abff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side
  CheckRAM8bitPattern(0x7a000, 0x7abff, 0xABAD);

/*
  for(address = 0; address < 0x1ffff ; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    delay(5);

    data = ReadData();
    EndRequestAddress();
    
    sprintf(text, " 0x%0.6lX: ", address);
    Serial.print(text);
    sprintf(text, "0x%0.4X", data);
    Serial.println(text);
  }
  */
}


