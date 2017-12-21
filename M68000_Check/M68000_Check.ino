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

#include "lcdsimp.h"
#include "crc32.h"


//  ----------------------------------------------------------------------------------------------------------------------------
//  M68000 interface pins (Free 0, 1, 10, 13)
//  ----------------------------------------------------------------------------------------------------------------------------
#define ADDR_L  23

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

#define DATA_L  16

#define D00     14 
#define D01     15  
#define D02     16
#define D03     17
#define D04     18
#define D05     19
#define D06     2
#define D07     12
#define D08     22
#define D09     24
#define D10     26
#define D11     28  
#define D12     30
#define D13     32
#define D14     34
#define D15     36

#define RESET   11
#define RW      13
#define AS      44
#define UDS     3
#define LDS     9

int address_bus[ADDR_L] = { A01, A02, A03, A04, A05, A06, A07, A08, A09, A10,
                      A11, A12, A13, A14, A15, A16, A17, A18, A19, A20,
                      A21, A22, A23 };

int data_bus[DATA_L] = { D00, D01, D02, D03, D04, D05, D06, D07, D08, D09,
                   D10, D11, D12, D13, D14, D15 };


//  ----------------------------------------------------------------------------------------------------------------------------
//  Backend
//  ----------------------------------------------------------------------------------------------------------------------------

void SetDataWrite()
{
  int count = 0;

  for(count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], OUTPUT); 
}

void SetDataRead()
{
  int count = 0;

  for(count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], INPUT); 
}

void PrepareOutput()
{
  int count = 0;

  for(count = 0; count < ADDR_L; count++)
    pinMode(address_bus[count], OUTPUT); 

  SetDataRead(); 

  pinMode(AS, OUTPUT); 
  pinMode(RW, OUTPUT); 
  pinMode(RESET, INPUT); 
  pinMode(UDS, OUTPUT); 
  pinMode(LDS, OUTPUT); 
  
  for(count = 0; count < ADDR_L; count++)
    digitalWrite(address_bus[count], LOW);

  digitalWrite(AS, LOW);
  digitalWrite(RW, HIGH);
  digitalWrite(UDS, LOW);
  digitalWrite(LDS, LOW);
}

void SetAddress(uint32_t address)
{
  int pos = 0, b = 0, count = ADDR_L - 1, start = 0;;
  uint8_t bits[8];
  uint8_t cByte = 0;

  for(pos = 0; pos < 4; pos ++)
  {
    cByte = (address & (uint32_t)0xFF000000) >> 24;
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

inline void RequestAddress()
{
  digitalWrite(AS, LOW);
}

inline void EndRequestAddress()
{
  digitalWrite(AS, HIGH);
}

inline void SetReadDataFromBus()
{
  SetDataRead(); 
  digitalWrite(RW, HIGH);
}

inline void SetWriteDataToBus()
{
  SetDataWrite(); 
  digitalWrite(RW, LOW);
}

void SetData(uint16_t data)
{
  int pos = 0, b = 0, count = DATA_L - 1;
  uint8_t bits[8];
  uint8_t cByte = 0;

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

uint16_t ReadData()
{
  int count = 0;
  uint16_t data = 0;

  for(count = DATA_L - 1; count >= 0; count--)
    data = (data << 1) | digitalRead(data_bus[count]);
  
  return data;
}


//  ----------------------------------------------------------------------------------------------------------------------------
//  Macro Functions
//  ----------------------------------------------------------------------------------------------------------------------------


void CheckROM(uint32_t startAddress, uint32_t endAddress)
{
  char text[40];
  uint32_t address, checksum;
  CRC32 crc;

  DisplayTop("<ROM CRC> ...");
  StartProgress(startAddress, endAddress);
  for(address = startAddress; address < endAddress; address+=2)
  {
    uint16_t data = 0;
    uint8_t mbyte = 0;

    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    mbyte = (data & 0xFF00) >> 8;
    crc.update(mbyte);
    mbyte = data & 0x00FF;
    crc.update(mbyte);

    DisplayProgress(address);
    EndRequestAddress();
  }
  
  checksum = crc.finalize();
  sprintf(text, "ROM: 0x%lX", checksum);
  DisplayBottom(text);
  WaitKey();
}


void CheckROMInterleaved(uint32_t startAddress, uint32_t endAddress)
{
  char text[40];
  uint32_t address, checksum1, checksum2;
  CRC32 crc1, crc2;

  DisplayTop("<ROM CRC> ...");
  StartProgress(startAddress, endAddress);
  for(address = startAddress; address < endAddress; address+=2)
  {
    uint16_t data = 0;
    uint8_t mbyte = 0;

    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    mbyte = (data & 0xFF00) >> 8;
    crc1.update(mbyte);
    mbyte = data & 0x00FF;
    crc2.update(mbyte);
    
    EndRequestAddress();
    
    DisplayProgress(address);
  }
  
  checksum1 = crc1.finalize();
  sprintf(text, "ROM1: 0x%lX", checksum1);
  DisplayTop(text);
  
  checksum2 = crc2.finalize();
  sprintf(text, "ROM2: 0x%lX", checksum2);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAM(uint32_t startAddress, uint32_t endAddress)
{
  uint32_t address = 0;
  uint16_t data = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    SetData(address);
      
    EndRequestAddress();
    
    DisplayProgress(address);
  }
  
  DisplayTop("<Read RAM> ...");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    DisplayProgress(address);
    if(data != (0x0000FFFF & address))
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);
      
      sprintf(text, "R: %0.4X E: %0.4X", data, 0x0000FFFF & address);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if(lcd_key == btnSELECT)
        return;
    }
  }
  
  DisplayTop("RAM OK");
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAMPattern(uint32_t startAddress, uint32_t endAddress, uint16_t pattern)
{
  uint32_t address = 0;
  uint16_t data = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();

    SetData(pattern);
      
    EndRequestAddress();
    DisplayProgress(address);
  }
  
  DisplayTop("<Read RAM> ...");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    DisplayProgress(address);
    if(data != pattern)
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);
      
      sprintf(text, "R: %0.4X E: %0.4X", data, pattern);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if(lcd_key == btnSELECT)
        return;
    }
  }
  
  DisplayTop("RAM OK");
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAM8bit(uint32_t startAddress, uint32_t endAddress)
{
  uint32_t address = 0;
  uint16_t data = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    SetData(address);
      
    EndRequestAddress();
    DisplayProgress(address);
  }
  
  DisplayTop("<Read RAM> ...");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    DisplayProgress(address);
    if((data & 0x00FF) != (0x000000FF & address))
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);
      
      sprintf(text, "R: %0.4X E: %0.4X", data, 0x000000FF & address);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if(lcd_key == btnSELECT)
        return;
    }
  }
  
  DisplayTop("RAM OK");
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAM8bitPattern(uint32_t startAddress, uint32_t endAddress, uint16_t pattern)
{
  uint32_t address = 0;
  uint8_t data = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);
  
  SetWriteDataToBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();

    SetData(pattern);
      
    EndRequestAddress();
    DisplayProgress(address);
  }
  
  DisplayTop("<Read RAM> ...");
  SetReadDataFromBus();
  for(address = startAddress; address < endAddress; address+=2)
  {
    SetAddress(address);
    RequestAddress();
    
    data = ReadData();
    EndRequestAddress();

    DisplayProgress(address);
    if((data & 0x00FF) != (pattern & 0x00FF))
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);
      
      printf(text, "R: %0.4X E: %0.4X", data, pattern & 0x00FF);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if(lcd_key == btnSELECT)
        return;
    }
  }
  
  DisplayTop("RAM OK");
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

