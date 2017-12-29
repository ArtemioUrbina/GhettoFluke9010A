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

#include "lcdsimp.h"
#include "crc32.h"

//  ----------------------------------------------------------------------------------------------------------------------------
//  M68000 interface pins 
//  ----------------------------------------------------------------------------------------------------------------------------
#define ADDR_L  16

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

#define DATA_L  8

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
#define BUSRQ   48
#define WAIT    50
#define BUSAK   52
#define WR      2
#define RD      3

int address_bus[] = { A00, A01, A02, A03, A04, A05, A06, A07, A08, A09, 
                      A10, A11, A12, A13, A14, A15 };

int data_bus[] = { D00, D01, D02, D03, D04, D05, D06, D07 };

int err_WAIT_low = 0;


//  ----------------------------------------------------------------------------------------------------------------------------
//  User Interface
//  ----------------------------------------------------------------------------------------------------------------------------

void DisplayIntro()
{
  Display("Ghetto Z80", "Artemio 2017");  
  delay(1500);
}

//  ----------------------------------------------------------------------------------------------------------------------------
//  Backend
//  ----------------------------------------------------------------------------------------------------------------------------

void SetDataWrite()
{
  int count = 0;

  for(count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], OUTPUT); 

  err_WAIT_low = 0;
}

void SetDataRead()
{
  int count = 0;

  for(count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], INPUT); 

  err_WAIT_low = 0;
}

void PrepareOutput()
{
  int count = 0;

  for(count = 0; count < ADDR_L; count++)
    pinMode(address_bus[count], OUTPUT); 

  SetDataRead(); 

  pinMode(MREQ, OUTPUT); 
  pinMode(IORQ, OUTPUT); 
  pinMode(RFSH, OUTPUT); 
  pinMode(M1, OUTPUT); 
  pinMode(WR, OUTPUT); 
  pinMode(RD, OUTPUT); 
  pinMode(BUSAK, OUTPUT);
  
  pinMode(BUSRQ, INPUT);
  pinMode(RESET, INPUT); 
  pinMode(WAIT, INPUT);
  
  
  for(count = 0; count < ADDR_L; count++)
    digitalWrite(address_bus[count], LOW);

  digitalWrite(MREQ, HIGH);
  digitalWrite(IORQ, HIGH);
  digitalWrite(RFSH, HIGH);
  digitalWrite(M1, HIGH);
  digitalWrite(WR, HIGH);
  digitalWrite(RD, HIGH);
  digitalWrite(BUSAK, HIGH);
}

