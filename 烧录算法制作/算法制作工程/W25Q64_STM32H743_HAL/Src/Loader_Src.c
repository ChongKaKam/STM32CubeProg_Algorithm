/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  MCD Tools Team
  * @date    October-2015
  * @brief   This file defines the operations of the external loader for
  *          N25Q256A QSPI memory of STM32L476G-EVAL.
  ******************************************************************************
  */

#include "Loader_Src.h"
#include <string.h>


/* Private variables ---------------------------------------------------------*/
//QSPI structure
QSPI_HandleTypeDef QSPIHandle;
QSPI_CommandTypeDef sCommand;
QSPI_AutoPollingTypeDef sConfig;



/**
  * @brief  System initialization.
  * @param  None
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
int Init (uint8_t configureMemoryMappedMode)
{
    __disable_irq(); 
    SystemInit();
    SystemClock_Config();

    /* Zero Init structs */
    memset(&QSPIHandle, 0, sizeof(QSPIHandle));
    memset(&sCommand, 0, sizeof(sCommand));
    memset(&sConfig, 0, sizeof(sConfig));

    /* Initialize QuadSPI ------------------------------------------------------ */
    QSPIHandle.Instance = QUADSPI;
    HAL_QSPI_DeInit(&QSPIHandle) ;

    QSPIHandle.Init.ClockPrescaler     = 2 - 1;
    QSPIHandle.Init.FifoThreshold      = 4;
    QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    QSPIHandle.Init.FlashSize          = 24 - 1;
    QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_5_CYCLE;
    QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    QSPIHandle.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    QSPIHandle.Init.FlashID            = QSPI_FLASH_ID_1;

    if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
    {
        return 0;
    }

    ResetMemory(&QSPIHandle);
    QSPI_WriteEnable(&QSPIHandle);
    QSPI_Enable(&QSPIHandle);
    
    #if 0
    /* Enable 32-Bit address mode */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_32_BITS;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.DummyCycles       = 0;

    sCommand.Instruction       = ENTER_4_BYTE_ADDR_MODE_CMD ;
    QSPIHandle.State           = HAL_QSPI_STATE_READY;
    HAL_QSPI_Command(&QSPIHandle, &sCommand, 0x10);

    /* Configure automatic polling mode to wait the memory is ready */
    if(QSPI_AutoPollingMemReady(&QSPIHandle) != 1)
    {
        return 0;
    }
    #endif

    if(!configureMemoryMappedMode)
    {
        if(QUADSPI_MappedMode() != 1)
            return 0; //fail
    }

    return 1;
}




/*******************************************************************************
Description    :	         erase sector in the device
Inputs 	:
sector address
outputs 	:
"1" 		: Operation succeeded
********************************************************************************/
int EraseSector (unsigned long adr)
{
    if(QSPI_WriteEnable(&QSPIHandle) != 1)
    {
        return 0;
    }

    /* Erasing Sequence -------------------------------------------------- */
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = SECTOR_ERASE_CMD;
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.Address = adr;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = QSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if(HAL_QSPI_Command( &QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* Configure automatic polling mode to wait for end of erase ------- */
    if (QSPI_AutoPollingMemReady(&QSPIHandle) != 1)
    {
        return 0;
    }

    return 1 ;
}

/*******************************************************************************
Description    :	         Erase a full sector in the device
Inputs 	:		 StartSectrorAddress	: Start of sector
EndSectrorAddress	: End of sector
outputs 	:
"1" 		: Operation succeeded
Note : Not Mandatory for SRAM PSRAM and NOR_FLASH
********************************************************************************/
KeepInCompilation int SectorErase (uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    uint32_t BlockAddr;
    
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % W25QxJV_SUBSECTOR_SIZE);

    while (EraseEndAddress >= EraseStartAddress)
    {
        BlockAddr = EraseStartAddress & 0x0FFFFFFF;

        if(EraseSector(BlockAddr) != 1)
        {
            return 0;
        }

        EraseStartAddress += W25QxJV_SUBSECTOR_SIZE;
    }

    return 1;
}

/*******************************************************************************
Description :	                Write data to QSPI
Inputs :
Address 	: Write location
Size 		: Length in bytes
buffer 		: Address where to get the data to write
outputs :
"1" 	        : Operation succeeded
Info :
Note : Mandatory for all types except SRAM and PSRAM
********************************************************************************/
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    uint32_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
    uint32_t   QSPI_DataNum = 0;
    
    Addr = Address % QSPI_PAGE_SIZE;
    count = QSPI_PAGE_SIZE - Addr;
    NumOfPage =  Size / QSPI_PAGE_SIZE;
    NumOfSingle = Size % QSPI_PAGE_SIZE;

    if (Addr == 0) /*!< Address is QSPI_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /*!< NumByteToWrite < QSPI_PAGESIZE */
        {
            QSPI_DataNum = Size;
            QSPI_WritePage(Address, QSPI_DataNum, buffer);
        }
        else /*!< Size > QSPI_PAGESIZE */
        {
            while (NumOfPage--)
            {
                QSPI_DataNum = QSPI_PAGE_SIZE;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
                Address +=  QSPI_PAGE_SIZE;
                buffer += QSPI_PAGE_SIZE;
            }

            QSPI_DataNum = NumOfSingle;

            if(QSPI_DataNum > 0)
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
        }
    }
    else /*!< Address is not QSPI_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /*!< Size < QSPI_PAGESIZE */
        {
            if (NumOfSingle > count) /*!< (Size + Address) > QSPI_PAGESIZE */
            {
                temp = NumOfSingle - count;
                QSPI_DataNum = count;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
                Address +=  count;
                buffer += count;
                QSPI_DataNum = temp;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
            }
            else
            {
                QSPI_DataNum = Size;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
            }
        }
        else /*!< Size > QSPI_PAGESIZE */
        {
            Size -= count;
            NumOfPage =  Size / QSPI_PAGE_SIZE;
            NumOfSingle = Size % QSPI_PAGE_SIZE;
            QSPI_DataNum = count;
            QSPI_WritePage(Address, QSPI_DataNum, buffer);
            Address +=  count;
            buffer += count;

            while (NumOfPage--)
            {
                QSPI_DataNum = QSPI_PAGE_SIZE;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
                Address +=  QSPI_PAGE_SIZE;
                buffer += QSPI_PAGE_SIZE;
            }

            if (NumOfSingle != 0)
            {
                QSPI_DataNum = NumOfSingle;
                QSPI_WritePage(Address, QSPI_DataNum, buffer);
            }
        }
    }

    return 1;
}

