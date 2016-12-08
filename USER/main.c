#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "common.h" 
#include "wifista.h"
#include "malloc.h"
#include "lcd.h"
#include "cc1101.h"
#include "touch.h"	
#include "spi.h"
#include "usart2.h"	
 
volatile u16 Time_1ms = 0;               // 1ms计数器  接收方应答超时检测
u16 SendCnt = 0;
u16 RecvCnt = 0;
int RecvWaitTime = 0;                    // 接收等待超时时间

                                // 帧头  源地址  目标地址 有效数据9B                                   帧尾2B
u8 SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,    '2', '9', '6', '8', '5', '1', '2', '9', '2', 0x0d, 0x0a};       // 待发送数据
                                // 帧头  源地址  目标地址  帧尾2B
u8 AckBuffer[ACK_LENGTH]   = {0x55,  0xff,     0,     0x0d, 0x0a};                                  // 主机应答数据

volatile u8 Str_Info[20];       // 从机数据包中的 有效数据

u8 RF_SendPacket(uint8_t *Sendbuffer, uint8_t length);     // 无线发送数据函数  
u8 RF_RecvHandler(void);                                   // 无线数据接收处理 

void Load_Drow_Dialog(void);                 
void rtp_test(void);                           // 电阻触摸屏测试函数
	
int main(void)
{ 
	u8 Link_Error = 0;

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
	
	printf("Oil Can...\r\n");
	atk_8266_init();                                 // ATK-ESP8266模块初始化配置函数
	
	while(1);
//	CC1101Init();                                    // 初始化CC1101

//#if (WORK_MODE == TX)     // 执行发送模块程序
//	SendBuffer[1] = TX_Address;  // 数据包源地址
//	printf("Mode:TX\r\n");
//	CC1101SetTRMode(TX_MODE);    // 发送模式  
//	while(1)
//	{
////		do
////		{
////			res = RF_SendPacket(SendBuffer, SEND_LENGTH);
////			if(res != 0) printf("\r\nSend ERROR:%d\r\n", (int)res);  // 发送错误
////		} while(res != 0);
////		printf("\r\nSend OK\r\n"); 
//		u8 res = RF_SendPacket(SendBuffer, SEND_LENGTH);
//		if(res != 0) printf("\r\nSend ERROR:%d\r\n", (int)res);  // 发送错误代码
//		delay_ms(1000);
//		delay_ms(1000);
//		delay_ms(1000);
//		delay_ms(1000);
//	}
//#else                     // 执行接收模块程序
//	printf("Mode:RX\r\n");
//	CC1101SetTRMode(RX_MODE);   // 接收模式  
//	while(1)
//	{
//		u8 res = RF_RecvHandler();                              // 无线数据接收处理 
//		if(res != 0) printf("Rec ERROR:%d\r\n", (int)res);      // 接收错误
//		else                                                    // 接收成功
//		{
//			while(1)    // 持续发送，直到发送成功！
//			{
//				if(atk_8266_wifisend_data((u8*)Str_Info) == 0)      // wifi发送失败
//				{
//					if(++Link_Error == 3)     // 连续3次发送失败，重新连接TCP
//					{
//						Link_Error = 0;       // 清零
//						printf("连接出错，正在重新连接...\r\n\r\n");	
//						atk_8266_init();      // ATK-ESP8266模块初始化配置函数
//						delay_ms(500);
//					}
//					delay_ms(500);
//				}
//				else                                                // wifi发送成功
//				{
//					Link_Error = 0;      // 清零
//					printf("wifi发送成功\r\n\r\n");
//					break;
//				}
//			}
//		}
//	}
//#endif
//}

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
}

void Load_Drow_Dialog(void)
{
		LCD_Clear(WHITE);//清屏   
		POINT_COLOR=BLUE;//设置字体为蓝色 
		LCD_ShowString(lcddev.width-24,0,200,16,16,(u8*)"RST");//显示清屏区域
		POINT_COLOR=RED;//设置画笔蓝色 
}
////////////////////////////////////////////////////////////////////////////////
//5个触控点的颜色												 
//电阻触摸屏测试函数
void rtp_test(void)
{
	while(1)
	{
		tp_dev.scan(0); 		 
		if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16)Load_Drow_Dialog();//清除
				else TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);		//画图	  			   
			}
		}
		else delay_ms(10);	//没有按键按下的时候 	    
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

============================================================================*/
uint8_t RF_SendPacket(uint8_t *Sendbuffer, uint8_t length)
{
	volatile uint8_t ack_flag = 1;         // =1,接收应答信号,=0不处理
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
============================================================================*/
uint8_t RF_RecvHandler(void)
{
	uint8_t i = 0, j = 0, length = 0, recv_buffer[30] = {0};
	
	CC1101SetTRMode(RX_MODE);            // 设置RF芯片为接收模式,接收数据
	
	printf("waiting for data...\r\n");
	while(CC_IRQ_READ() != 0);           // 等待接收数据包
	printf("waiting for data1...\r\n");
	while(CC_IRQ_READ() == 0);

	for(i=0; i<SEND_LENGTH; i++) recv_buffer[i] = 0;  // 数据清零,防止误判
	length = CC1101RecPacket(recv_buffer);            // 读取接受到的数据长度和数据内容

//	                              // 帧头  源地址  目标地址  有效数据9B                 帧尾2B
//uint8_t SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,     2, 9, 6, 8, 5, 1, 2, 9, 2, 0x0d, 0x0a};       // 待发送数据
	if(length != SEND_LENGTH)  return 1;                                              // 数据包长度错误
	if(recv_buffer[0] != 0x55) return 2;                                              // 数据包帧头错误
	if(recv_buffer[1] == 0xff) return 3;                                              // 数据包源地址错误        非法地址（0xff为主机地址！！）
	if(recv_buffer[2] != 0xff) return 4;                                              // 数据包目标地址错误
	if((recv_buffer[length-2] != 0x0d) || (recv_buffer[length-1] != 0x0a)) return 5;  // 数据包帧尾错误
	
	
	// 数据接收正确  提取出从机数据包中的有效数据到  Str_Info 中    从机地址+有效数据
	Str_Info[j++] = recv_buffer[1];                // 从机地址
	for(i = 3; i < (length-2); i++, j++)           // 有效数据             
	{
		Str_Info[j] = recv_buffer[i];
	}
	Str_Info[j] = 0;           // 添加字符串结束符
	printf("Rec from:%d\r\n", (int)Str_Info[0]);        // 显示从机地址
	printf("Str_length=%d; Str_Info:%s; RSSI=%d dB\r\n", (int)strlen((const char*)Str_Info), Str_Info, (int)Get_1101RSSI());
	
	// 主机向从机回发应答信号

	CC1101SetTRMode(TX_MODE);  
	AckBuffer[2] = recv_buffer[1];        // 从机地址
	CC1101SendPacket(AckBuffer, ACK_LENGTH, ADDRESS_CHECK);
	 
	printf("%d:Ack to %d OK\r\n", (int)++RecvCnt, (int)AckBuffer[2]);

	return 0;
}
















