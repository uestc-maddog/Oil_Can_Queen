#include "delay.h"
#include "sys.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "usart.h"
#include "common.h" 
#include "wifista.h"
#include "malloc.h"
#include "lcd.h"
#include "cc1101.h"
#include "touch.h"	
#include "spi.h"
#include "usart2.h"	

const u32 Queen_ID = 0x00000000;         // 32位ID   

volatile u16 Time_1ms = 0;               // 1ms计数器  接收方应答超时检测
u16 SendCnt = 0;
u16 RecvCnt = 0;
volatile int RecvWaitTime = 0;                    // 接收等待超时时间

u8 Link_Flag = 0;                        // 标记已建立过连接，可直接连入上次选择的wifi

                                // 帧头  源地址  目标地址 有效数据9B                                   帧尾2B
u8 SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,    '2', '9', '6', '8', '5', '1', '2', '9', '2', 0x0d, 0x0a};  // 从机待发送数据
                                // 帧头  源地址  目标地址  帧尾2B
u8 AckBuffer[ACK_LENGTH]   = {0x55,  0xff,     0,     0x0d, 0x0a};                                              // 主机应答数据

volatile u8 Str_Info[20];       // 从机数据包中的 有效数据

u8 RF_SendPacket(uint8_t *Sendbuffer, uint8_t length);     // 无线发送数据函数  
u8 RF_RecvHandler(void);                                   // 无线数据接收处理 
	
int main(void)
{ 
	u8 Link_Error = 0, res = 0;
	u8 *ID;
	
	delay_init();	    	                         // 延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 设置NVIC中断分组2:2位抢占优先级，2位响应优先级	  
	uart_init(115200);	 	                         // 初始化串口1波特率为115200	调试
	USART2_Init(115200);                             // 初始化串口2波特率为115200   wifi
	
	LCD_Init();				                         // 初始化液晶 		
	LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
	tp_dev.init();			                         // 初始化触摸屏
	
	SPI1_Init();                                     // CC1101 SPI通信初始化
	TIM3_Init(99,7199);		                         // CC1101 1ms中断
	mem_init();                                      // 初始化内存池
	
	printf("Oil_Can_Queen\r\n");
	atk_8266_init();                                 // ATK-ESP8266模块初始化配置函数	
		
	printf("atk_8266_init OK!\r\n");

	ID = mymalloc(32);							     // 申请32字节内存
	sprintf((char*)ID, "Queen_ID:%x", Queen_ID);
	atk_8266_wifisend_data((u8*)ID);                 // 传送Queen_ID到TCP服务器
	myfree(ID);		                                 // 释放内存 
	printf("Mode:RX\r\n");
 
	CC1101Init(); 
	while(1)
	{
		CC1101Init(); 
		res = RF_RecvHandler();                                 // 无线数据接收处理 
		if(res != 0) printf("Rec ERROR:%d\r\n", (int)res);      // 接收错误
		else                                                    // 接收成功
		{
			while(1)    // 持续发送，直到发送成功！
			{
				if(atk_8266_wifisend_data((u8*)Str_Info) == 0)      // wifi发送失败
				{
					if(++Link_Error == 3)     // 连续3次发送失败，重新连接TCP
					{
						Link_Error = 0;       // 清零
						printf("连接出错，正在重新连接...\r\n\r\n");	
						atk_8266_init();      // ATK-ESP8266模块初始化配置函数
						CC1101Init(); 
						delay_ms(500);
					}
					delay_ms(500);
				}
				else                                                // wifi发送成功
				{
					Link_Error = 0;      // 清零
					printf("wifi发送成功\r\n\r\n");
					break;
				}
			}
		}
	}
}


// wifi测试程序
//	while(1)    // 持续发送，直到发送成功！
//	{
//		res = atk_8266_wifisend_data((u8*)"wifi test");
//		if(res) printf("send ok\r\n");
//		else    
//		{
//			printf("send error\r\n");
//			atk_8266_init();                                        // ATK-ESP8266模块初始化配置函数
//		}
//		delay_ms(500);delay_ms(500);delay_ms(500);
//	}

//int main(void)
//{
//	uint8_t res = 0;
//	
//  HAL_Init();
//  SystemClock_Config();