/*******************************************************************************
Description :		        Write pages to QSPI
Inputs :
Adr	        : Write location
Size 		: Length in bytes
buf 		: Address where to get the data to write
outputs :
"1" 	        : Operation succeeded
********************************************************************************/
int  QSPI_WritePage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    if(QSPI_WriteEnable(&QSPIHandle) != 1)
    {
        return 0;
    }

    /* Writing Sequence ------------------------------------------------ */
    sCommand.Address = adr & 0x0FFFFFFF;
    sCommand.NbData = sz;
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = QUAD_INPUT_PAGE_PROG_CMD;
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = QSPI_DATA_4_LINES;
    sCommand.DummyCycles = 0;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    if (HAL_QSPI_Transmit(&QSPIHandle, buf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* Configure automatic polling mode to wait for end of program ----- */
    if(QSPI_AutoPollingMemReady(&QSPIHandle) != 1)
    {
        return 0;
    }

    return 1;
}

/*******************************************************************************
Description :			This function send a Write Enable and wait it is effective.
Inputs :
hqspi    : QSPI handle

outputs :
"1" 	        : Operation succeeded
********************************************************************************/
int QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
    /* Enable write operations ------------------------------------------ */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = WRITE_ENABLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* Configure automatic polling mode to wait for write enabling ---- */
    sConfig.Match           = W25QxJV_FSR_WREN;
    sConfig.Mask            = W25QxJV_FSR_WREN;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    sCommand.Instruction    = READ_STATUS_REG1_CMD;
    sCommand.DataMode       = QSPI_DATA_1_LINE  ;
    sCommand.NbData         = 1;

    if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/*******************************************************************************
Description :		        This function wait for end of operation (erase,write,read).
Inputs :
hqspi    : QSPI handle

outputs :
NONE
********************************************************************************/
int QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
    /* Configure automatic polling mode to wait for memory ready ------ */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_STATUS_REG1_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    sConfig.Match           = 0x00;
    sConfig.Mask            = W25QxJV_FSR_BUSY;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/*******************************************************************************
Description :			Reset memory.
Inputs :
hqspi    : QSPI handle

outputs :
None
********************************************************************************/
void ResetMemory(QSPI_HandleTypeDef *hqspi)
{
    /* Reset memory config, Cmd in 1 line */
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2166;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2199;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();

    /* Reset memory config, Cmd in 2 lines*/
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2266;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2299;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();

    /* Reset memory config, Cmd in 4 lines*/
    /* Send RESET ENABLE command (0x66) to be able to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2366;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();

    /* Send RESET command (0x99) to reset the memory registers */
    while(QSPIHandle.Instance->SR & QSPI_FLAG_BUSY);  /* Wait for busy flag to be cleared */

    QSPIHandle.Instance->CCR = 0x2399;
    QSPIHandle.Instance->AR = 0;
    QSPIHandle.Instance->ABR = 0;
    QSPIHandle.Instance->DLR = 0;
    __DSB();
}

/*******************************************************************************
Description :                  This function configure the dummy cycle to read.
Inputs :
hqspi    : QSPI handle

outputs :
"1" 	 : Operation succeeded
********************************************************************************/
#if 0
static int QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi)
{
    uint8_t reg;

    /* Read Volatile Configuration register --------------------------- */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = READ_VOL_CFG_REG_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.NbData            = 1;

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* Enable write operations ---------------------------------------- */
    if (!QSPI_WriteEnable(&QSPIHandle))
    {
        return 0;
    }

    /* Write Volatile Configuration register (with new dummy cycles) -- */
    sCommand.Instruction = WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(reg, 0xF0, (DUMMY_CLOCK_CYCLES_READ_QUAD << POSITION_VAL(0xF0)));

    if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    if (HAL_QSPI_Transmit(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    return 1;
}
#endif
/*******************************************************************************
Description :			This function configure the memory mapped mode.
Inputs :
hqspi    : QSPI handle

outputs :
"1" 	 : Operation succeeded
********************************************************************************/
int QUADSPI_MappedMode(void)
{
    char MemoryMappedMode = 0;

    if (!MemoryMappedMode)
    {
        QSPI_MemoryMappedTypeDef sMemMappedCfg;

        #if 0
        if( QSPI_DummyCyclesCfg(&QSPIHandle) != 1)
        {
            return 0;
        }
        #endif

        sCommand.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressSize        = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        sCommand.DdrMode            = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

        sCommand.Instruction        = QUAD_INOUT_FAST_READ_CMD;
        sCommand.AddressMode        = QSPI_ADDRESS_4_LINES;
        sCommand.Address            = 0;
        sCommand.DataMode           = QSPI_DATA_4_LINES;
        sCommand.NbData             = 0; // undefined length until end of memory
        sCommand.DummyCycles        = 6;

        sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

        if (HAL_QSPI_MemoryMapped(&QSPIHandle, &sCommand, &sMemMappedCfg) != HAL_OK)
        {
            return 0;
        }

        MemoryMappedMode = 1;
    }

    return 1;
}

/*******************************************************************************
Description :		       Initialisation of ticks counter
Inputs :
TickPriority : Set tick priority
outputs :
NONE
********************************************************************************/
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    return HAL_OK;
}

/*******************************************************************************
Description :		       Delay in mS.
Inputs :
Delay : Time in mS
outputs :
NONE
********************************************************************************/
void HAL_Delay(__IO uint32_t Delay)
{
    for(int i = 0; i < Delay * 480000; i++);
}


/**
  * @brief  System Clock Configuration
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Supply configuration update enable
    */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    /** Configure the main internal regulator output voltage
    */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                  | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_QSPI;
    PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    while(1);

    /* USER CODE END Error_Handler_Debug */
}


