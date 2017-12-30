/*****************************************************************

     Ghetto Fluke 9010A 68k "pod"
         by Artemio Urbina 2017
         GNU GPL 2.0


     This is intended for Arcade PCB repair
     will have routines for ROM checksum,
     RAM checking and step by step addressing
     of a memory map.

     Version 0.1: Added RAM check routines and basic functions
 *****************************************************************/

#include "lcdsimp.h"
#include "crc32.h"


//  ----------------------------------------------------------------------------------------------------------------------------
//  M68000 interface pins
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

#define RW      13
#define AS      44
#define UDS_LDS 11
#define DTACK   3

int address_bus[ADDR_L] = { A01, A02, A03, A04, A05, A06, A07, A08, A09, A10,
                            A11, A12, A13, A14, A15, A16, A17, A18, A19, A20,
                            A21, A22, A23
                          };

int data_bus[DATA_L] = { D00, D01, D02, D03, D04, D05, D06, D07, D08, D09,
                         D10, D11, D12, D13, D14, D15
                       };


int err_DTACK_high = 0;

//  ----------------------------------------------------------------------------------------------------------------------------
//  Backend
//  ----------------------------------------------------------------------------------------------------------------------------

void SetDataWrite()
{
  int count = 0;

  for (count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], OUTPUT);
  err_DTACK_high = 0;
}

void SetDataRead()
{
  int count = 0;

  for (count = 0; count < DATA_L; count++)
    pinMode(data_bus[count], INPUT);
  err_DTACK_high = 0;
}

void PrepareOutput()
{
  int count = 0;

  for (count = 0; count < ADDR_L; count++)
    pinMode(address_bus[count], OUTPUT);

  SetDataRead();

  pinMode(DTACK, INPUT);

  pinMode(AS, OUTPUT);
  pinMode(RW, OUTPUT);
  pinMode(UDS_LDS, OUTPUT);

  for (count = 0; count < ADDR_L; count++)
    digitalWrite(address_bus[count], LOW);

  digitalWrite(AS, HIGH);
  digitalWrite(RW, HIGH);
  digitalWrite(UDS_LDS, HIGH); 
}

void SetAddress(uint32_t address)
{
  int pos = 0, b = 0, count = ADDR_L - 1, start = 0;;
  uint8_t bits[8];
  uint8_t cByte = 0;

  for (pos = 0; pos < 4; pos ++)
  {
    cByte = (address & (uint32_t)0xFF000000) >> 24;
    address = address << 8;
    for (b = 7; b > -1; b--)
    {
      if (start++ > 7) // We start at Address 23
      {
        bits[b] = (cByte & (1 << b)) != 0;
        if (bits[b] == 1)
          digitalWrite(address_bus[count], HIGH);
        else
          digitalWrite(address_bus[count], LOW);
        count --;
      }
    }
  }
}

inline void WaitForData()
{
  unsigned long _start = 0, _current;

  if(err_DTACK_high && digitalRead(DTACK) != LOW)
    return;
 
  _start = millis();
  while(digitalRead(DTACK) != LOW)
  {
    _current = millis();
    if(_current - _start >= 5000)
    {
        Display("No DTACK signal", "Ignoring");
        err_DTACK_high = 1;
        WaitKey();
        delay(1000);
        return;
    }
  }
  
  err_DTACK_high = 0;
}

inline void RequestDataReady()
{
  digitalWrite(UDS_LDS, LOW);
}

inline void EndRequestDataReady()
{
  digitalWrite(UDS_LDS, HIGH);
}

inline void RequestAddress()
{
  digitalWrite(AS, LOW);
}

inline void EndRequestAddress()
{
  digitalWrite(AS, HIGH);
}

inline void EnableRead()
{
  digitalWrite(RW, HIGH);
}

inline void EnableWrite()
{
  digitalWrite(RW, LOW);
}

inline void SetReadDataFromBus()
{
  SetDataRead();
  EnableRead();
}

inline void SetWriteDataToBus()
{
  SetDataWrite();
  EnableRead();
}

