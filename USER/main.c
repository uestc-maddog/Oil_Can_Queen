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

const u32 Queen_ID = 0x12131415;         // 32λID   

volatile u16 Time_1ms = 0;               // 1ms������  ���շ�Ӧ��ʱ���
volatile int RecvWaitTime = 0;           // ���յȴ���ʱʱ��

u8 Wifi_LinkFlag = 0;                    // ����ѽ��������ӣ���������ʱ��ֱ�������ϴ�ѡ���wifi

                           // ֡ͷ  Դ��ַ  Ŀ���ַ distance*10  �����ٷֱ� ֡β
u8 SendBuffer[SEND_LENGTH] = {0x55,   0,    0xff,    15,          50,        0xaa};  // �ӻ�����������
                           // ֡ͷ  Դ��ַ  Ŀ���ַ  ֡β
u8 AckBuffer[ACK_LENGTH]   = {0x55,  0xff,     0,     0xaa};                         // ����Ӧ������

volatile u8 Str_Info[20];  // TCP���������ݰ��������ӻ����ݰ��е���Ч����

u8 RF_SendPacket(uint8_t *Sendbuffer, uint8_t length);     // ���߷������ݺ���  
u8 RF_RecvHandler(void);                                   // �������ݽ��մ��� 
void Sys_Init(void);                                       // Oil_Can Queen ϵͳ�����ʼ��

int main(void)
{ 
	u8 Wifi_SendError = 0, res = 0;           // Wifi_SendError:wifiģ����������������ݳ������   res��Queen CC1101���ܽ��  
	
	Sys_Init();                               // Oil_Can Queen ϵͳ�����ʼ�� 
    QueenRun_UI();     //queen ������������UI

	while(1)
	{
		res = RF_RecvHandler();                                 // �������ݽ��մ��� 
		if(res != 0) 
		{
			printf("Rec ERROR:%d\r\n", (int)res);               // ���մ���
			LCD_Fill(0,271,239,295,WHITE);
			LCD_ShowString(84,275,72,16,16, (u8*)"Rec ERROR");
			LCD_ShowxNum(156,275,res,1,16,0);     
		}	
		else                                                    // ���ճɹ�
		{
			while(1)    // �������ͣ�ֱ�����ͳɹ���
			{
				if(atk_8266_wifisend_data((u8*)Str_Info) == 0)      // wifi����ʧ��
				{
					LCD_Fill(0,271,239,295,WHITE);
					LCD_ShowString(44,275,152,16,16, (u8*)"Device#  Send ERROR");
					LCD_ShowxNum(100,275,Str_Info[5],1,16,0);       // Device number   Drone_ID
					if(++Wifi_SendError == 3)     // ����3�η���ʧ�ܣ���������TCP
					{
						Wifi_SendError = 0;       // ����
						printf("���ӳ���������������...\r\n\r\n");	
						atk_8266_init();      // ATK-ESP8266ģ���ʼ�����ú���
						QueenRun_UI();     //queen ������������UI
						CC1101Init(); 
						delay_ms(500);
					}
					delay_ms(500);
				}
				else                                                // wifi���ͳɹ�
				{
					LCD_Fill(0,271,239,295,WHITE);
					LCD_ShowString(44,275,152,16,16, (u8*)"Device#  Send OK!");
					LCD_ShowxNum(100,275,Str_Info[5],1,16,0);       // Device number   Drone_ID
					Wifi_SendError = 0;      // ����
					printf("wifi���ͳɹ�\r\n\r\n");
					break;
				}
			}
		}
	}
}

