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
#include "key.h"
#include "usart2.h"	

const u32 Queen_ID = 0x12131415;         // 32位ID   

volatile u16 Time_1ms = 0;               // 1ms计数器  接收方应答超时检测
volatile int RecvWaitTime = 0;           // 接收等待超时时间

u8 Wifi_LinkFlag = 0;                    // 标记已建立过连接，出错重连时可直接连入上次选择的wifi

                           // 帧头  源地址  目标地址 distance*10  电量百分比 帧尾
u8 SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,    15,          50,        0xaa};  // 从机待发送数据
                           // 帧头  源地址  目标地址  帧尾
u8 AckBuffer[ACK_LENGTH]   = {0x55,  0xff,     0,     0xaa};                         // 主机应答数据

volatile u8 Str_Info[20];  // TCP服务器数据包。包含从机数据包中的有效数据

u8 RF_SendPacket(uint8_t *Sendbuffer, uint8_t length);     // 无线发送数据函数  
u8 RF_RecvHandler(void);                                   // 无线数据接收处理 
void Sys_Init(void);                                       // Oil_Can Queen 系统外设初始化

int main(void)
{ 
	u8 Wifi_SendError = 0, res = 0;           // Wifi_SendError:wifi模块向服务器发送数据出错次数   res：Queen CC1101接受结果  
	
	Sys_Init();                               // Oil_Can Queen 系统外设初始化 
    QueenRun_UI();     //queen 接入服务器后的UI

	while(1)
	{
		res = RF_RecvHandler();                                 // 无线数据接收处理 
		if(res != 0) 
		{
			printf("Rec ERROR:%d\r\n", (int)res);               // 接收错误
			LCD_Fill(0,271,239,295,WHITE);
			LCD_ShowString(84,275,72,16,16, (u8*)"Rec ERROR");
			LCD_ShowxNum(156,275,res,1,16,0);     
		}	
		else                                                    // 接收成功
		{
			while(1)    // 持续发送，直到发送成功！
			{
				if(atk_8266_wifisend_data((u8*)Str_Info) == 0)      // wifi发送失败
				{
					LCD_Fill(0,271,239,295,WHITE);
					LCD_ShowString(44,275,152,16,16, (u8*)"Device#  Send ERROR");
					LCD_ShowxNum(100,275,Str_Info[5],1,16,0);       // Device number   Drone_ID
					if(++Wifi_SendError == 3)     // 连续3次发送失败，重新连接TCP
					{
						Wifi_SendError = 0;       // 清零
						printf("连接出错，正在重新连接...\r\n\r\n");	
						atk_8266_init();      // ATK-ESP8266模块初始化配置函数
						QueenRun_UI();     //queen 接入服务器后的UI
						CC1101Init(); 
						delay_ms(500);
					}
					delay_ms(500);
				}
				else                                                // wifi发送成功
				{
					LCD_Fill(0,271,239,295,WHITE);
					LCD_ShowString(44,275,152,16,16, (u8*)"Device#  Send OK!");
					LCD_ShowxNum(100,275,Str_Info[5],1,16,0);       // Device number   Drone_ID
					Wifi_SendError = 0;      // 清零
					printf("wifi发送成功\r\n\r\n");
					break;
				}
			}
		}
	}
}

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
         7: 应答信号错误