void SetData(uint16_t data)
{
  int pos = 0, b = 0, count = DATA_L - 1;
  uint8_t bits[8];
  uint8_t cByte = 0;

  for (pos = 0; pos < 2; pos ++)
  {
    cByte = (data & 0xFF00) >> 8;
    data = data << 8;
    for (b = 7; b > -1; b--)
    {
      bits[b] = (cByte & (1 << b)) != 0;
      if (bits[b] == 1)
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

  WaitForData();
  for (count = DATA_L - 1; count >= 0; count--)
    data = (data << 1) | digitalRead(data_bus[count]);

  return data;
}

inline uint16_t ReadDataFrom(uint32_t address)
{
  uint16_t data = 0;

  EnableRead();
  SetAddress(address);
  RequestAddress();
  RequestDataReady();

  data = ReadData();
  
  EndRequestDataReady();
  EndRequestAddress();
  
  return data;
}

inline void WriteDataTo(uint32_t address, uint16_t data)
{
  SetAddress(address);
  RequestAddress();
  EnableWrite();
  
  SetData(data);
  RequestDataReady();

  WaitForData();
  
  EndRequestDataReady();
  EndRequestAddress();
  EnableRead();

  SetData(0);
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

  SetReadDataFromBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    uint16_t data = 0;
    uint8_t mbyte = 0;

    data = ReadDataFrom(address);

    mbyte = data & 0x00FF;
    crc.update(mbyte);
    mbyte = (data & 0xFF00) >> 8;
    crc.update(mbyte);

    DisplayProgress(address);
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

  SetReadDataFromBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    uint16_t data = 0;
    uint8_t mbyte = 0;

    data = ReadDataFrom(address);
    
    mbyte = (data & 0xFF00) >> 8;
    crc1.update(mbyte);
    mbyte = data & 0x00FF;
    crc2.update(mbyte);
    
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
  uint16_t data = 0, pattern[2] = { 0xAA55, 0x55AA}, alter = 0;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);

  SetWriteDataToBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    WriteDataTo(address, pattern[alter]);
    alter = !alter;
    
    DisplayProgress(address);
  }

  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);

  alter = 0;
  SetReadDataFromBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    data = ReadDataFrom(address);
    
    if (data != pattern[alter])
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);

      sprintf(text, "R: %0.4X E: %0.4X", data, pattern[alter]);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    alter = !alter;
  
    DisplayProgress(address);
  }

  DisplayTop("RAM OK AA55-55AA");
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
  for (address = startAddress; address < endAddress; address += 2)
  {
    WriteDataTo(address, pattern);
    
    DisplayProgress(address);
  }

  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);

  SetReadDataFromBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    data = ReadDataFrom(address);

    if (data != pattern)
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);

      sprintf(text, "R: %0.4X E: %0.4X", data, pattern);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    DisplayProgress(address);
  }

  sprintf(text, "RAM OK - 0x%0.4X", pattern);
  DisplayTop(text);
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}

void CheckRAM8bit(uint32_t startAddress, uint32_t endAddress)
{
  uint32_t address = 0;
  uint16_t data = 0, pattern[2] = { 0x5A, 0xA5}, alter = 0;;
  char text[40];

  DisplayTop("<Write RAM> ...");
  StartProgress(startAddress, endAddress);

  SetWriteDataToBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    WriteDataTo(address, pattern[alter]);
    alter = !alter;

    DisplayProgress(address);
  }

  alter = 0;
  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);
  SetReadDataFromBus();

  for (address = startAddress; address < endAddress; address += 2)
  {
    data = ReadDataFrom(address);

    if ((data & 0x00FF) != pattern[alter])
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);

      sprintf(text, "R: %0.4X E: %0.4X", data, pattern[alter]);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    DisplayProgress(address);
    alter = !alter;
  }

  DisplayTop("RAM OK 0x5A-A5");
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
  for (address = startAddress; address < endAddress; address += 2)
  {
    WriteDataTo(address, pattern);
    
    DisplayProgress(address);
  }
  SetData(0);

  DisplayTop("<Read RAM> ...");
  ResetProgress(startAddress, endAddress);
  SetReadDataFromBus();

  for (address = startAddress; address < endAddress; address += 2)
  {
    data = ReadDataFrom(address);

    if ((data & 0x00FF) != (pattern & 0x00FF))
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);

      printf(text, "R: %0.4X E: %0.4X", data, pattern & 0x00FF);
      DisplayBottom(text);
      lcd_key = WaitKey();
      if (lcd_key == btnSELECT)
        return;
    }
    DisplayProgress(address);
  }

  sprintf(text, "RAM OK - 0x%0.2X", pattern & 0x00FF);
  DisplayTop(text);
  sprintf(text, "0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();
}


void CheckRAMContinuous(uint32_t startAddress, uint32_t endAddress)
{
  uint32_t address = 0;
  uint16_t data = 0, pattern[2] = { 0xFFFF, 0x0000}, alter = 0;
  char text[40];

  sprintf(text, "W:0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  WaitKey();

  StartProgress(startAddress, endAddress);

  SetWriteDataToBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    WriteDataTo(address, pattern[alter]);
    alter = !alter;

    DisplayProgress(address);
  }

  sprintf(text, "R:0x%0.6lX-%0.6lX", startAddress, endAddress);
  DisplayBottom(text);
  ResetProgress(startAddress, endAddress);
  WaitKey();

  alter = 0;
  SetReadDataFromBus();
  for (address = startAddress; address < endAddress; address += 2)
  {
    data = ReadDataFrom(address);

    if(data != pattern[alter])
    {
      sprintf(text, "Error @ 0x%0.6lX", address);
      DisplayTop(text);

      sprintf(text, "R: %0.4X E: %0.4X", data, pattern[alter]);
      DisplayBottom(text);

      delay(1000);
    }
    else
        DisplayProgress(address);
    alter = !alter;
  }
}


