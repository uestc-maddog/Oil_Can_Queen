#include "key.h"
#include "lcd.h"
#include "delay.h"

//按键输入 驱动代码	  屏幕背光开关	   

 	    
//按键初始化函数 
//PC11 设置成输入
void KEY_Init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);         // 使能PORTC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);          // 外部中断，需要使能AFIO时钟

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);     // 关闭jtag，使能SWD，可以用SWD模式调试
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;//PC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIOC11
 
	//GPIOC.11	  中断线以及中断初始化配置
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource11);

	EXTI_InitStructure.EXTI_Line=EXTI_Line11;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//使能按键所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//抢占优先级2， 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;		    //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure); 
} 

void EXTI15_10_IRQHandler(void)
{
	delay_ms(10);       // 消抖			 
	if(BL_SWITCH == 0)	// 背光开关按键按下
	{
		LCD_LED = !LCD_LED;
	}
	EXTI_ClearITPendingBit(EXTI_Line11);  //清除LINE11线路挂起位
}