============================================================================*/
uint8_t RF_SendPacket(uint8_t *Sendbuffer, uint8_t length)
{
	uint8_t  i = 0, ack_len = 0, ack_buffer[10] = {0};
	RecvWaitTime = (int)RECV_TIMEOUT;           // 等待应答超时限制1500ms
	
	CC1101SendPacket(SendBuffer, length, ADDRESS_CHECK);    // 发送数据   
	CC1101SetTRMode(RX_MODE);                   // 设置RF芯片为接收模式,接收数据
	
	TIM3_Set(1);                                // 开启定时器TIM3
	//printf("waiting for ack...\r\n");
	while(CC_IRQ_READ() != 0)                   // 等待接收数据包
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                        // 关闭定时器TIM3
			return 1;                           // 等待应答超时
		}
	}
	
	RecvWaitTime = 50;                          // 等待应答超时限制50ms
	while(CC_IRQ_READ() == 0)
    {
        if(RecvWaitTime <= 0)      
        {  
            TIM3_Set(0);                         // 关闭定时器TIM3
            return 7;                            // 应答信号错误
        }
    }               
	
	TIM3_Set(0);                                // 关闭定时器TIM3
	ack_len = CC1101RecPacket(ack_buffer);      // 读取收到的数据
	
	if(ack_len <= 0 || ack_len > 10)  
    {
        CC1101Init(); 
        //printf("ack_len1=%d\r\n", ack_len);
        return 2;                               // 数据包长度错误
    }
	
	if(ack_len != ACK_LENGTH) return 2;                                               // 数据包长度错误
	if(ack_buffer[0] != 0x55) return 3;                                               // 数据包帧头错误
	if(ack_buffer[1] != 0xff) return 4;                                               // 数据包源地址错误        
	if(ack_buffer[2] != TX_Address) return 5;                                         // 数据包目标地址错误     非法地址（0xff为主机地址！！）
	if(ack_buffer[3] != 0xaa) return 6;  // 数据包帧尾错误

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
			7: 接收缓存数组越界
============================================================================*/
uint8_t RF_RecvHandler(void)
{
	uint8_t i = 0, length = 0;    
	u8 *recv_buffer = mymalloc(10);	     // 申请10字节内存
	u16 Wait_Timer = 0;                  // CC1101接收数据等待时间
	
	for(i=0; i<SEND_LENGTH; i++) recv_buffer[i] = 0;  // 数据清零,防止误判
	RecvWaitTime = 40;                   // 等待应答超时限制40ms  正常情况10ms之内会完成
	CC1101_IRQFlag = 0;
	
	CC1101SetTRMode(RX_MODE);            // 接收模式 
	EXTI4_Set(1);                        // 使能EXTI4中断
	
	while(1)
	{
		if(++Wait_Timer == 15000) LCD_Fill(0,271,239,295,WHITE); // 清除通信提示区
		delay_us(200);
		if(CC1101_IRQFlag)                  // CC1101接收到数据包信号
		{
			//printf("2\r\n");
			break;        
		}
		else
		{
			if(Wait_Timer == 65500)   // 等待从机数据包时， 定期检测当前网络连接状态
			{
				Wait_Timer = 0;
				printf("Checking\r\n");
//				constate = atk_8266_consta_check(); // 得到连接状态
//				if(constate != '+')       // TCP连接出错
//				{
//					atk_8266_init();      // wifi模块重新连接TCP
//				}
			}
		}
	}
	TIM3_Set(1);                         // 开启定时器TIM3
	while(CC_IRQ_READ() == 0)
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                              // 关闭定时器TIM3
			return 6;                                 // 等待应答超时   
		}
	}
	TIM3_Set(0);                                      // 关闭定时器TIM3
	
	length = CC1101RecPacket(recv_buffer);            // 读取接受到的数据长度和数据内容
	if(length <= 0 || length > 10)     // 数组越界处理
	{
		CC1101Init(); 
		myfree(recv_buffer);		   // 释放内存
		return 7;                      // 数据包长度错误
	}
	if(length != SEND_LENGTH)  
	{
		CC1101Init(); 
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
	if(recv_buffer[5] != 0xaa)
	{
		myfree(recv_buffer);		// 释放内存
		return 5;                   // 数据包帧尾错误
	}
	
	// 数据接收正确 LCD显示 
	POINT_COLOR = RED;
	switch(recv_buffer[1])         // 从机地址   Drone_ID
	{
		case 1:
			LCD_ShowString(54,72,36,24,24,(u8*)"   ");       // 清除#1 Dis显示
			if(recv_buffer[3] == 255) LCD_ShowString(54,72,36,24,24,(u8*)"N/A");       // 提示测距出错
			else                      LCD_ShowxNum(54, 72,recv_buffer[3],3,24,0);      // #1   Dis
			LCD_ShowxNum(54,102,recv_buffer[4],3,24,0);      // Bat
			break;
		case 2:
			LCD_ShowString(174,72,36,24,24,(u8*)"   ");      // 清除#2 Dis显示
			if(recv_buffer[3] == 255) LCD_ShowString(174,72,36,24,24,(u8*)"N/A");      // 提示测距出错
			else                      LCD_ShowxNum(174, 72,recv_buffer[3],3,24,0);     // #2   Dis
			LCD_ShowxNum(174,102,recv_buffer[4],3,24,0);     // Bat
			break;
		case 3:
			LCD_ShowString(54,197,36,24,24,(u8*)"   ");      // 清除#3 Dis显示
			if(recv_buffer[3] == 255) LCD_ShowString(54,197,36,24,24,(u8*)"N/A");      // 提示测距出错
			else                      LCD_ShowxNum(54,197,recv_buffer[3],3,24,0);      // #3   Dis
			LCD_ShowxNum(54,227,recv_buffer[4],3,24,0);      // Bat
			break;
		case 4:
			LCD_ShowString(174,197,36,24,24,(u8*)"   ");     // 清除#4 Dis显示
			if(recv_buffer[3] == 255) LCD_ShowString(174,197,36,24,24,(u8*)"N/A");     // 提示测距出错
			else                      LCD_ShowxNum(174,197,recv_buffer[3],3,24,0);     // #4   Dis
			LCD_ShowxNum(174,227,recv_buffer[4],3,24,0);     // Bat
			break;
		default:
			break;
	}
	
	//提取出从机数据包中的有效数据到  Str_Info 中    从机地址+有效数据
	Str_Info[5] = recv_buffer[1];                // 从机地址   Drone_ID
	Str_Info[6] = recv_buffer[3];                // Distance         单位：cm
	Str_Info[7] = recv_buffer[4];                // 电池电量百分比   [0,100]
	Str_Info[8] = 0;                             // 添加字符串结束符

	printf("Rec from:%d Distance:%dcm\r\n", (int)Str_Info[5], (int)Str_Info[6]);        // 显示从机地址
	//printf("Str_length=%d; Str_Info:%s; RSSI=%d dB\r\n", (int)strlen((const char*)Str_Info), Str_Info, (int)Get_1101RSSI());
	
	// 主机向从机回发应答信号    ！！！！！必须要，否则接收状态将接收不到数据！！！！！
	CC1101SetTRMode(TX_MODE);  
	AckBuffer[2] = Str_Info[5];        // 从机地址
	delay_us(500);
	CC1101SendPacket(AckBuffer, ACK_LENGTH, ADDRESS_CHECK);
	
	printf("Ack to %d OK\r\n", (int)Str_Info[5]);

	myfree(recv_buffer);		//释放内存
	return 0;
}