/*===========================================================================
* ���� : RF_SendPacket() => ���߷������ݺ���                            *
* ���� : Sendbufferָ������͵����ݰ���length���ݰ�����                 *
* ��� : 0�����ͳɹ�
		 1���ȴ�Ӧ��ʱ 
		 2�����ݰ����ȴ���
		 3�����ݰ�֡ͷ����
		 4�����ݰ�Դ��ַ����        
		 5�����ݰ�Ŀ���ַ����
		 6�����ݰ�֡β����
         7: Ӧ���źŴ���
============================================================================*/
uint8_t RF_SendPacket(uint8_t *Sendbuffer, uint8_t length)
{
	uint8_t  i = 0, ack_len = 0, ack_buffer[10] = {0};
	RecvWaitTime = (int)RECV_TIMEOUT;           // �ȴ�Ӧ��ʱ����1500ms
	
	CC1101SendPacket(SendBuffer, length, ADDRESS_CHECK);    // ��������   
	CC1101SetTRMode(RX_MODE);                   // ����RFоƬΪ����ģʽ,��������
	
	TIM3_Set(1);                                // ������ʱ��TIM3
	//printf("waiting for ack...\r\n");
	while(CC_IRQ_READ() != 0)                   // �ȴ��������ݰ�
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                        // �رն�ʱ��TIM3
			return 1;                           // �ȴ�Ӧ��ʱ
		}
	}
	
	RecvWaitTime = 50;                          // �ȴ�Ӧ��ʱ����50ms
	while(CC_IRQ_READ() == 0)
    {
        if(RecvWaitTime <= 0)      
        {  
            TIM3_Set(0);                         // �رն�ʱ��TIM3
            return 7;                            // Ӧ���źŴ���
        }
    }               
	
	TIM3_Set(0);                                // �رն�ʱ��TIM3
	ack_len = CC1101RecPacket(ack_buffer);      // ��ȡ�յ�������
	
	if(ack_len <= 0 || ack_len > 10)  
    {
        CC1101Init(); 
        //printf("ack_len1=%d\r\n", ack_len);
        return 2;                               // ���ݰ����ȴ���
    }
	
	if(ack_len != ACK_LENGTH) return 2;                                               // ���ݰ����ȴ���
	if(ack_buffer[0] != 0x55) return 3;                                               // ���ݰ�֡ͷ����
	if(ack_buffer[1] != 0xff) return 4;                                               // ���ݰ�Դ��ַ����        
	if(ack_buffer[2] != TX_Address) return 5;                                         // ���ݰ�Ŀ���ַ����     �Ƿ���ַ��0xffΪ������ַ������
	if(ack_buffer[3] != 0xaa) return 6;  // ���ݰ�֡β����

	// Ӧ����ȷ
	printf("ack_len=%d;ack_buffer:", (int)ack_len);
	for(i = 0; i < ack_len; i++)                     
	{
		printf("%d ", (int)ack_buffer[i]);
	}
	printf("\r\n");

	return 0;  
}

