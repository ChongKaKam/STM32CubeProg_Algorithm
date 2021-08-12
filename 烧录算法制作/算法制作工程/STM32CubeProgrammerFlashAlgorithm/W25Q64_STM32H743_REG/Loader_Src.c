#include "sys.h"
#include "sys.h"
#include "delay.h"
#include "w25qxx.h"
#include "led.h"
#include "qspi.h"
#include "usart.h"

#define		SPI_FLASH_CHECK		0x0FFFFFFF

/**
  * Description :
  * Initilize the MCU Clock, the GPIO Pins corresponding to the
  * device and initilize the FSMC with the chosen configuration
  * Inputs    :
  *      None
  * outputs   :
  *      R0             : "1" 			: Operation succeeded
  * 			  "0" 			: Operation failure
  * Note: Mandatory for all types of device
  */
int Init (void)
{
    __disable_irq(); 
    Stm32_Clock_Init(100, 1, 2, 2); //设置时钟,400Mhz
    delay_init(400);				//延时初始化
    W25QXX_Init();		            //初始化W25QXX
    
    return 1;
}


/**
  * Description :
  * Read data from the device
  * Inputs    :
  *      Address       : Write location
  *      Size          : Length in bytes
  *      buffer        : Address where to get the data to write
  * outputs   :
  *      R0             : "1" 			: Operation succeeded
  * 			  "0" 			: Operation failure
  * Note: Mandatory for all types except SRAM and PSRAM
  */
int Read (uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    Address = Address & SPI_FLASH_CHECK;
    W25QXX_Read(buffer, Address, Size);
    return 1;
}


/**
  * Description :
  * Write data from the device
  * Inputs    :
  *      Address       : Write location
  *      Size          : Length in bytes
  *      buffer        : Address where to get the data to write
  * outputs   :
  *      R0           : "1" 			: Operation succeeded
  *                     "0" 			: Operation failure
  * Note: Mandatory for all types except SRAM and PSRAM
  */
int Write (uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    Address = Address & SPI_FLASH_CHECK;
    W25QXX_Write_NoCheck(buffer, Address, Size);
    return 1;
}


/**
  * Description :
  * Erase a full sector in the device
  * Inputs    :
  *     None
  * outputs   :
  *     R0             : "1" : Operation succeeded
  * 			 "0" : Operation failure
  * Note: Not Mandatory for SRAM PSRAM and NOR_FLASH
  */
int MassErase (void)
{
    W25QXX_Erase_Chip();
    return 1;
}

/**
  * Description :
  * Erase a full sector in the device
  * Inputs    :
  *      SectrorAddress	: Start of sector
  *      Size          : Size (in WORD)
  *      InitVal       : Initial CRC value
  * outputs   :
  *     R0             : "1" : Operation succeeded
  * 			 "0" : Operation failure
  * Note: Not Mandatory for SRAM PSRAM and NOR_FLASH
  */
int SectorErase (uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    EraseStartAddress = EraseStartAddress & SPI_FLASH_CHECK;
    EraseEndAddress = EraseEndAddress & SPI_FLASH_CHECK;
    EraseStartAddress = EraseStartAddress -  EraseStartAddress % W25QxJV_SUBSECTOR_SIZE;    //Erase 4KBytes

    while (EraseEndAddress >= EraseStartAddress)
    {
        W25QXX_Erase_Sector(EraseStartAddress);
        EraseStartAddress += W25QxJV_SUBSECTOR_SIZE;        //Erase 4KBytes
    }

    return 1;
}

/**
  * Description :
  * Calculates checksum value of the memory zone
  * Inputs    :
  *      StartAddress  : Flash start address
  *      Size          : Size (in WORD)
  *      InitVal       : Initial CRC value
  * outputs   :
  *     R0             : Checksum value
  * Note: Optional for all types of device
  */
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
    uint8_t missalignementAddress = StartAddress % 4;
    uint8_t missalignementSize = Size ;
    int cnt;
    uint32_t Val;
    uint8_t value;
    StartAddress = StartAddress & SPI_FLASH_CHECK;
    StartAddress -= StartAddress % 4;
    Size += (Size % 4 == 0) ? 0 : 4 - (Size % 4);

    for(cnt = 0; cnt < Size ; cnt += 4)
    {
        W25QXX_Read(&value, StartAddress, 1);
        Val = value;
        W25QXX_Read(&value, StartAddress + 1, 1);
        Val += value << 8;
        W25QXX_Read(&value, StartAddress + 2, 1);
        Val += value << 16;
        W25QXX_Read(&value, StartAddress + 3, 1);
        Val += value << 24;

        if(missalignementAddress)
        {
            switch (missalignementAddress)
            {
                case 1:
                    InitVal += (uint8_t) (Val >> 8 & 0xff);
                    InitVal += (uint8_t) (Val >> 16 & 0xff);
                    InitVal += (uint8_t) (Val >> 24 & 0xff);
                    missalignementAddress -= 1;
                    break;

                case 2:
                    InitVal += (uint8_t) (Val >> 16 & 0xff);
                    InitVal += (uint8_t) (Val >> 24 & 0xff);
                    missalignementAddress -= 2;
                    break;

                case 3:
                    InitVal += (uint8_t) (Val >> 24 & 0xff);
                    missalignementAddress -= 3;
                    break;
            }
        }
        else if((Size - missalignementSize) % 4 && (Size - cnt) <= 4)
        {
            switch (Size - missalignementSize)
            {
                case 1:
                    InitVal += (uint8_t) Val;
                    InitVal += (uint8_t) (Val >> 8 & 0xff);
                    InitVal += (uint8_t) (Val >> 16 & 0xff);
                    missalignementSize -= 1;
                    break;

                case 2:
                    InitVal += (uint8_t) Val;
                    InitVal += (uint8_t) (Val >> 8 & 0xff);
                    missalignementSize -= 2;
                    break;

                case 3:
                    InitVal += (uint8_t) Val;
                    missalignementSize -= 3;
                    break;
            }
        }
        else
        {
            InitVal += (uint8_t) Val;
            InitVal += (uint8_t) (Val >> 8 & 0xff);
            InitVal += (uint8_t) (Val >> 16 & 0xff);
            InitVal += (uint8_t) (Val >> 24 & 0xff);
        }

        StartAddress += 4;
    }

    return (InitVal);
}


/**
  * Description :
  * Verify flash memory with RAM buffer and calculates checksum value of
  * the programmed memory
  * Inputs    :
  *      FlashAddr     : Flash address
  *      RAMBufferAddr : RAM buffer address
  *      Size          : Size (in WORD)
  *      InitVal       : Initial CRC value
  * outputs   :
  *     R0             : Operation failed (address of failure)
  *     R1             : Checksum value
  * Note: Optional for all types of device
  */
uint64_t Verify (uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement)
{
    uint32_t InitVal = 0;
    uint32_t VerifiedData = 0;
    uint8_t TmpBuffer = 0x00;
    uint64_t checksum;
    Size *= 4;
    MemoryAddr = MemoryAddr & SPI_FLASH_CHECK;
    checksum = CheckSum((uint32_t)MemoryAddr + (missalignement & 0xf), Size - ((missalignement >> 16) & 0xF), InitVal);

    while (Size > VerifiedData)
    {
        W25QXX_Read(&TmpBuffer, MemoryAddr + VerifiedData, 1);

        if (TmpBuffer != *((uint8_t*)RAMBufferAddr + VerifiedData))
            return ((checksum << 32) + MemoryAddr + VerifiedData);

        VerifiedData++;
    }

    return (checksum << 32);
}

