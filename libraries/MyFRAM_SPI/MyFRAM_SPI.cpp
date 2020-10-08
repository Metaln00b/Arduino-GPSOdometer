//MyFRAM_SPI.cpp
//Code fuer Arduino
//Author Retian
//Version 1.2


/*
Ansteuerung eines FRAM mit SPI-Schnittstelle bis maximal 512 kBit
(getestet mit 64 kBit Adafruit Breakout Board)

MyFRAM_SPI Name(FRAM_Groesse) //Groesse in kBit

Beispiel siehe unter:
http://arduino-projekte.webnode.at/meine-libraries/fram-speicher/

Funktionen siehe unter:
http://arduino-projekte.webnode.at/meine-libraries/fram-speicher/funktionen/

*/

#include "Arduino.h"
#include "MyFRAM_SPI.h"
#include "SPI.h"


MyFRAM_SPI::MyFRAM_SPI(uint16_t framSize_kBit)
{
  _framSize_kBit = framSize_kBit; //Groesse in kBit
}

MyFRAM_SPI::MyFRAM_SPI(): _framSize_kBit(0)
{
}

//***************************************************************
//Initialisierung der Library

void MyFRAM_SPI::init(byte cs)
{
  _cs = cs;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  SPI.begin();

  if (_framSize_kBit == 0) readSize(_framSize_kBit);
  framSize = (_framSize_kBit << 7); //Groesse in Byte
}

//Aus Kompatibilitaetsgruenden zu alten Versionen
void MyFRAM_SPI::begin(byte cs)
{
  init(cs);
}

//***************************************************************
//Aendern der FRAM-Speichergroesse

void MyFRAM_SPI::setSize(uint16_t framSize_kBit)
{
  _framSize_kBit = framSize_kBit;
 
  framSize = (_framSize_kBit << 7); //Groesse in Byte
}

//***************************************************************
//Setzen/Ruecksetzen der Schreibfreigabe
//(interne Verwendung)

void MyFRAM_SPI::setWriteEnableLatch(bool enable)
{
  byte _enable = enable;

  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  if (_enable) SPI.transfer(FRAM_OPCODE_WREN);
  else SPI.transfer(FRAM_OPCODE_WRDI);
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

//***************************************************************
//Setzen des Write Enable Bit

void MyFRAM_SPI::setWriteEnableBit(bool enable)
{
  writeEnableBit = enable;	
}

//***************************************************************
//Abfrage des Write Enable Bit

bool MyFRAM_SPI::getWriteEnableBit(void)
{
  return writeEnableBit;
}

//***************************************************************
//Schreiben eines Bytes ins FRAM

void MyFRAM_SPI::writeByte(uint32_t framAddress, byte val)
{
  uint32_t _framAddress = framAddress;
  byte _val = val;
  
  //Pruefe, ob FRAM-Adresse innerhalb des Speicherbereichs ist
  if (_framAddress >= framSize) return;
  
  setWriteEnableLatch(writeEnableBit);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_WRITE);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  SPI.transfer(_val);
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

//***************************************************************
//Schreiben einer Bytefolge aus einem Buffer-Array ins FRAM

void MyFRAM_SPI::writeBuffer(uint32_t framAddress, byte* buffer, byte anzByte, bool overRun)
{
  uint32_t _framAddress = framAddress;
  byte* _buffer = buffer;
  byte _anzByte = anzByte;
  bool _overRun = overRun;
  
  //Pruefe, ob FRAM-Adresse innerhalb des Speicherbereichs ist
  if (_framAddress >= framSize) return;
  
  //Wenn kein Speicherueberlauf (NO_OVERRUN) gewuenscht:
  if (!_overRun)
  {
    if (_framAddress + _anzByte > framSize)
    {
      _anzByte = framSize - _framAddress;
    }
  }
  setWriteEnableLatch(writeEnableBit);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_WRITE);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  for (byte i = 0; i < _anzByte; i++) SPI.transfer(_buffer[i]);
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

//***************************************************************
//Vergleichen des Buffer-Array mit dem FRAM

bool MyFRAM_SPI::verifyBuffer(uint32_t framAddress, byte* buffer, byte anzByte, bool overRun)
{
  uint32_t _framAddress = framAddress;
  byte* _buffer = buffer;
  byte _anzByte = anzByte;
  bool _overRun = overRun;
  
  //Pruefe, ob FRAM-Adresse innerhalb des Speicherbereichs ist
  if (_framAddress >= framSize) return false;
  
  //Wenn kein Speicherueberlauf (NO_OVERRUN) gewuenscht:
  if (!_overRun)
  {
    if (_framAddress + _anzByte > framSize)
    {
      _anzByte = framSize - _framAddress;
    }
  }
  byte val;
  bool verify = true;
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_READ);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  for (byte i = 0; i < _anzByte; i++)
  {
    val = SPI.transfer(0);
    if (val != _buffer[i])
    {
      verify = false;
      break;
    }
  }
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
  return verify;
}