//  MX_GPIO_Init();
//	MX_USART1_UART_Init();
//	MX_TIM3_Init();
//  MX_SPI1_Init();	
//	
//	CC1101Init();                                   // 初始化L01寄存器     

//#if (WORK_MODE == TX)     // 执行发送模块程序
//	
//	SendBuffer[0] = TX_Address;  // 数据包首字节标记源地址
//	printf("Mode:TX\r\n");
//	CC1101SetTRMode(TX_MODE);    // 发送模式  
//	while(1)
//	{
//		res = RF_SendPacket(SendBuffer, SEND_LENGTH);
//		if(res == 1)         // 发送数据包成功
//		{
//			SendCnt++;
//			printf("Send OK\r\n\r\n");
//		}
//		else if(res == 2)    // 应答超时
//		{
//			SendCnt++;
//			printf("Ack ERROR\r\n\r\n");
//		}
//		else
//		{
//			printf("Send ERROR\r\n\r\n");
//		}
//		HAL_Delay(1000);HAL_Delay(1000);HAL_Delay(1000);HAL_Delay(800);
//	}
//	
//#else                     // 执行接收模块程序
//	
//	AckBuffer[0] = RX_Address;  // 数据包首字节标记源地址
//	printf("Mode:RX\r\n");
//	CC1101SetTRMode(RX_MODE);   // 接收模式  
//	while(1)
//	{
//		RF_RecvHandler();     // 无线数据接收处理 
//	}
//	
//#endif

/*===========================================================================
* 函数 : RF_SendPacket() => 无线发送数据函数                            *
* 输入 : Sendbuffer指向待发送的数据包，length数据包长度                 *
* 输出 : 0：发送成功
		 1：等待应答超时 
		 2：数据包长度错误
		 3：数据包帧头错误
		 4：数据包源地址错误        
		 5：数据包目标地址错误
		 6：数据包帧尾错误

============================================================================*/
uint8_t RF_SendPacket(uint8_t *Sendbuffer, uint8_t length)
{
	uint8_t  i = 0, ack_len = 0, ack_buffer[65] = {0};
	
	CC1101SendPacket(SendBuffer, length, ADDRESS_CHECK);    // 发送数据   
	                       
	CC1101Init();                               // 初始化L01寄存器    
	CC1101SetTRMode(RX_MODE);                   // 设置RF芯片为接收模式,接收数据

	RecvWaitTime = (int)RECV_TIMEOUT;           // 等待应答超时限制1500ms
	TIM3_Set(1);                                // 开启定时器TIM3

	printf("Send Over, waiting for ack...\r\n");
	while(CC_IRQ_READ() != 0)                   // 等待接收数据包
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                            // 关闭定时器TIM3
			return 1;                               // 等待应答超时
		}
	}
	while(CC_IRQ_READ() == 0);               
	
	TIM3_Set(0);                                // 关闭定时器TIM3
	ack_len = CC1101RecPacket(ack_buffer);      // 读取收到的数据
	
//	                                 //帧头  源地址  目标地址  帧尾2B
//uint8_t AckBuffer[ACK_LENGTH]   = {0x55,  0xff,     0,     0x0d, 0x0a};    // 主机应答数据
	
	if(ack_len != ACK_LENGTH) return 2;                                               // 数据包长度错误
	if(ack_buffer[0] != 0x55) return 3;                                               // 数据包帧头错误
	if(ack_buffer[1] != 0xff) return 4;                                               // 数据包源地址错误        
	if(ack_buffer[2] == 0xff) return 5;                                               // 数据包目标地址错误     非法地址（0xff为主机地址！！）
	if((ack_buffer[ack_len-2] != 0x0d) || (ack_buffer[ack_len-1] != 0x0a)) return 6;  // 数据包帧尾错误

	// 应答正确
	printf("ack_len=%d;ack_buffer:", (int)ack_len);
	for(i = 0; i < ack_len; i++)                     
	{
		printf("%d ", (int)ack_buffer[i]);
	}
	printf("\r\n");

	return 0;  
}

