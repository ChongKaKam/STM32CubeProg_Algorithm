#ifndef __LED_H
#define __LED_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32H7������
//LED ��������
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2018/6/6
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////

//LED�˿ڶ���
#define LED_ON			GPIO_Pin_Set(GPIOB,PIN0,0)		// DS0
#define LED_OFF			GPIO_Pin_Set(GPIOB,PIN0,1)		// DS0

void LED_Init(void);	//��ʼ��
#endif

