void ReadPort(uint32_t address, uint32_t count)
{
  char text[40];
  uint32_t counter = 0;
  CRC32 crc;

  DisplayTop("<Read Port> ...");
  StartProgress(0, count);

  SetReadDataFromBus();
  for (counter = 0; counter < count; counter ++)
  {
    uint16_t data = 0;
    
    data = ReadDataFrom(address);

    sprintf(text, "@0x%lX: 0x%X", address, data);
    DisplayTop(text);

    DisplayProgress(counter);
  }

  WaitKey();
}


void SelectAddressData()
{
  char text1[20], text2[20];
  const char *type[4] = { "Read", "Write", "Read w/increment", "Write w/increment" };
  int typeSel = 0, pos = 1;
  uint32_t address = 0, increment = 0;
  uint16_t data = 0;

  typeSel = displaymenu("Select Mode", type, 4);
  DisplayTop("");
  address = SelectHex(0, 0xFFFFFF, 6, 1, 1, "Addr:");
  if(typeSel == 1 || typeSel == 3)
    data = SelectHex(0, 0xFFFF, 4, 1, 1, "Val:");
  if(typeSel >= 2)
    increment = SelectHex(0, 0xFFFFFF, 6, 1, 1, "Incr:");
  do
  {
    if(typeSel == 0 || typeSel == 2)
    {
      SetReadDataFromBus();  
      
      EnableRead();
      SetAddress(address);
      RequestAddress();
      RequestDataReady();
  
      data = ReadData();
  
      sprintf(text1, "Read 0x%0.6lX:", address);
      DisplayTop(text1);
      sprintf(text2, "0x%0.4X", data);
      DisplayBottom(text2);
  
      lcd_key = WaitKey();  
      
      EndRequestDataReady();
      EndRequestAddress();

      data = 0;
    }
    else
    {
      SetWriteDataToBus();
      
      SetAddress(address);
      RequestAddress();
      EnableWrite();
      
      SetData(data);
      RequestDataReady();
    
      WaitForData();
      
      sprintf(text1, "Write 0x%0.6lX:", address);
      DisplayTop(text1);
      sprintf(text2, "0x%0.4X", data);
      DisplayBottom(text2);
  
      lcd_key = WaitKey();  
      
      EndRequestDataReady();
      EndRequestAddress();
      EnableRead();
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
  uint32_t startval, endval;

  DisplayTop("<ROM CRC>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");

  CheckROM(startval, endval);
}

void SelectCheckROMInterleaved()
{
  uint32_t startval, endval;

  DisplayTop("<ROM CRC Intlv>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");

  CheckROMInterleaved(startval, endval);
}

void SelectCheckRAM()
{
  uint32_t startval, endval;

  DisplayTop("<Check RAM>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");

  CheckRAM(startval, endval);
}

void SelectCheckRAMPattern()
{
  uint32_t startval, endval;
  uint16_t pattern = 0xFFFF;

  DisplayTop("<Check RAM Ptn>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");
  pattern = SelectHex(0, 0xFFFF, 4, 1, 1, "pattern:");

  CheckRAMPattern(startval, endval, pattern);
}

void SelectCheckRAM8bit()
{
  uint32_t startval, endval;

  DisplayTop("<Check RAM 8>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");

  CheckRAM8bit(startval, endval);
}

void SelectCheckRAM8bitPattern()
{
  uint32_t startval, endval;
  uint8_t pattern = 0xFF;

  DisplayTop("<Check RAM 8Pt>");
  startval = SelectHex(0, 0xFFFFFF, 6, 1, 1, "start:");
  endval = SelectHex(startval, 0xFFFFFF, 6, 1, 1, "  end:");
  pattern = SelectHex(0, 0xFF, 2, 1, 1, "pattern:");

  CheckRAM8bitPattern(startval, endval, pattern);
}


//  ----------------------------------------------------------------------------------------------------------------------------
//  User Interface
//  ----------------------------------------------------------------------------------------------------------------------------

void DisplayIntro()
{
  Display("Ghetto 68000", "Artemio 2017");
  delay(1000);
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
  SelectAddressData();

  //FixEight
  //CheckRAM(0x100000, 0x103fff);
  //CheckROM(0x000000, 0x080000);
  //CheckRAM8bit(0x280000, 0x28ffff);  //Fail MSD -> 8 bit RAM
  //CheckRAM(0x400000, 0x400fff); // WiIl FAIL Always, byteswapped/latched
  //CheckRAM(0x500000, 0x501fff);
  //CheckRAM8bit(0x600000, 0x60ffff);  //Fail MSD -> 8 bit RAM

  /*
  // Hishouzame
  
  CheckROMInterleaved(00000, 0x1ffff);
  CheckRAM(0x30000, 0x33fff);
  CheckRAM(0x40000, 0x40fff);
  CheckRAM(0x50000, 0x50dff);
  CheckRAM8bit(0x7a000, 0x7abff);
  */
  
}