/**
 * @brief  QSPI使能
 * @retval QSPI存储器状态
 */
int QSPI_Enable(QSPI_HandleTypeDef *hqspi)
{
    uint8_t value = W25QxJV_FSR_QE;

    /* 设置四路使能的状态寄存器，使能四通道IO2和IO3引脚 */
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = WRITE_STATUS_REG2_CMD;
    sCommand.AddressMode = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = QSPI_DATA_1_LINE;
    sCommand.DummyCycles = 0;
    sCommand.NbData = 1;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* 配置命令 */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* 传输数据 */
    if (HAL_QSPI_Transmit(hqspi, &value, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    /* 自动轮询模式等待存储器就绪 */
    if (QSPI_AutoPollingMemReady(hqspi) != 1)
    {
        return 0;
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
    uint8_t missalignementSize = Size;
    int cnt;
    uint32_t Val;

    StartAddress -= StartAddress % 4;
    Size += (Size % 4 == 0) ? 0 : 4 - (Size % 4);

    for(cnt = 0; cnt < Size ; cnt += 4)
    {
        Val = *(uint32_t*)StartAddress;

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
KeepInCompilation uint64_t Verify (uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement)
{
    uint32_t VerifiedData = 0, InitVal = 0;
    uint64_t checksum;
    Size *= 4;

    //Init(0);
    if(QUADSPI_MappedMode() != 1)
        return 0;

    checksum = CheckSum((uint32_t)MemoryAddr + (missalignement & 0xF), Size - ((missalignement >> 16) & 0xF), InitVal);

    while (Size > VerifiedData)
    {
        if ( *(uint8_t*)MemoryAddr++ != *((uint8_t*)RAMBufferAddr + VerifiedData))
            return ((checksum << 32) + (MemoryAddr + VerifiedData));

        VerifiedData++;
    }

    return (checksum << 32);
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
