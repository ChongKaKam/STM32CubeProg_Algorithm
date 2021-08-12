#ifndef __LED_H
#define __LED_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32H7开发板
//LED 驱动代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2018/6/6
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////

//LED端口定义
#define LED_ON			GPIO_Pin_Set(GPIOB,PIN0,0)		// DS0
#define LED_OFF			GPIO_Pin_Set(GPIOB,PIN0,1)		// DS0

void LED_Init(void);	//初始化
#endif

















