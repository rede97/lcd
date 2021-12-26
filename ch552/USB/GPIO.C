
/********************************** (C) COPYRIGHT *******************************
* File Name          : GPIO.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 IO 设置接口函数和GPIO中断函数  
*******************************************************************************/

#include ".\Public\CH554.H"
#include ".\Public\Debug.H"
#include "GPIO.H"
#include "stdio.h"

#pragma NOAREGS

/*******************************************************************************
* Function Name  : Port1Cfg()
* Description    : 端口1配置
* Input          : Mode  0 = 浮空输入，无上拉
                         1 = 推挽输入输出
                         2 = 开漏输入输出，无上拉
                         3 = 类51模式，开漏输入输出，有上拉，内部电路可以加速由低到高的电平爬升		
                   ,UINT8 Pin	(0-7)											 
* Output         : None
* Return         : None
*******************************************************************************/
void Port1Cfg(UINT8 Mode, UINT8 Pin)
{
  switch (Mode)
  {
  case 0:
    P1_MOD_OC = P1_MOD_OC & ~(1 << Pin);
    P1_DIR_PU = P1_DIR_PU & ~(1 << Pin);
    break;
  case 1:
    P1_MOD_OC = P1_MOD_OC & ~(1 << Pin);
    P1_DIR_PU = P1_DIR_PU | (1 << Pin);
    break;
  case 2:
    P1_MOD_OC = P1_MOD_OC | (1 << Pin);
    P1_DIR_PU = P1_DIR_PU & ~(1 << Pin);
    break;
  case 3:
    P1_MOD_OC = P1_MOD_OC | (1 << Pin);
    P1_DIR_PU = P1_DIR_PU | (1 << Pin);
    break;
  default:
    break;
  }
}

/*******************************************************************************
* Function Name  : Port3Cfg()
* Description    : 端口3配置
* Input          : Mode  0 = 浮空输入，无上拉
                         1 = 推挽输入输出
                         2 = 开漏输入输出，无上拉
                         3 = 类51模式，开漏输入输出，有上拉，内部电路可以加速由低到高的电平爬升		
                   ,UINT8 Pin	(0-7)											 
* Output         : None
* Return         : None
*******************************************************************************/
void Port3Cfg(UINT8 Mode, UINT8 Pin)
{
  switch (Mode)
  {
  case 0:
    P3_MOD_OC = P3_MOD_OC & ~(1 << Pin);
    P3_DIR_PU = P3_DIR_PU & ~(1 << Pin);
    break;
  case 1:
    P3_MOD_OC = P3_MOD_OC & ~(1 << Pin);
    P3_DIR_PU = P3_DIR_PU | (1 << Pin);
    break;
  case 2:
    P3_MOD_OC = P3_MOD_OC | (1 << Pin);
    P3_DIR_PU = P3_DIR_PU & ~(1 << Pin);
    break;
  case 3:
    P3_MOD_OC = P3_MOD_OC | (1 << Pin);
    P3_DIR_PU = P3_DIR_PU | (1 << Pin);
    break;
  default:
    break;
  }
}

sbit LCD_BL = P1 ^ 1;
sbit LCD4 = P1 ^ 4;
sbit LCD5 = P1 ^ 5;
sbit LCD6 = P1 ^ 6;
sbit LCD7 = P1 ^ 7;
sbit LCD_E = P3 ^ 2;
sbit LCD_RW = P3 ^ 3;
sbit LCD_RS = P3 ^ 4;

void GpioInit()
{
  Port1Cfg(1, 1);
  // PORT 14 15 16 16
  Port1Cfg(1, 4);
  Port1Cfg(1, 5);
  Port1Cfg(1, 6);
  Port1Cfg(1, 7);
  // PORT 32 33 34
  Port3Cfg(1, 2);
  Port3Cfg(1, 3);
  Port3Cfg(1, 4);

  LCD4 = 0;
  LCD5 = 0;
  LCD6 = 0;
  LCD7 = 0;

  LCD_E = 0;
  LCD_RW = 0;
  LCD_RS = 0;
  LCD_BL = 0;
}

// void dump()
// {
// 	int v;
// 	v = LCD7;
// 	printf("D7:%d ", v);
// 	v = LCD6;
// 	printf("D6:%d ", v);
// 	v = LCD5;
// 	printf("D5:%d ", v);
// 	v = LCD4;
// 	printf("D4:%d ", v);
// 	v = LCD_E;
// 	printf("E:%d ", v);
// 	v = LCD_RS;
// 	printf("RS:%d ", v);
// 	v = LCD_RW;
// 	printf("RW:%d\n", v);
// }

void cmd4(UINT8 c)
{
  LCD_RW = 0;
  LCD4 = (c >> 0) & 0x01;
  LCD5 = (c >> 1) & 0x01;
  LCD6 = (c >> 2) & 0x01;
  LCD7 = (c >> 3) & 0x01;
  LCD_RS = 0;
  LCD_E = 1;
  mDelayuS(1);
  LCD_E = 0;
  mDelayuS(50);
}

void dat4(UINT8 d)
{
  LCD_RW = 0;
  LCD4 = (d >> 0) & 0x01;
  LCD5 = (d >> 1) & 0x01;
  LCD6 = (d >> 2) & 0x01;
  LCD7 = (d >> 3) & 0x01;
  LCD_RS = 1;
  LCD_E = 1;
  mDelayuS(1);
  LCD_E = 0;
  mDelayuS(50);
}

void setio(UINT8 io)
{
  LCD_BL = (io >> 0) & 0x01;
}
