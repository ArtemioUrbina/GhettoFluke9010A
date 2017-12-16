/*****************************************************************
 * 
 *   Ghetto Fluke 9010A Z80 "pod"
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
//  M68000 interface pins 
//  ----------------------------------------------------------------------------------------------------------------------------
#define A00     53
#define A01     51
#define A02     49  
#define A03     47
#define A04     45
#define A05     43
#define A06     41
#define A07     39
#define A08     37
#define A09     35
#define A10     33
#define A11     23
#define A12     25  
#define A13     27
#define A14     29
#define A15     31

#define D00     34
#define D01     36  
#define D02     30
#define D03     24
#define D04     22
#define D05     26
#define D06     28
#define D07     32

#define MREQ    38
#define IORQ    40
#define RFSH    42
#define M1      44
#define RESET   46
#define WAIT    48
#define WR      50
#define RD      52

int address_bus[] = { A00, A01, A02, A03, A04, A05, A06, A07, A08, A09, 
                      A10, A11, A12, A13, A14, A15 };

int data_bus[] = { D00, D01, D02, D03, D04, D05, D06, D07 };



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
  lcd.print("Ghetto Z80");
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

  for(count = 0; count < 8; count++)
    pinMode(data_bus[count], OUTPUT); 
}

void SetDataRead()
{
  int count = 0;

  for(count = 0; count < 8; count++)
    pinMode(data_bus[count], INPUT); 
}

void PrepareOutput()
{
  int count = 0;

  for(count = 0; count < 16; count++)
    pinMode(address_bus[count], OUTPUT); 

  SetDataRead(); 

  pinMode(MREQ, OUTPUT); 
  pinMode(IORQ, OUTPUT); 
  pinMode(RFSH, OUTPUT); 
  pinMode(M1, OUTPUT); 
  pinMode(WR, OUTPUT); 
  pinMode(RD, OUTPUT); 
  pinMode(RESET, INPUT); 
  pinMode(WAIT, INPUT);
  
  for(count = 0; count < 16; count++)
    digitalWrite(address_bus[count], LOW);

  digitalWrite(MREQ, HIGH);
  digitalWrite(IORQ, HIGH);
  digitalWrite(RFSH, HIGH);
  digitalWrite(M1, HIGH);
  digitalWrite(WR, HIGH);
  digitalWrite(RD, HIGH);
}

void SetAddress(unsigned int data)
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
        digitalWrite(address_bus[count], HIGH);
      else
        digitalWrite(address_bus[count], LOW);
      count --;
    }
  }
}

void RequestAddress()
{
  digitalWrite(MREQ, LOW);
  digitalWrite(RD, LOW);
}

void EndRequestAddress()
{
  digitalWrite(MREQ, HIGH);
  digitalWrite(RD, HIGH);
}

void SetRealeaseDataBus()
{
  digitalWrite(RD, HIGH);
  digitalWrite(WR, HIGH);
}

void SetReadDataFromBus()
{
  SetDataRead(); 
  digitalWrite(RD, LOW);
  digitalWrite(WR, HIGH);
}

void SetWriteDataToBus()
{
  SetDataWrite(); 
  digitalWrite(RD, HIGH);
  digitalWrite(WR, LOW);
}

void SetData(unsigned int data)
{
  int b = 0;
  unsigned char bits[8];
  
  for (b = 7; b > -1; b--) 
  {  
    bits[b] = (data & (1 << b)) != 0;
    if(bits[b] == 1)
      digitalWrite(data_bus[b], HIGH);
    else
      digitalWrite(data_bus[b], LOW);
  }
}

unsigned int ReadData()
{
  int count = 0;
  unsigned int data = 0;

  for(count = 7; count >= 0; count--)
    data = (data << 1) | digitalRead(data_bus[count]);
  
  return data;
}

void CheckRAM(long startAddress, long endAddress)
{
  long address = 0;
  unsigned int data = 0, error = 0;
  char text[40];

  Serial.print("Writing to RAM (lower 8 bit): ");
  sprintf(text, "0x%0.6lX", startAddress);
  Serial.print(text);
  sprintf(text, "-0x%0.6lX", endAddress);
  Serial.print(text);
  sprintf(text, " with address & 0x00FF");
  Serial.println(text);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address++)
  {
    SetAddress(address);
    RequestAddress();
    
    SetData(address);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address++)
  {
 
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
      
    EndRequestAddress();

    if((data & 0x00FF) != (0x00FF & address))
    {
      Serial.println("ERROR IN RAM");
      sprintf(text, "Address: 0x%0.6lX", address);
      Serial.print(text);
      sprintf(text, " Got: 0x%0.4X", data);
      Serial.print(text);
      sprintf(text, " Expected: 0x%0.4X", 0x00FF & address);
      Serial.println(text);
      error++;
    }
  }
  if(!error)
  {
    Serial.println("RAM OK");
    delay(2000);
  }
}

void CheckRAMPattern(long startAddress, long endAddress, int pattern)
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
  for(address = startAddress; address < endAddress; address++)
  {
    SetAddress(address);
    RequestAddress();

    SetData(pattern);
      
    EndRequestAddress();
  }
  
  Serial.println("Reading from RAM");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address++)
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
      error++;
    }
  }
  if(!error)
  {
    Serial.println("RAM OK");
    delay(2000);
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
  long address = 0x0000;
  char text[40];
  
  //8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side
  CheckRAM(0x8000, 0x87ff);
  CheckRAMPattern(0x8000, 0x87ff, 0XAA);

  delay(5000);
  Serial.println("Dumping ROM 0000-7fff");
  for(address = 0; address < 0x7fff ; address++)
  {
    int data = 0;
    
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
}