/*===========================================================================
*   函数: 	RF_RecvHandler() => 无线数据接收处理                            *
* 返回值：	0：接收正确                                                     *
			1：数据包长度错误
			2：数据包帧头错误
			3：数据包源地址错误        
			4：数据包目标地址错误
			5：数据包帧尾错误
			6：接收超时
============================================================================*/
uint8_t RF_RecvHandler(void)
{
	uint8_t i = 0, j = 0, length = 0;    // recv_buffer[30] = {0};
	u8 *recv_buffer = mymalloc(30);	     // 申请30字节内存
	
	CC1101SetTRMode(RX_MODE);            // 接收模式 
	delay_ms(1);
	
	printf("waiting...\r\n");
	while(CC_IRQ_READ() != 0);           // 等待接收数据包
	printf("waiting1...\r\n");
	
	RecvWaitTime = 40;                   // 等待应答超时限制40ms  正常情况10ms之内会完成
	TIM3_Set(1);                         // 开启定时器TIM3
	while(CC_IRQ_READ() == 0)
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                              // 关闭定时器TIM3
			printf("waiting2...\r\n");
			return 6;                                 // 等待应答超时   
		}
	}
	TIM3_Set(0);                                      // 关闭定时器TIM3
	printf("RecvWaitTime1=%d\r\n", RecvWaitTime);     // 正常情况10ms之内会完成
	
	for(i=0; i<SEND_LENGTH; i++) recv_buffer[i] = 0;  // 数据清零,防止误判
	length = CC1101RecPacket(recv_buffer);            // 读取接受到的数据长度和数据内容

//	                              // 帧头  源地址  目标地址  有效数据9B                 帧尾2B
//uint8_t SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,     2, 9, 6, 8, 5, 1, 2, 9, 2, 0x0d, 0x0a};       // 待发送数据
//	if((strlen((const char*)recv_buffer) <= 0) || (strlen((const char*)recv_buffer)) > 29)  
//	{
//		CC1101Init(); 
//		printf("length0=%d\r\n", length);
//		myfree(recv_buffer);		//释放内存
//		return 1;                                              // 数据包长度错误
//	}
	printf("strlen(recv_buffer)=%d\r\n", (int)strlen((const char*)recv_buffer));
	
	if(length <= 0 || length > 30)  
	{
		CC1101Init(); 
		printf("length1=%d\r\n", length);
		myfree(recv_buffer);		//释放内存
		return 1;                                              // 数据包长度错误
	}
	if(length != SEND_LENGTH)  
	{
		CC1101Init(); 
		printf("length2=%d\r\n", length);
		myfree(recv_buffer);		// 释放内存
		return 1;                   // 数据包长度错误
	}
	if(recv_buffer[0] != 0x55) 
	{
		myfree(recv_buffer);		// 释放内存
		return 2;                   // 数据包帧头错误
	}
	if(recv_buffer[1] == 0xff) 
	{
		myfree(recv_buffer);		// 释放内存
		return 3;                   // 数据包源地址错误        非法地址（0xff为主机地址！！）
	}
	if(recv_buffer[2] != 0xff) 
	{
		myfree(recv_buffer);		// 释放内存
		return 4;                   // 数据包目标地址错误
	}
	if((recv_buffer[length-2] != 0x0d) || (recv_buffer[length-1] != 0x0a)) 
	{
		myfree(recv_buffer);		// 释放内存
		return 5;                   // 数据包帧尾错误
	}
	
	// 数据接收正确  提取出从机数据包中的有效数据到  Str_Info 中    从机地址+有效数据
	Str_Info[j++] = recv_buffer[1];                // 从机地址
	for(i = 3; i < (length-2); i++, j++)           // 有效数据             
	{
		Str_Info[j] = recv_buffer[i];
	}
	Str_Info[j] = 0;                                    // 添加字符串结束符
	printf("Rec from:%d\r\n", (int)Str_Info[0]);        // 显示从机地址
	printf("Str_length=%d; Str_Info:%s; RSSI=%d dB\r\n", (int)strlen((const char*)Str_Info), Str_Info, (int)Get_1101RSSI());
	
	// 主机向从机回发应答信号    ！！！！！必须要，否则接收状态将接收不到数据！！！！！
	CC1101SetTRMode(TX_MODE);  
	AckBuffer[2] = recv_buffer[1];        // 从机地址
	CC1101SendPacket(AckBuffer, ACK_LENGTH, ADDRESS_CHECK);
	
	printf("%d:Ack to %d OK\r\n", (int)++RecvCnt, (int)AckBuffer[2]);

	myfree(recv_buffer);		//释放内存
	return 0;
}
