void SetAddress(uint16_t address)
{
  int pos = 0, b = 0, count = ADDR_L - 1;
  uint8_t bits[8];
  uint8_t cByte = 0;

  for(pos = 0; pos < 2; pos ++)
  {
    cByte = (address & 0xFF00) >> 8;
    address = address << 8;
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

inline void SampleBUSRQ()
{
  unsigned long _start = 0, _current;

  _start = millis();
  while(digitalRead(BUSRQ) == LOW)
  {
    digitalWrite(BUSAK, LOW);
    _current = millis();
    if(_current - _start >= 5000)
    {
        Display("BUSRQ is LOW", "Ignoring");
        WaitKey();
        delay(1000);
        //digitalWrite(BUSAK, HIGH);
        return;
    }
  }
  digitalWrite(BUSAK, HIGH);
}

inline void WaitForData()
{
  unsigned long _start = 0, _current;

  if(err_WAIT_low && digitalRead(WAIT) != HIGH)
    return;
 
  _start = millis();
  while(digitalRead(WAIT) != HIGH)
  {
    _current = millis();
    if(_current - _start >= 5000)
    {
        Display("No WAIT signal", "Ignoring");
        err_WAIT_low = 1;
        WaitKey();
        delay(1000);
        return;
    }
  }
  
  err_WAIT_low = 0;
}

inline void EnableRead()
{
  digitalWrite(RD, LOW);
  digitalWrite(WR, HIGH);
}

inline void EnableWrite()
{
  digitalWrite(RD, HIGH);
  digitalWrite(WR, LOW);
}

inline void RequestAddress()
{
  digitalWrite(MREQ, LOW);
}

inline void EndRequestAddress()
{
  digitalWrite(MREQ, HIGH);
}

inline void RequestPortAddress()
{
  digitalWrite(IORQ, LOW);
}

inline void EndRequestPortAddress()
{
  digitalWrite(IORQ, HIGH);
}

inline void SetRealeaseDataBus()
{
  digitalWrite(RD, HIGH);
  digitalWrite(WR, HIGH);
}

inline void SetReadDataFromBus()
{
  SetDataRead(); 
  EnableRead();
}

inline void SetWriteDataToBus()
{
  SetDataWrite(); 
  EnableWrite();
}

void SetData(uint8_t data)
{
  int b = 0;
  uint8_t bits[8];
  
  for (b = DATA_L - 1; b > -1; b--) 
  {  
    bits[b] = (data & (1 << b)) != 0;
    if(bits[b] == 1)
      digitalWrite(data_bus[b], HIGH);
    else
      digitalWrite(data_bus[b], LOW);
  }
}

uint8_t ReadData()
{
  int count = 0;
  uint8_t data = 0;

  WaitForData();
  for(count = DATA_L - 1; count >= 0; count--)
    data = (data << 1) | digitalRead(data_bus[count]);
  
  return data;
}

inline uint8_t ReadDataFrom(uint16_t address)
{
  uint8_t data = 0;

  SampleBUSRQ();
  
  SetAddress(address);
  RequestAddress();
  EnableRead();

  data = ReadData();

  SetRealeaseDataBus();
  EndRequestAddress();
  
  return data;
}

inline uint8_t ReadDataFromPort(uint8_t address)
{
  uint8_t data = 0;

  SampleBUSRQ();

  SetAddress(address);
  RequestPortAddress();
  EnableRead();

  data = ReadData();

  SetRealeaseDataBus();
  EndRequestPortAddress();
  
  return data;
}

inline void WriteDataTo(uint16_t address, uint8_t data)
{
  SampleBUSRQ();
  
  SetAddress(address);
  RequestAddress();
  EnableWrite();
  
  SetData(data);

  WaitForData();

  SetRealeaseDataBus();
  EndRequestAddress();

  SetData(0);
}

inline void WriteDataToPort(uint8_t address, uint8_t data)
{
  SampleBUSRQ();
  
  SetAddress(address);
  RequestPortAddress();
  EnableWrite();
  
  SetData(data);

  WaitForData();

  SetRealeaseDataBus();
  EndRequestPortAddress();

  SetData(0);
}


void CheckROM(uint16_t startAddress, uint16_t endAddress)
{
  char text[40];
  uint16_t address;
  uint32_t checksum;
  CRC32 crc;

  DisplayTop("<ROM CRC> ...");
  StartProgress(startAddress, endAddress);

  SetReadDataFromBus();
  for (address = startAddress; address <= endAddress; address ++)
  {
    uint8_t data = 0;

    data = ReadDataFrom(address);

    Serial.println(data, HEX);
    crc.update(data);

    DisplayProgress(address);
  }

  checksum = crc.finalize();
  sprintf(text, "ROM: 0x%lX", checksum);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAM(uint16_t startAddress, uint16_t endAddress)
{
  uint16_t address = 0;
  uint8_t data = 0, pattern[2] = { 0x5A, 0xA5}, alter = 0;;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);

  SetWriteDataToBus();
  for (address = startAddress; address <= endAddress; address ++)
  {
    WriteDataTo(address, pattern[alter]);
    alter = !alter;

    DisplayProgress(address);
  }

  alter = 0;
  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);
  SetReadDataFromBus();

  for (address = startAddress; address <= endAddress; address ++)
  {
    data = ReadDataFrom(address);

    if ((data & 0x00FF) != pattern[alter])
    {
      sprintf(text, "Error @ 0x%0.4X", address);
      DisplayTop(text);

      sprintf(text, "R: %0.2X E: %0.2X", data, pattern[alter]);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    DisplayProgress(address);
    alter = !alter;
  }

  DisplayTop("RAM OK 0x5A-A5");
  sprintf(text, "0x%0.4X-%0.4X", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAMPattern(uint16_t startAddress, uint16_t endAddress, uint8_t pattern)
{
  uint16_t address = 0;
  uint8_t data = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);

  SetWriteDataToBus();
  for (address = startAddress; address <= endAddress; address ++)
  {
    WriteDataTo(address, pattern);
    
    DisplayProgress(address);
  }
  SetData(0);

  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);
  SetReadDataFromBus();

  for (address = startAddress; address <= endAddress; address ++)
  {
    data = ReadDataFrom(address);

    if ((data & 0x00FF) != (pattern & 0x00FF))
    {
      sprintf(text, "Error @ 0x%0.4X", address);
      DisplayTop(text);

      printf(text, "R: %0.2X E: %0.2X", data, pattern & 0x00FF);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    DisplayProgress(address);
  }

  sprintf(text, "RAM OK - 0x%0.2X", pattern & 0x00FF);
  DisplayTop(text);
  sprintf(text, "0x%0.4X-%0.4X", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void SelectPortData()
{
  char text1[20], text2[20];
  const char *type[4] = { "Read", "Write", "Read w/increment", "Write w/increment" };
  int typeSel = 0, pos = 1;
  uint8_t address = 0, increment = 0;
  uint8_t data = 0;

  typeSel = displaymenu("Select Mode", type, 4);
  DisplayTop("");
  address = SelectHex(0, 0xFF, 2, 1, 1, "Addr:");
  if(typeSel == 1 || typeSel == 3)
    data = SelectHex(0, 0xFF, 2, 1, 1, "Val:");
  if(typeSel >= 2)
    increment = SelectHex(0, 0xFF, 2, 1, 1, "Incr:");
  do
  {
    if(typeSel == 0 || typeSel == 2)
    {
      SetReadDataFromBus();  
      
      SetAddress(address);
      RequestPortAddress();
      EnableRead();
  
      data = ReadData();
  
      sprintf(text1, "Read 0x%0.2X:", address);
      DisplayTop(text1);
      sprintf(text2, "0x%0.2X", data);
      DisplayBottom(text2);
  
      lcd_key = WaitKey();  

      SetRealeaseDataBus();
      EndRequestPortAddress();

      data = 0;
    }
    else
    {
      SetWriteDataToBus();
      
      SetAddress(address);
      RequestPortAddress();
      EnableWrite();
      
      SetData(data);
    
      WaitForData();
      
      sprintf(text1, "Write 0x%0.2X:", address);
      DisplayTop(text1);
      sprintf(text2, "0x%0.2X", data);
      DisplayBottom(text2);
  
      lcd_key = WaitKey();  

      SetRealeaseDataBus();
      EndRequestPortAddress();
    }

    if(typeSel >= 2)
        address += increment;
  }
  while(lcd_key == btnSELECT);
}

//  ----------------------------------------------------------------------------------------------------------------------------
//  MENU Functions
//  ----------------------------------------------------------------------------------------------------------------------------

void SelectCheckROM()
{
  uint16_t startval, endval;

  DisplayTop("<ROM CRC>");
  startval = SelectHex(0, 0xFFFF, 4, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFF, 4, 1, 1, "  end:");

  CheckROM(startval, endval);
}

void SelectCheckRAM()
{
  uint16_t startval, endval;

  DisplayTop("<Check RAM>");
  startval = SelectHex(0, 0xFFFF, 4, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFF, 4, 1, 1, "  end:");

  CheckRAM(startval, endval);
}

void SelectCheckRAMPattern()
{
  uint16_t startval, endval;
  uint8_t pattern = 0xFF;

  DisplayTop("<Check RAM 8Pt>");
  startval = SelectHex(0, 0xFFFF, 4, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFF, 4, 1, 1, "  end:");
  pattern = SelectHex(0, 0xFF, 2, 1, 1, "pattern:");

  CheckRAMPattern(startval, endval, pattern);
}

void MainMenu()
{
  const char *type[5] = { "Check ROM", "Check RAM", "Check RAM Pattn", "R/W Port", "Game tests" };
  int typeSel = 0;

  typeSel = displaymenu("Select Test", type, 5);
  switch(typeSel)
  {
    case 0:
      SelectCheckROM();
      break;
    case 1:
      SelectCheckRAM();
      break;
    case 2:
      SelectCheckRAMPattern();
      break;
    case 3:
      SelectPortData();
      break;
    case 4:
      GameTests();
      break;
  }
}

void GameTests()
{
  const char *type[1] = { "Hishouzame" };
  int typeSel = 0;

  typeSel = displaymenu("Select Test", type, 1);
  switch(typeSel)
  {
    case 0:
      TestHishouza();
      break;
  }
}

void TestHishouza()
{
  //0000-7fff ROM
  CheckROM(0, 0x7fff);
  //8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side
  CheckRAM(0x8000, 0x87ff);
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
  MainMenu();
}


