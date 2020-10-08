//MyFRAM_SPI.h

#ifndef MyFRAM_SPI_h
#define MyFRAM_SPI_h

#include "Arduino.h"

#define FRAM_OPCODE_WREN  0b0110     // Write Enable Latch
#define FRAM_OPCODE_WRDI  0b0100     // Reset Write Enable Latch
#define FRAM_OPCODE_RDSR  0b0101     // Read Status Register
#define FRAM_OPCODE_WRSR  0b0001     // Write Status Register
#define FRAM_OPCODE_READ  0b0011     // Read Memory
#define FRAM_OPCODE_WRITE 0b0010     // Write Memory 
#define FRAM_OPCODE_RDID  0b10011111 // Read Device ID 

#define FRAM_OVERRUN true
#define FRAM_NO_OVERRUN false

#define SPI_CLOCK 8000000
#define SPI_MODE SPI_MODE0
#define SPI_BITORDER MSBFIRST

//#define FRAM_MAX_BUFFER_SIZE 30

class MyFRAM_SPI
{
  public:
    MyFRAM_SPI();
    MyFRAM_SPI(uint16_t);
    void begin(byte); //Kompatibilitaet zu alten Versionen
    void init(byte);
    void setSize(uint16_t);
    void setWriteEnableBit(bool);
    bool getWriteEnableBit(void);
    void writeByte(uint32_t, byte);
    void writeBuffer(uint32_t, byte*, byte, bool);
    bool verifyBuffer(uint32_t, byte*, byte, bool);
    void setStartAddress32Bit(uint32_t);
    void writeLong(byte, long);
    void writeUINT32T(byte, uint32_t);
    void writeFloat(byte, float);
    void writeDouble(byte, double);
    byte readByte(uint32_t);
    void readBuffer(uint32_t, byte*, byte, bool);
    long readLong(byte);
    uint32_t readUINT32T(byte);
    float readFloat(byte);
    double readDouble(byte);
    void clearAll();
    void clear(uint32_t);
    void clear(uint32_t, uint32_t);

    uint16_t framSize; //Speichergroesse in Byte
    
    

  private:
    void readSize(uint16_t&);
    void setWriteEnableLatch(bool);

    uint16_t _framSize_kBit;
    byte _cs;
    uint32_t startAddress32Bit = 0;
    bool writeEnableBit = true;
};

#endif