/*===========================================================================
*   ����: 	RF_RecvHandler() => �������ݽ��մ���                            *
* ����ֵ��	0��������ȷ                                                     *
			1�����ݰ����ȴ���
			2�����ݰ�֡ͷ����
			3�����ݰ�Դ��ַ����        
			4�����ݰ�Ŀ���ַ����
			5�����ݰ�֡β����
			6�����ճ�ʱ
			7: ���ջ�������Խ��
============================================================================*/
uint8_t RF_RecvHandler(void)
{
	uint8_t i = 0, length = 0;    
	u8 *recv_buffer = mymalloc(10);	     // ����10�ֽ��ڴ�
	u16 Wait_Timer = 0;                  // CC1101�������ݵȴ�ʱ��
	
	for(i=0; i<SEND_LENGTH; i++) recv_buffer[i] = 0;  // ��������,��ֹ����
	RecvWaitTime = 40;                   // �ȴ�Ӧ��ʱ����40ms  �������10ms֮�ڻ����
	CC1101_IRQFlag = 0;
	
	CC1101SetTRMode(RX_MODE);            // ����ģʽ 
	EXTI4_Set(1);                        // ʹ��EXTI4�ж�
	
	while(1)
	{
		if(++Wait_Timer == 15000) LCD_Fill(0,271,239,295,WHITE); // ���ͨ����ʾ��
		delay_us(200);
		if(CC1101_IRQFlag)                  // CC1101���յ����ݰ��ź�
		{
			//printf("2\r\n");
			break;        
		}
		else
		{
			if(Wait_Timer == 65500)   // �ȴ��ӻ����ݰ�ʱ�� ���ڼ�⵱ǰ��������״̬
			{
				Wait_Timer = 0;
				printf("Checking\r\n");
//				constate = atk_8266_consta_check(); // �õ�����״̬
//				if(constate != '+')       // TCP���ӳ���
//				{
//					atk_8266_init();      // wifiģ����������TCP
//				}
			}
		}
	}
	TIM3_Set(1);                         // ������ʱ��TIM3
	while(CC_IRQ_READ() == 0)
	{
		if(RecvWaitTime <= 0)      
		{  
			TIM3_Set(0);                              // �رն�ʱ��TIM3
			return 6;                                 // �ȴ�Ӧ��ʱ   
		}
	}
	TIM3_Set(0);                                      // �رն�ʱ��TIM3
	
	length = CC1101RecPacket(recv_buffer);            // ��ȡ���ܵ������ݳ��Ⱥ���������
	if(length <= 0 || length > 10)     // ����Խ�紦��
	{
		CC1101Init(); 
		myfree(recv_buffer);		   // �ͷ��ڴ�
		return 7;                      // ���ݰ����ȴ���
	}
	if(length != SEND_LENGTH)  
	{
		CC1101Init(); 
		myfree(recv_buffer);		// �ͷ��ڴ�
		return 1;                   // ���ݰ����ȴ���
	}
	if(recv_buffer[0] != 0x55) 
	{
		myfree(recv_buffer);		// �ͷ��ڴ�
		return 2;                   // ���ݰ�֡ͷ����
	}
	if(recv_buffer[1] == 0xff) 
	{
		myfree(recv_buffer);		// �ͷ��ڴ�
		return 3;                   // ���ݰ�Դ��ַ����        �Ƿ���ַ��0xffΪ������ַ������
	}
	if(recv_buffer[2] != 0xff) 
	{
		myfree(recv_buffer);		// �ͷ��ڴ�
		return 4;                   // ���ݰ�Ŀ���ַ����
	}
	if(recv_buffer[5] != 0xaa)
	{
		myfree(recv_buffer);		// �ͷ��ڴ�
		return 5;                   // ���ݰ�֡β����
	}
	
	// ���ݽ�����ȷ LCD��ʾ 
	POINT_COLOR = RED;
	switch(recv_buffer[1])         // �ӻ���ַ   Drone_ID
	{
		case 1:
			LCD_ShowString(54,72,36,24,24,(u8*)"   ");       // ���#1 Dis��ʾ
			if(recv_buffer[3] == 255) LCD_ShowString(54,72,36,24,24,(u8*)"N/A");       // ��ʾ������
			else                      LCD_ShowxNum(54, 72,recv_buffer[3],3,24,0);      // #1   Dis
			LCD_ShowxNum(54,102,recv_buffer[4],3,24,0);      // Bat
			break;
		case 2:
			LCD_ShowString(174,72,36,24,24,(u8*)"   ");      // ���#2 Dis��ʾ
			if(recv_buffer[3] == 255) LCD_ShowString(174,72,36,24,24,(u8*)"N/A");      // ��ʾ������
			else                      LCD_ShowxNum(174, 72,recv_buffer[3],3,24,0);     // #2   Dis
			LCD_ShowxNum(174,102,recv_buffer[4],3,24,0);     // Bat
			break;
		case 3:
			LCD_ShowString(54,197,36,24,24,(u8*)"   ");      // ���#3 Dis��ʾ
			if(recv_buffer[3] == 255) LCD_ShowString(54,197,36,24,24,(u8*)"N/A");      // ��ʾ������
			else                      LCD_ShowxNum(54,197,recv_buffer[3],3,24,0);      // #3   Dis
			LCD_ShowxNum(54,227,recv_buffer[4],3,24,0);      // Bat
			break;
		case 4:
			LCD_ShowString(174,197,36,24,24,(u8*)"   ");     // ���#4 Dis��ʾ
			if(recv_buffer[3] == 255) LCD_ShowString(174,197,36,24,24,(u8*)"N/A");     // ��ʾ������
			else                      LCD_ShowxNum(174,197,recv_buffer[3],3,24,0);     // #4   Dis
			LCD_ShowxNum(174,227,recv_buffer[4],3,24,0);     // Bat
			break;
		default:
			break;
	}
	
	//��ȡ���ӻ����ݰ��е���Ч���ݵ�  Str_Info ��    �ӻ���ַ+��Ч����
	Str_Info[5] = recv_buffer[1];                // �ӻ���ַ   Drone_ID
	Str_Info[6] = recv_buffer[3];                // Distance         ��λ��cm
	Str_Info[7] = recv_buffer[4];                // ��ص����ٷֱ�   [0,100]
	Str_Info[8] = 0;                             // ����ַ���������

	printf("Rec from:%d Distance:%dcm\r\n", (int)Str_Info[5], (int)Str_Info[6]);        // ��ʾ�ӻ���ַ
	//printf("Str_length=%d; Str_Info:%s; RSSI=%d dB\r\n", (int)strlen((const char*)Str_Info), Str_Info, (int)Get_1101RSSI());
	
	// ������ӻ��ط�Ӧ���ź�    ��������������Ҫ���������״̬�����ղ������ݣ���������
	CC1101SetTRMode(TX_MODE);  
	AckBuffer[2] = Str_Info[5];        // �ӻ���ַ
	delay_us(500);
	CC1101SendPacket(AckBuffer, ACK_LENGTH, ADDRESS_CHECK);
	
	printf("Ack to %d OK\r\n", (int)Str_Info[5]);

	myfree(recv_buffer);		//�ͷ��ڴ�
	return 0;
}