// Oil_Can Queen系统外设初始化
void Sys_Init(void)
{
	u8 *ID;

	// 固定数据
	Str_Info[0] = 0x23;                          // TCP包头
	Str_Info[1] = (u8)(Queen_ID >> 24);          // Queen_ID
	Str_Info[2] = (u8)(Queen_ID >> 16);          // Queen_ID
	Str_Info[3] = (u8)(Queen_ID >> 8);           // Queen_ID
	Str_Info[4] = Queen_ID & 0xff;               // Queen_ID

	delay_init();	    	                         // 延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // 设置NVIC中断分组2:2位抢占优先级，2位响应优先级	  
	uart_init(115200);	 	                         // 初始化串口1波特率为115200	调试
	USART2_Init(115200);                             // 初始化串口2波特率为115200   wifi

	LCD_Init();				                         // 初始化液晶 	
	KEY_Init();                                      // LCD背光灯开关键
	tp_dev.init();			                         // 初始化触摸屏

	SPI1_Init();                                     // CC1101 SPI通信初始化
	TIM3_Init(99,7199);		                         // CC1101 1ms中断
	mem_init();                                      // 初始化内存池

	printf("Oil_Can_Queen\r\n");
	while(atk_8266_init());                          // 等待wifi、TCP连接成功	

	ID = mymalloc(32);							     // 申请32字节内存
	sprintf((char*)ID, "Queen_ID: %x Connected.", Queen_ID);
	atk_8266_wifisend_data((u8*)ID);                 // 传送Queen_ID到TCP服务器
	myfree(ID);		                                 // 释放内存 

	CC1101Init(); 
}


// wifi发送  测试程序
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