//***************************************************************
//Setzen der FRAM-Adresse, ab der Long- oder Float-Variable gespeichert werden

void MyFRAM_SPI::setStartAddress32Bit(uint32_t add)
{
  startAddress32Bit = add;
}

//***************************************************************
//Schreiben einer Integer-Zahl (Long-Format) ins FRAM

void MyFRAM_SPI::writeLong(byte id, long val)
{
  byte _id = id;
  long _val = val;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(long);

  long* p_val = &_val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(long)];
  
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = *(p_dummy + i);
  }
  setWriteEnableLatch(writeEnableBit);
  writeBuffer(add, buffer, sizeof(buffer), 0);
}

//***************************************************************
//Schreiben einer Integer-Zahl (uint32_t-Format) ins FRAM

void MyFRAM_SPI::writeUINT32T(byte id, uint32_t val)
{
  byte _id = id;
  uint32_t _val = val;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(uint32_t);

  uint32_t* p_val = &_val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(uint32_t)];
  
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = *(p_dummy + i);
  }
  setWriteEnableLatch(writeEnableBit);
  writeBuffer(add, buffer, sizeof(buffer), 0);
}

//***************************************************************
//Schreiben einer Float-Zahl ins FRAM

void MyFRAM_SPI::writeFloat(byte id, float val)
{
  byte _id = id;
  float _val = val;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(float);

  float* p_val = &_val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(float)];
  
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = *(p_dummy + i);
  }
  setWriteEnableLatch(writeEnableBit);
  writeBuffer(add, buffer, sizeof(buffer), 0);
}

//***************************************************************
//Schreiben einer Double-Zahl ins FRAM

void MyFRAM_SPI::writeDouble(byte id, double val)
{
  byte _id = id;
  double _val = val;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(double);

  double* p_val = &_val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(double)];
  
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = *(p_dummy + i);
  }
  setWriteEnableLatch(writeEnableBit);
  writeBuffer(add, buffer, sizeof(buffer), 0);
}

//***************************************************************
//Lesen eines Bytes aus dem FRAM

byte MyFRAM_SPI::readByte(uint32_t framAddress)
{ 
  uint32_t _framAddress = framAddress;
  byte val;
  
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_READ);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  val = SPI.transfer(0);
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
  return val;
}

//***************************************************************
//Lesen einer Bytefolge aus dem FRAM in ein Buffer-Array

void MyFRAM_SPI::readBuffer(uint32_t framAddress, byte* buffer, byte anzByte, bool overRun)
{
  uint32_t _framAddress = framAddress;
  byte* _buffer = buffer;
  byte _anzByte = anzByte;
  bool _overRun = overRun;
  
  //Pruefe, ob FRAM-Adresse innerhalb des Speicherbereichs ist
  if (_framAddress >= framSize) return;
  
  //Wenn kein Speicherueberlauf (NO_OVER_RUN) gewuenscht:
  if (!_overRun)
  {
    if (_framAddress + _anzByte > framSize)
    {
      _anzByte = framSize - _framAddress;
    }
  }
  
  //Loesche Buffer
  for (byte i = 0; i < _anzByte; i++) _buffer[i] = '\0';

  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_READ);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  for (byte i = 0; i < _anzByte; i++)
  {
    buffer[i] = SPI.transfer(0);
  }
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

