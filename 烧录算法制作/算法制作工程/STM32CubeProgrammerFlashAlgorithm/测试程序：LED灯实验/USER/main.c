#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"


int main(void)
{
    SCB->VTOR = QSPI_BASE;
    Cache_Enable();                 //��L1-Cache
    HAL_Init();				        //��ʼ��HAL��
    Stm32_Clock_Init(160, 5, 2, 4); //����ʱ��,400Mhz
    delay_init(400);				//��ʱ��ʼ��
    uart_init(115200);				//���ڳ�ʼ��
    LED_Init();						//��ʼ��LED

    while(1)
    {
        printf("This is XIP LED\r\n");
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);	//PB0��0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET); //PB1��0
        delay_ms(500);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);	//PB0��1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);	//PB1��1
        delay_ms(500);
    }
}