//  ----------------------------------------------------------------------------------------------------------------------------
//  MENU Functions
//  ----------------------------------------------------------------------------------------------------------------------------

void SelectCheckROM()
{
  uint32_t startval, endval;
  
  DisplayTop("<ROM CRC>");
  startval = SelectHex(0, 0x7FFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  
  CheckROM(startval, endval);
}
  
void SelectCheckROMInterleaved()
{
  uint32_t startval, endval;
  
  DisplayTop("<ROM CRC Intlv>");
  startval = SelectHex(0, 0x7FFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  
  CheckROMInterleaved(startval, endval);
}

void SelectCheckRAM()
{
  uint32_t startval, endval;
  
  DisplayTop("<Check RAM>");
  startval = SelectHex(0, 0x7FFFFE, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  
  CheckRAM(startval, endval);
}

void SelectCheckRAMPattern()
{
  uint32_t startval, endval;
  uint16_t pattern = 0xFFFF;
  
  DisplayTop("<Check RAM Ptn>");
  startval = SelectHex(0, 0x7FFFFE, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  pattern = SelectHex(0, 0xFFFF, 4, 1, 1, "pattern:");
  
  CheckRAMPattern(startval, endval, pattern);
}

void SelectCheckRAM8bit()
{
  uint32_t startval, endval;
  
  DisplayTop("<Check RAM 8>");
  startval = SelectHex(0, 0x7FFFFE, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  
  CheckRAM8bit(startval, endval);
}

void SelectCheckRAM8bitPattern()
{
  uint32_t startval, endval;
  uint8_t pattern = 0xFF;
  
  DisplayTop("<Check RAM 8Pt>");
  startval = SelectHex(0, 0x7FFFFE, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0x7FFFFF, 6, 1, 1, "  end:");
  pattern = SelectHex(0, 0xFF, 2, 1, 1, "pattern:");
  
  CheckRAM8bitPattern(startval, endval, pattern);
}

//  ----------------------------------------------------------------------------------------------------------------------------
//  User Interface
//  ----------------------------------------------------------------------------------------------------------------------------

void DisplayIntro()
{
  Display("Ghetto 68000", "Artemio 2017");
  delay(2000);
}


//  ----------------------------------------------------------------------------------------------------------------------------
//  Main Loop
//  ----------------------------------------------------------------------------------------------------------------------------

void setup() 
{
  initLCD();
  PrepareOutput();
  DisplayIntro();
  
  Serial.begin(115200);
}
                                 
void loop() 
{
  Display("Press SELECT", "to check PCB");  
  lcd_key = WaitKey();

  CheckROMInterleaved(00000, 0x1ffff);
  CheckRAM(0x30000, 0x33fff);
  CheckRAM(0x40000, 0x40fff);
  CheckRAM(0x50000, 0x50dff);
  CheckRAM8bit(0x7a000, 0x7abff);
 
  //if(lcd_key == btnSELECT)
    //SelectCheckRAM();
}