//***************************************************************
//Lesen einer Integer-Zahl (Long-Format) aus dem FRAM

long MyFRAM_SPI::readLong(byte id)
{
  byte _id = id;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(long);
  long val;
  long* p_val = &val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(long)];
  
  readBuffer(add, buffer, sizeof(buffer), 0);
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    *(p_dummy + i) = buffer[i];
  }
  return val;
}

//***************************************************************
//Lesen einer Integer-Zahl (uint32_t-Format) aus dem FRAM

uint32_t MyFRAM_SPI::readUINT32T(byte id)
{
  byte _id = id;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(uint32_t);
  uint32_t val;
  uint32_t* p_val = &val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(uint32_t)];
  
  readBuffer(add, buffer, sizeof(buffer), 0);
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    *(p_dummy + i) = buffer[i];
  }
  return val;
}

//***************************************************************
//Lesen einer Float-Zahl aus dem FRAM

float MyFRAM_SPI::readFloat(byte id)
{
  byte _id = id;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(float);
  float val;
  float* p_val = &val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(float)];
  
  readBuffer(add, buffer, sizeof(buffer), 0);
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    *(p_dummy + i) = buffer[i];
  }
  return val;
}

//***************************************************************
//Lesen einer Double-Zahl aus dem FRAM

double MyFRAM_SPI::readDouble(byte id)
{
  byte _id = id;
  uint32_t add = startAddress32Bit + (_id - 1) * sizeof(double);
  double val;
  double* p_val = &val;
  byte* p_dummy = (byte*)p_val;
  byte buffer[sizeof(double)];
  
  readBuffer(add, buffer, sizeof(buffer), 0);
  for (byte i = 0; i < sizeof(buffer); i++)
  {
    *(p_dummy + i) = buffer[i];
  }
  return val;
}

//***************************************************************
//Loeschen des gesamten FRAMs

void MyFRAM_SPI::clearAll()
{
  clear(0, framSize - 1);
}

//***************************************************************
//Loeschen eines Bytes

void MyFRAM_SPI::clear(uint32_t framAddress)
{
  uint32_t _framAddress = framAddress;
  
  writeByte(_framAddress, 0);
}

//***************************************************************
//Loeschen eines FRAM-Bereiches

void MyFRAM_SPI::clear(uint32_t framAddress, uint32_t framAddressEnd)
{
  uint32_t _framAddress = framAddress;
  uint32_t _framAddressEnd = framAddressEnd;
  
  //Pruefe, ob FRAM-Adresse innerhalb des Speicherbereichs ist
  if (_framAddressEnd < _framAddress ||
  _framAddressEnd > framSize) return;
  
  uint16_t anzByteClear = _framAddressEnd - _framAddress + 1;

  setWriteEnableLatch(writeEnableBit);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_WRITE);
  SPI.transfer(_framAddress >> 8);
  SPI.transfer(_framAddress);
  for (uint16_t i = 0; i < anzByteClear; i++)
  {
    SPI.transfer(0);
  }
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

//***************************************************************
//FRAM-Speichergroesse auslesen in kBit
//(interne Verwendung)

void MyFRAM_SPI::readSize(uint16_t &size_kBit)
{
  uint16_t& _size_kBit = size_kBit;
  byte val;
  
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(SPISettings(SPI_CLOCK, SPI_BITORDER, SPI_MODE));
  SPI.transfer(FRAM_OPCODE_RDID);
  for (byte i = 0; i < 3; i++)
  {
    val = SPI.transfer(0);
  }
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
  val &= 0x1F;
 
  switch (val)
  {
    case 1:
      _size_kBit = 16;
      break;
    case 2:
      _size_kBit = 32;
      break;
    case 3:
      _size_kBit = 64;
      break;
    case 4:
      _size_kBit = 128;
      break;
    case 5:
      _size_kBit = 256;
      break;
    case 6:
      _size_kBit = 512;
      break;
     /* Wird derzeit noch nicht benötigt
    case 7:
      _size_kBit = 1024;
      break;
    case 8:
      _size_kBit = 2048;
      break;
      */
    default:
      _size_kBit = 0;
      break;
  }
}
