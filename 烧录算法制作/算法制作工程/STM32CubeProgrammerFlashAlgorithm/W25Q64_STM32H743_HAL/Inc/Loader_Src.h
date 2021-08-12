/**
  ******************************************************************************
  * @file    Loader_Src.h
  * @author  MCD Tools Team
  * @date    October-2015
  * @brief   Loader Header file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LOADER_SRC_H
#define __LOADER_SRC_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/


#if defined(__CC_ARM)
extern uint32_t Load$$QSPI$$Base;
extern uint32_t Load$$QSPI$$Length;
#elif defined(__ICCARM__)
#pragma section =".qspi"
#pragma section =".qspi_init"
#elif defined(__GNUC__)
extern uint32_t _qspi_init_base;
extern uint32_t _qspi_init_length;
#endif


#ifdef __ICCARM__                //IAR
#define KeepInCompilation __root 
#elif __CC_ARM                   //MDK-ARM
#define KeepInCompilation __attribute__((used))
#else //TASKING               //TrueStudio
#define KeepInCompilation __attribute__((used))
#endif

/* W25QxJV Micron memory */
/* Size of the flash */
#define QSPI_FLASH_SIZE            24     /* 地址总线宽度访问整个内存空间 */
#define QSPI_PAGE_SIZE             256

/*命令定义-开头*******************************/
/**
  * @brief  W25QxJV配置
  */
#define W25QxJV_FLASH_SIZE                  0x2000000  	/* 256 MBits => 32MBytes */
#define W25QxJV_SECTOR_SIZE                 0x10000   	/* 512 sectors of 64KBytes */
#define W25QxJV_SUBSECTOR_SIZE              0x1000    	/* 2048 subsectors of 4kBytes */
#define W25QxJV_PAGE_SIZE                   0x100     	/* 65536 pages of 256 bytes */

#define W25QxJV_DUMMY_CYCLES_READ           4
#define W25QxJV_DUMMY_CYCLES_READ_QUAD      10

#define W25QxJV_BULK_ERASE_MAX_TIME         250000
#define W25QxJV_SECTOR_ERASE_MAX_TIME       3000
#define W25QxJV_SUBSECTOR_ERASE_MAX_TIME    800

/**
  * @brief  W25QxJV 指令
  */
/* 复位操作 */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

#define ENTER_QPI_MODE_CMD                   0x38
#define EXIT_QPI_MODE_CMD                    0xFF

/* 识别操作 */
#define READ_ID_CMD                          0x90
#define DUAL_READ_ID_CMD                     0x92
#define QUAD_READ_ID_CMD                     0x94
#define READ_JEDEC_ID_CMD                    0x9F

/* 读操作 */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define DUAL_OUT_FAST_READ_CMD               0x3B
#define DUAL_INOUT_FAST_READ_CMD             0xBB
#define QUAD_OUT_FAST_READ_CMD               0x6B
#define QUAD_INOUT_FAST_READ_CMD             0xEB

/* 写操作 */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* 寄存器操作 */
#define READ_STATUS_REG1_CMD                  0x05
#define READ_STATUS_REG2_CMD                  0x35
#define READ_STATUS_REG3_CMD                  0x15

#define WRITE_STATUS_REG1_CMD                 0x01
#define WRITE_STATUS_REG2_CMD                 0x31
#define WRITE_STATUS_REG3_CMD                 0x11


/* 编程操作 */
#define PAGE_PROG_CMD                        0x02
#define QUAD_INPUT_PAGE_PROG_CMD             0x32
#define EXT_QUAD_IN_FAST_PROG_CMD            0x12
#define Enter_4Byte_Addr_Mode_CMD            0xB7

/* 擦除操作 */
#define SECTOR_ERASE_CMD                     0x20    //0xD8擦：64K    0x52擦：32K     0x20擦：4K
#define CHIP_ERASE_CMD                       0xC7

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75


/* 状态寄存器标志 */
#define W25QxJV_FSR_BUSY                    ((uint8_t)0x01)    /*!< busy */
#define W25QxJV_FSR_WREN                    ((uint8_t)0x02)    /*!< write enable */
#define W25QxJV_FSR_QE                      ((uint8_t)0x02)    /*!< quad enable */
#define W25Q256FV_FSR_4ByteAddrMode         ((uint8_t)0x01)    /*!< 4字节地址模式 */

void Error_Handler(void);
void SystemClock_Config(void);
void ResetMemory(QSPI_HandleTypeDef *hqspi);
int QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
int QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi);
int QSPI_Enable(QSPI_HandleTypeDef *hqspi);
int QUADSPI_MappedMode(void);
int QSPI_WritePage(unsigned long adr, unsigned long sz, unsigned char *buf);
void HAL_Delay(__IO uint32_t Delay);
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);

//All system initialisation
int Init (uint8_t configureMemoryMappedMode);

//QSPI operation functions
KeepInCompilation int EraseSector (unsigned long adr); 
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer);
KeepInCompilation uint64_t Verify (uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement);
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal);

#endif /* __LOADER_SRC_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