// Oil_Can Queenϵͳ�����ʼ��
void Sys_Init(void)
{
	u8 *ID;

	// �̶�����
	Str_Info[0] = 0x23;                          // TCP��ͷ
	Str_Info[1] = (u8)(Queen_ID >> 24);          // Queen_ID
	Str_Info[2] = (u8)(Queen_ID >> 16);          // Queen_ID
	Str_Info[3] = (u8)(Queen_ID >> 8);           // Queen_ID
	Str_Info[4] = Queen_ID & 0xff;               // Queen_ID

	delay_init();	    	                         // ��ʱ������ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  // ����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�	  
	uart_init(115200);	 	                         // ��ʼ������1������Ϊ115200	����
	USART2_Init(115200);                             // ��ʼ������2������Ϊ115200   wifi

	LCD_Init();				                         // ��ʼ��Һ�� 	
	KEY_Init();                                      // LCD����ƿ��ؼ�
	tp_dev.init();			                         // ��ʼ��������

	SPI1_Init();                                     // CC1101 SPIͨ�ų�ʼ��
	TIM3_Init(99,7199);		                         // CC1101 1ms�ж�
	mem_init();                                      // ��ʼ���ڴ��

	printf("Oil_Can_Queen\r\n");
	while(atk_8266_init());                          // �ȴ�wifi��TCP���ӳɹ�	

	ID = mymalloc(32);							     // ����32�ֽ��ڴ�
	sprintf((char*)ID, "Queen_ID: %x Connected.", Queen_ID);
	atk_8266_wifisend_data((u8*)ID);                 // ����Queen_ID��TCP������
	myfree(ID);		                                 // �ͷ��ڴ� 

	CC1101Init(); 
}


// wifi����  ���Գ���
//	while(1)    // �������ͣ�ֱ�����ͳɹ���
//	{
//		res = atk_8266_wifisend_data((u8*)"wifi test");
//		if(res) printf("send ok\r\n");
//		else    
//		{
//			printf("send error\r\n");
//			atk_8266_init();                                        // ATK-ESP8266ģ���ʼ�����ú���
//		}
//		delay_ms(500);delay_ms(500);delay_ms(500);
//	}







