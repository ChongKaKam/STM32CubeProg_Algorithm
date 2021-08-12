#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"


int main(void)
{
    SCB->VTOR = QSPI_BASE;
    Cache_Enable();                 //打开L1-Cache
    HAL_Init();				        //初始化HAL库
    Stm32_Clock_Init(160, 5, 2, 4); //设置时钟,400Mhz
    delay_init(400);				//延时初始化
    uart_init(115200);				//串口初始化
    LED_Init();						//初始化LED

    while(1)
    {
        printf("This is XIP LED\r\n");
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);	//PB0置0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET); //PB1置0
        delay_ms(500);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);	//PB0置1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);	//PB1置1
        delay_ms(500);
    }
}
