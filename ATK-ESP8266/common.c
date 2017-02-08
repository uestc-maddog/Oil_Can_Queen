#include "common.h"
#include "stdlib.h"
#include "stdio.h"
#include "lcd.h"
#include "touch.h"	

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//�û�������
const u8* portnum   = (u8*)"8080";	 

//const u8* remote_ip = (u8*)"192.168.191.1";	
//u8* wifista_ssid = (u8*)"Growl";       
//volatile u8 PWD_Temp[15] = {'1','4','7','2','5','8','3','6','9',0};       

const u8* remote_ip = (u8*)"123.207.37.14";	
u8* wifista_ssid = (u8*)"MadDog";        
volatile u8 PWD_Temp[15] = {'1','2','3','4','a','b','c','d',0};             
                                                                                                                                            
extern u8 Wifi_LinkFlag;                   // wifi������,�û�������Զ�����wifi
volatile u8 PWD_Index = 0;             // �������뻺��

volatile u8 IP_Temp[16] = {0,};        // IP���뻺��
volatile u8 IP_Index = 0;              // ��ǵ�ǰIP����

volatile u8 Pnum_Temp[7] = {0,};       // �������뻺��
volatile u8 Pnum_Index = 0;            // ��ǵ�ǰIP����

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//4������ģʽ
const u8 *ATK_ESP8266_CWMODE_TBL[3]={(u8*)"STAģʽ ",(u8*)"APģʽ ",(u8*)"AP&STAģʽ "};	//ATK-ESP8266,3������ģʽ,Ĭ��Ϊ·����(ROUTER)ģʽ 
//4�ֹ���ģʽ
const u8 *ATK_ESP8266_WORKMODE_TBL[3]={(u8*)"TCP������",(u8*)"TCP�ͻ���",(u8*)"UDPģʽ"};	//ATK-ESP8266,4�ֹ���ģʽ
//5�ּ��ܷ�ʽ
const u8 *ATK_ESP8266_ECN_TBL[5]={(u8*)"OPEN",(u8*)"WEP",(u8*)"WPA_PSK",(u8*)"WPA2_PSK",(u8*)"WPA_WAP2_PSK"};
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
volatile u8 Join_Scan = 0;           // Join ATָ���־

const u8 Kb_Num[4][3] = {            // ���ּ���
'7', '8', '9',
'4', '5', '6', 
'1', '2', '3', 
'A', '0', 'C'     // A: char/num�л�   C:delete
};

const u8 Kb_char[5][7] = {                 // ��ĸ����  Сд
'a', 'b', 'c', 'd', 'e', 'f', 'g',
'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u',
' ', 'v', 'w', 'x', 'y', 'z', ' ', 
'1', '+', '2' ,' ', ' ', ' ', ' ',    // 1: char/num�л�   +: shift   2:delete
};

const u8 Kb_Char[5][7] = {                 // ��ĸ����  Сд
'A', 'B', 'C', 'D', 'E', 'F', 'G',
'H', 'I', 'J', 'K', 'L', 'M', 'N',
'O', 'P', 'Q', 'R', 'S', 'T', 'U',
' ', 'V', 'W', 'X', 'Y', 'Z', ' ', 
'1', '+', '2' ,' ', ' ', ' ', ' ',    // 1: char/num�л�   +: shift   2:delete
};

//usmart֧�ֲ���
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART2_RX_STA;
//     1,����USART2_RX_STA;
void atk_8266_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		printf("%s",USART2_RX_BUF);	//���͵�����
		if(mode)USART2_RX_STA=0;
	} 
}
//ATK-ESP8266���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//    ����,�ڴ�Ӧ������λ��(str��λ��)
u8* atk_8266_check_cmd(u8 *str)
{
	
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//��ATK-ESP8266��������
//cmd:���͵������ַ���
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 atk_8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	u2_printf("%s\r\n",cmd);	// ��������
	if(ack&&waittime)		    // ��Ҫ�ȴ�Ӧ��
	{
		while(--waittime)	    // �ȴ�����ʱ
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//���յ��ڴ���Ӧ����
			{
				if(atk_8266_check_cmd(ack))
				{
					printf("ack:%s\r\n",(u8*)ack);
					break;//�õ���Ч���� 
				}
				USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 
//��ATK-ESP8266����ָ������
//data:���͵�����(����Ҫ��ӻس���)
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)luojian
u8 atk_8266_send_data(u8 *data,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	u2_printf("%s",data);	//��������
	if(ack&&waittime)		//��Ҫ�ȴ�Ӧ��
	{
		while(--waittime)	//�ȴ�����ʱ
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//���յ��ڴ���Ӧ����
			{
				if(atk_8266_check_cmd(ack))break;//�õ���Ч���� 
				USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
}
//ATK-ESP8266�˳�͸��ģʽ
//����ֵ:0,�˳��ɹ�;
//       1,�˳�ʧ��
u8 atk_8266_quit_trans(void)
{
	while((USART2->SR&0X40)==0);	//�ȴ����Ϳ�
	USART2->DR='+';      
	delay_ms(15);					//���ڴ�����֡ʱ��(10ms)
	while((USART2->SR&0X40)==0);	//�ȴ����Ϳ�
	USART2->DR='+';      
	delay_ms(15);					//���ڴ�����֡ʱ��(10ms)
	while((USART2->SR&0X40)==0);	//�ȴ����Ϳ�
	USART2->DR='+';      
	delay_ms(500);					//�ȴ�500ms
	return atk_8266_send_cmd((u8*)"AT",(u8*)"OK",20);//�˳�͸���ж�.
}
//��ȡATK-ESP8266ģ���AP+STA����״̬
//����ֵ:0��δ����;1,���ӳɹ�
u8 atk_8266_apsta_check(void)
{
	if(atk_8266_quit_trans())return 0;			//�˳�͸�� 
	atk_8266_send_cmd((u8*)"AT+CIPSTATUS",(u8*)":",80);	//����AT+CIPSTATUSָ��,��ѯ����״̬
	if(atk_8266_check_cmd((u8*)"+CIPSTATUS:0") && atk_8266_check_cmd((u8*)"+CIPSTATUS:1") && atk_8266_check_cmd((u8*)"+CIPSTATUS:2") &&	atk_8266_check_cmd((u8*)"+CIPSTATUS:4"))
				return 0;
	else  return 1;
}
//��ȡATK-ESP8266ģ�������״̬
//����ֵ:0,δ����;1,���ӳɹ�.
u8 atk_8266_consta_check(void)
{
	u8 *p;
	u8 res;
	if(atk_8266_quit_trans())return 0;			        // �˳�͸�� 
	atk_8266_send_cmd((u8*)"AT+CIPSTATUS",(u8*)":",50);	// ����AT+CIPSTATUSָ��,��ѯ����״̬
	p=atk_8266_check_cmd((u8*)"+CIPSTATUS:");           // +CIPSTATUS:��Ϊid��
	res=*p;									            // �õ�����״̬	
	return res;
}


//��ȡClient ip��ַ
//ipbuf:ip��ַ���������
void atk_8266_get_wanip(u8* ipbuf)
{
	u8 *p,*p1;
	if(atk_8266_send_cmd((u8*)"AT+CIFSR",(u8*)"OK",50))//��ȡWAN IP��ַʧ��
	{
		ipbuf[0]=0;
		return;
	}		
	p=atk_8266_check_cmd((u8*)"\"");
	p1=(u8*)strstr((const char*)(p+1),(const char*)"\"");
	*p1=0;
	sprintf((char*)ipbuf,"%s",p+1);	
}


// ATK-ESP8266ģ���ʼ�����ú���
// ����ֵ��0��wifi��TCP���ӳɹ�     1��ERROR��wifiδ�����������������     
u8 atk_8266_init(void)
{
	u8 timer = 0, Wifi_Link_Timer = 0;
	u8 ipbuf[16]; 	                             // IP���뻺��
	u8 *p = mymalloc(32);				         // ����32�ֽ��ڴ�

	while(atk_8266_send_cmd((u8*)"AT",(u8*)"OK",20))  //���WIFIģ���Ƿ�����
	{
		atk_8266_quit_trans();                        //�˳�͸��
		atk_8266_send_cmd((u8*)"AT+CIPMODE=0",(u8*)"OK",200);  //�ر�͸��ģʽ	
		printf("δ��⵽wifiģ��!!!\r\n");
		delay_ms(1200);
		printf("��������wifiģ��...\r\n");
	} 
	printf("�ɹ���⵽wifiģ��...\r\n");
	while(atk_8266_send_cmd((u8*)"ATE0",(u8*)"OK",20));      //�رջ���
	
	delay_ms(10); 
	atk_8266_at_response(1);//���ATK-ESP8266ģ�鷢�͹���������,��ʱ�ϴ�������

	printf("��������ATK-ESP8266ģ�飬���Ե�...\r\n");
	
	atk_8266_send_cmd((u8*)"AT+CWMODE=1",(u8*)"OK",50);		     // ����WIFI STAģʽ
	atk_8266_send_cmd((u8*)"AT+RST",(u8*)"OK",20);		         // DHCP�������ر�(��APģʽ��Ч) 
	delay_ms(1000);         //��ʱ3S�ȴ������ɹ�
	delay_ms(1000);
	delay_ms(1000);
//	delay_ms(100);          //delay_ms(800);

	/*************************�û��ֶ�����wifi*********************************/
	while(1)
	{
		if(Wifi_LinkFlag == 1) break;  // �ѽ��������ӣ���ֱ�������ϴ�ѡ���wifi
		if(AP_Choose() == 0)           // �ɹ�ѡ��wifi
		{
			PWD_Index = 0;                 // ��ǵ�ǰ���볤��
			if(Enter_AP_PWD() == 0) break; // �ɹ���������
		}
	}
	
	// ����wifi�����������ʾ������ʾ
	LCD_Clear(WHITE);	
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_ShowString(18,120,200,24,24,(u8*)"Wifi Connecting");
	LCD_ShowString(25,150,200,24,24,(u8*)" Please wait");
	/*************************�û��ֶ�����wifi*********************************/
	
	
	//�������ӵ���WIFI��������/���ܷ�ʽ/����,�⼸��������Ҫ�������Լ���·�������ý����޸�!! 
	//printf("\r\n%s %s\r\n",wifista_ssid,wifista_password); //��ӡ���߲���:ssid,����
	printf("\r\n%s %s\r\n",wifista_ssid,PWD_Temp);        // ��ӡ���߲���:ssid,����

	sprintf((char*)p,"AT+CWJAP=\"%s\",\"%s\"",wifista_ssid,PWD_Temp);//�������߲���:ssid,����
	timer = 0; Wifi_Link_Timer = 0;                        //����
	while(atk_8266_send_cmd(p,(u8*)"WIFI GOT IP",300))     //����Ŀ��·����,���һ��IP
	{
		printf("��������Ŀ��wifi��%s����ȷ�ϸ�wifi�ѿ���������\r\n", wifista_ssid);
		LCD_Clear(WHITE);	
		POINT_COLOR = BLUE;
		BACK_COLOR = WHITE;
		LCD_ShowString(18,120,200,24,24,(u8*)"Wifi Connecting");
		if(++timer == 1)    LCD_ShowString(25,150,200,24,24,(u8*)" Please wait.");
		else if(timer == 2) LCD_ShowString(25,150,200,24,24,(u8*)" Please wait..");
		else if(timer == 3) 
		{
			timer = 0;
			LCD_ShowString(25,150,200,24,24,(u8*)" Please wait...");
		}
		delay_ms(300);
		if(++Wifi_Link_Timer == 6)           // wifi���ӳ�ʱ   ��ʾ�û���1�����wifi�Ƿ��ѳɹ�����  2��ȷ��wifi�����Ƿ�����ɹ�
		{
			LCD_Clear(WHITE);	
			POINT_COLOR = BLUE;
			BACK_COLOR = WHITE;
			LCD_ShowString(25, 90,200,24,24,(u8*)"  Please check");   // Please check whether the WiFi is turned on
			LCD_ShowString(25,120,200,24,24,(u8*)"  the wifi and");
			LCD_ShowString(25,150,200,24,24,(u8*)"  the password");  // Confirm that the WiFi password is correct
			POINT_COLOR = BLACK;
			
			myfree(p);		//�ͷ��ڴ� 
			USART2_RX_STA = 0;
			return 1;
		}
	}	
	LCD_Clear(WHITE);	
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_ShowString(15,120,225,16,16,(u8*)"wifi Connection successful!");
	
	Wifi_LinkFlag = 1;                        // ����ѽ��������ӣ���ֱ�������ϴ�ѡ���wifi
	printf("�ɹ�����Ŀ��wifi��%s\r\n", wifista_ssid);
	delay_ms(650);delay_ms(650);
	delay_ms(650);
	// Enter TCP IP Addr
//	IP_Temp[0] = 0;        // �������뻺��
//    IP_Index = 0;        // ��ǵ�ǰIP����
//	if(Enter_TCP_IP())   goto wifi_scan;        // ����IP���˿�ʧ��
	
	printf("��������TCP������ %s:%s\r\n", (u8*)remote_ip, (u8*)portnum);
	
	// �ɹ�����TCP������IP
	atk_8266_send_cmd((u8*)"AT+CIPMUX=0",(u8*)"OK",20);   //0�������ӣ�1��������
	sprintf((char*)p,(const char*)"AT+CIPSTART=\"TCP\",\"%s\",%s",(u8*)remote_ip,(u8*)portnum);    //����Ŀ��TCP������
	timer = 0; Wifi_Link_Timer = 0;                        //����
	while(atk_8266_send_cmd(p,(u8*)"OK",300))
	{
		printf("TCP����ʧ�ܣ���ȷ�Ϸ������Ƿ��\r\n");
		LCD_Clear(WHITE);	
		POINT_COLOR = BLUE;
		BACK_COLOR = WHITE;
		LCD_ShowString(25,120,200,24,24,(u8*)"TCP Connecting");
		if(++timer == 1)    LCD_ShowString(25,150,200,24,24,(u8*)" Please wait.");
		else if(timer == 2) LCD_ShowString(25,150,200,24,24,(u8*)" Please wait..");
		else if(timer == 3) 
		{
			timer = 0;
			LCD_ShowString(25,150,200,24,24,(u8*)" Please wait...");
		}
		delay_ms(500);
		
		if(++Wifi_Link_Timer == 6)           // TCP���������ӳ�ʱ 
		{
			LCD_Clear(WHITE);	
			POINT_COLOR = BLUE;
			BACK_COLOR = WHITE;
			LCD_ShowString(25,120,200,24,24,(u8*)"   TCP Server");   // Please check whether the WiFi is turned on
			LCD_ShowString(25,150,200,24,24,(u8*)"Connection error");
			POINT_COLOR = BLACK;
			
			myfree(p);		//�ͷ��ڴ� 
			USART2_RX_STA = 0;
			return 2;
		}
	}	
	printf("TCP���ӳɹ�\r\n");
	LCD_Clear(WHITE);	
	LCD_ShowString(15,120,225,16,16,(u8*)"TCP Connection successful!");
	delay_ms(500);delay_ms(500);delay_ms(500);
	QueenRun_UI();                                             // queen ������������UI
	atk_8266_send_cmd((u8*)"AT+CIPMODE=1",(u8*)"OK",200);      // ����ģʽΪ��͸��		
		
	atk_8266_get_wanip(ipbuf);                                 // ������ģʽ,��ȡWAN IP
	sprintf((char*)p,"IP��ַ:%s �˿�:%s",ipbuf,(u8*)portnum);
	printf("%s\r\n", p);
	printf("����ģʽ��%s\r\n", (u8*)ATK_ESP8266_WORKMODE_TBL[1]);      // TCP�ͻ���
	
	myfree(p);		//�ͷ��ڴ� 
	USART2_RX_STA=0;
	
	return 0;
}

// ESP_8266ɨ�����AP��������
// ����ֵ�� 0--����AP�ɹ�    other--ʧ�ܣ�Ӧ��ʱ
u8 AP_Choose(void)
{
	volatile u8 waittime = 200;                   // waittime:Ӧ��ȴ�ʱ��   20ms
	volatile u8 AP_TAB[25][30] = {0,};            // wifi �ȵ�������
	volatile u8 Flag = 0;
	volatile u16 i = 0, m = 0, n = 0, len = 0, timer = 0;
	
	while(1)
	{	
		LCD_Clear(WHITE); 
		LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning...");    // ��ʾwifiɨ����
		
		waittime = 240; Flag = 0; timer = 0;
		for(m = 0; m < 25; m++)
		{
			for(n = 0; n < 30; n++) AP_TAB[m][n] = 0;
		}
		i = 0; m = 0; n = 0; len = 0;
		USART2_RX_BUF[0] = 0;                // ���ڻ�������
		USART2_RX_STA = 0;
		Join_Scan = 1;
		
		u2_printf("AT+CWLAP=\r\n");	         // ��������  ɨ�����AP    Ӧ���ʽ��+CWLAP:(4,"MadDog",-42,"e4:d3:32:d0:22:fe",1,31,0)\r\n...
		while(--waittime)	                 // �ȴ�Ӧ��  ����ʱ
		{
			delay_ms(25);
			if(USART2_RX_STA&0X8000)         // ���յ�Ӧ��
			{
				Join_Scan = 0;
				len = USART2_RX_STA&0X7FFF;
				USART2_RX_BUF[len] = 0;      // ����ַ���������
				break;
			} 
		}
		if(waittime == 0) 
		{
			printf("Wifi Scan Ack Error...Retrying\r\n");
			LCD_Clear(WHITE);
			LCD_ShowString(25,120,200,24,24,(u8*)"There is no wifi!");    // ��ʾɨ�費��wifi
			LCD_ShowString(25,150,200,24,24,(u8*)"Retry the scan.");    // ��ʾwifiɨ����
			delay_ms(1000);
			LCD_ShowString(25,150,200,24,24,(u8*)"Retry the scan..");    // ��ʾwifiɨ����
			delay_ms(1000);
			LCD_ShowString(25,150,200,24,24,(u8*)"Retry the scan...");    // ��ʾwifiɨ����
			delay_ms(1000);
			continue;                        // 6sӦ��ʱ
		}

		// �ɹ�����Ӧ��
	//	printf("%d len=%d\r\n", waittime, USART2_RX_STA&0X7FFF);
	//	printf("%s\r\n", USART2_RX_BUF);
		for(i = 0; i < len; i++)              // ��wifi �ȵ������浽AP_TAB
		{
			if(USART2_RX_BUF[i] == '"')
			{
				if(USART2_RX_BUF[i-3] == '(') Flag = 1;
				else                          
				{
					if(Flag)
					{
						Flag = 0;  
						AP_TAB[m][n] = 0;
						m++;
						n = 0;
						i += 28;
						continue;
					}
				}				
			}
			if(Flag)
			{	
				if(USART2_RX_BUF[i+1] != '"') AP_TAB[m][n++] = USART2_RX_BUF[i+1];
			}
		}
		LCD_Fill(0,0,239,47,WHITE);
		POINT_COLOR = BLUE;
		LCD_ShowString(15,15,240,16,16, (u8*)"Please select wifi:"); 
		LCD_Fill(0,48,239,319,0xEEEE);		   				     // ��䵥ɫ
		POINT_COLOR = BLACK;
		LCD_DrawLine(0, 48, 239, 48);		         // ����

		for(i = 0; i < m; i++)
		{
			printf("AP_TAB[%d]:%s\r\n", i, AP_TAB[i]);
			LCD_ShowString(60,60+20*i,240,16,16,(u8*)AP_TAB[i]); 
			if(i != m-1) LCD_DrawLine(60, 78+20*i, 239, 78+20*i);		     //����
		}
		LCD_DrawLine(0, 68+20*i, 239, 68+20*i);		         // ����
		while(1)    // �ȴ��û�����ѡ��
		{
			tp_dev.scan(0); 		 
			if(tp_dev.sta&TP_PRES_DOWN)			// ������������
			{	
				if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
				{	
					if(tp_dev.y[0] > 60)        // AP��ʾ������
					{
						int Index = (tp_dev.y[0] - 60) / 20;
						wifista_ssid = (u8*)AP_TAB[Index];
						
						POINT_COLOR = RED;
						LCD_ShowString(60,60+20*Index,240,16,16,(u8*)AP_TAB[Index]);    // �û�ѡ��AP ��ɫ��ʾ
						POINT_COLOR = BLACK;
						
						printf("wifista_ssid:%s\r\n", wifista_ssid);
						delay_ms(300);
						return 0;
					}		   
				}
			}
			else 
			{
				delay_ms(10);	              // û�а������µ�ʱ�� 	 
				if(++timer == 950) break;     // 10s��δѡ�����磬��ˢ������
			}				
		}
	}
}

u8 Enter_AP_PWD(void)
{
	volatile u8 i = 0, j = 0, Touch_Up = 1, timer = 0;
	
	LCD_Clear(WHITE);	 								
	
	POINT_COLOR = BLUE;
	LCD_ShowString(64,20,200,16,16, (u8*)"Enter The PWD of"); 
	LCD_ShowString(64,40,200,16,16, (u8*)wifista_ssid); 
	
	POINT_COLOR = RED;
	BACK_COLOR = 0xEEEE;
	LCD_Fill(0,25,52,50,0xEEEE);	
	LCD_ShowString(2,30,50,16,16, (u8*)"Cancel"); 
	
	POINT_COLOR = GRAY;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
	LCD_Fill(203,25,239,50,0xEEEE);	
	LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
	
	
	POINT_COLOR = BLACK;
	BACK_COLOR = WHITE;
	LCD_DrawLine(0, 75, 239, 75);
	LCD_ShowString(20,80,200,16,16, (u8*)"PWD:"); 
	LCD_DrawLine(0, 101, 239, 101);
	
	// �������ּ���UI
	KbNum_UI(0);      // 0--�������� 
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// ������������
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 25 && tp_dev.y[0] <= 50)
				{
					if(tp_dev.x[0] < 52)                // Cancel 
					{
						POINT_COLOR = RED;                               // Cancel����Ч��  �����½�ƫ��2����
						BACK_COLOR = 0xEEEE;
						LCD_Fill(0,25,52,50,WHITE);           // ���ԭ������ʾ
						LCD_Fill(2,27,54,52,0xEEEE);	
						LCD_ShowString(4,32,50,16,16, (u8*)"Cancel"); 
						
						delay_ms(500);
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;                         
						BACK_COLOR = WHITE;
						LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
						return 1;
					}
					if(tp_dev.x[0] > 203)               // Join
					{
						if(PWD_Index >= 8)              // �������볤�� > 8     ��Ч����
						{
//							wifista_password = (u8*)PWD_Temp;  // ��ȡ������
							BACK_COLOR = 0xEEEE;                          // Join����Ч��  �����½�ƫ��2����
							POINT_COLOR = RED; 
							LCD_Fill(203,25,239,50,WHITE);    // ���ԭ������ʾ
							LCD_Fill(201,27,237,52,0xEEEE);	
							LCD_ShowString(203,32,35,16,16, (u8*)"Join");
							
							printf("wifista_password:%s\r\n", PWD_Temp);
							delay_ms(300);
							return 0;
						}
					}
				}
				if(tp_dev.y[0] > 120)        // ���ּ�����ʾ������
				{
					Touch_Up = 0;
					i = (tp_dev.y[0] - 120) / 50;
					j = tp_dev.x[0] / 80;
					if(Kb_Num[i][j] >= '0' && Kb_Num[i][j] <= '9')    // ����0~9
					{
						LCD_ShowChar(60+PWD_Index*10,80,Kb_Num[i][j],16,0);  // LCD��ʾ
						PWD_Temp[PWD_Index++] = Kb_Num[i][j];
						PWD_Temp[PWD_Index] = 0;     // ����ַ���������
						printf("PWD_Temp:%s\r\n", PWD_Temp);
						
						if(PWD_Index >= 8)           // �������볤�� >= 8
						{
							POINT_COLOR = RED;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
							BACK_COLOR = 0xEEEE;
							LCD_Fill(203,25,239,50,0xEEEE);	
							LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
							POINT_COLOR = BLACK;
							BACK_COLOR = WHITE;
						}
						delay_ms(300);
					}
					else
					{
						if(Kb_Num[i][j] == 'A')  // �л�����ĸ����
						{
							u8 res = Load_Kb_Char();
							if(res == 0)      return 0;      // ��������ɹ�
							else if(res == 1) KbNum_UI(0);   // �������ּ���  0--�������� 
							else              return 1;      // Cancel
						} 
						else                     // ɾ��ǰһ���ַ�            
						{
							if(PWD_Index > 0) PWD_Index--;
							LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);  // LCD��ʾ
							LCD_ShowChar(70+PWD_Index*10,80,' ',16,0);  // LCD��ʾ
							PWD_Temp[PWD_Index] = 0;
							printf("PWD_Temp:%s\r\n", PWD_Temp);
							if(PWD_Index < 8)           // �������볤�� < 8
							{
								POINT_COLOR = GRAY;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
								BACK_COLOR = 0xEEEE; 
								LCD_Fill(203,25,239,50,0xEEEE);	
								LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
								POINT_COLOR = BLACK;
								BACK_COLOR = WHITE;
							}
							delay_ms(300);
						}							
					}
				}	
			}
		}
		else                              //û�а���	
		{
			Touch_Up = 1;
		}		
		delay_ms(10);	
		if(++timer == 50)
		{
			LCD_ShowChar(60+PWD_Index*10,80,'|',16,0);
		}
		if(timer == 100)
		{
			timer = 0;
			LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);
		}
	}		
}

// ������ĸ����   
// ����ֵ��0--��������ɹ�    1--�������ּ���    2--ȡ���������룬����ɨ��wifi
u8 Load_Kb_Char(void)  
{
	volatile u8 Char_Shitf = 0;         // ��/Сд��ĸ��־
	volatile u8 i = 0, j = 0, Touch_Up = 1, timer = 0;

	delay_ms(500);
	KbChar_UI(Char_Shitf);
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// ������������
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 25 && tp_dev.y[0] <= 50)
				{
					if(tp_dev.x[0] < 52)                // Cancel 
					{
						POINT_COLOR = RED;                               // Cancel����Ч��  �����½�ƫ��2����
						BACK_COLOR = 0xEEEE;
						LCD_Fill(0,25,52,50,WHITE);           // ���ԭ������ʾ
						LCD_Fill(2,27,54,52,0xEEEE);	
						LCD_ShowString(4,32,50,16,16, (u8*)"Cancel");
						
						delay_ms(500);
						LCD_Clear(WHITE);
						POINT_COLOR = BLACK;                         
						BACK_COLOR = WHITE;
						LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
						return 2;
					}
					if(tp_dev.x[0] > 203)               // Join
					{
						if(PWD_Index >= 8)           // �������볤�� > 8
						{
//							wifista_password = (u8*)PWD_Temp;  // ��ȡ������
							BACK_COLOR = 0xEEEE;                          // Join����Ч��  �����½�ƫ��2����
							POINT_COLOR = RED; 
							LCD_Fill(203,25,239,50,WHITE);    // ���ԭ������ʾ
							LCD_Fill(201,27,237,52,0xEEEE);	
							LCD_ShowString(203,32,35,16,16, (u8*)"Join");
							
							printf("wifista_password:%s\r\n", PWD_Temp);
							delay_ms(300);
							return 0;
						}
					}
				}
				if(tp_dev.y[0] > 120)        // ������ʾ������
				{
					Touch_Up = 0;
					
					if(tp_dev.y[0] < 280)    // 26����ĸ��
					{
						i = (tp_dev.y[0] - 120) / 40;
						j = tp_dev.x[0] / 34;
						printf("%c ", Kb_char[i][j]);
						if(Char_Shitf)    // ��д
						{
							if(Kb_Char[i][j] >= 'A' && Kb_Char[i][j] <= 'Z')    // ����A~Z
							{
								LCD_ShowChar(60+PWD_Index*10,80,Kb_Char[i][j],16,0);  // LCD��ʾ
								PWD_Temp[PWD_Index++] = Kb_Char[i][j];
								PWD_Temp[PWD_Index] = 0;     // ����ַ���������
								printf("PWD_Temp:%s\r\n", PWD_Temp);
								
								if(PWD_Index >= 8)           // �������볤�� >= 8
								{
									POINT_COLOR = RED;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
									BACK_COLOR = 0xEEEE;
									LCD_Fill(203,25,239,50,0xEEEE);	
									LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
									POINT_COLOR = BLACK;
									BACK_COLOR = WHITE;
								}
								delay_ms(300);
							}
						}
						else         // Сдģʽ
						{
							if(Kb_char[i][j] >= 'a' && Kb_char[i][j] <= 'z')    // ����a~z
							{													
								LCD_ShowChar(60+PWD_Index*10,80,Kb_char[i][j],16,0);  // LCD��ʾ
								PWD_Temp[PWD_Index++] = Kb_char[i][j];
								PWD_Temp[PWD_Index] = 0;     // ����ַ���������
								printf("PWD_Temp:%s\r\n", PWD_Temp);
								
								if(PWD_Index >= 8)           // �������볤�� >= 8
								{
									POINT_COLOR = RED;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
									BACK_COLOR = 0xEEEE;
									LCD_Fill(203,25,239,50,0xEEEE);	
									LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
									POINT_COLOR = BLACK;
									BACK_COLOR = WHITE;
								}
								delay_ms(300);
							}
						}
					}
					else                       
					{
						//printf("x=%d y=%d\r\n", tp_dev.x[0], tp_dev.y[0]);
						i = (tp_dev.y[0] - 120) / 40;
						j = tp_dev.x[0] / 80;
						//printf("%d %d\r\n", i, j);
						
						if(Kb_char[i][j] == '1')       // �л������ּ���
						{
							delay_ms(800);
							return 1;         
						} 
						else if(Kb_char[i][j] == '+')  // �л���д��ĸ����
						{
							if(Char_Shitf == 0) Char_Shitf = 1;   
							else                Char_Shitf = 0; 
							delay_ms(700);
							KbChar_UI(Char_Shitf);
						} 
						else if(Kb_char[i][j] == '2')  // ɾ��ǰһ���ַ�            
						{
							if(PWD_Index > 0) PWD_Index--;
							LCD_ShowString(60+PWD_Index*10,80,200,16,16, (u8*)"                   "); 
//							LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);  // LCD��ʾ
//							LCD_ShowChar(70+PWD_Index*10,80,' ',16,0);  // LCD��ʾ
							PWD_Temp[PWD_Index] = 0;
							printf("PWD_Temp:%s\r\n", PWD_Temp);
							if(PWD_Index < 8)           // �������볤�� < 8
							{
								POINT_COLOR = GRAY;                             // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
								BACK_COLOR = 0xEEEE; 
								LCD_Fill(203,25,239,50,0xEEEE);	
								LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
								POINT_COLOR = BLACK;
								BACK_COLOR = WHITE;
							}
							delay_ms(300);
						}							
					}
					
				}	
			}
		}
		else                              //û�а���	
		{
			Touch_Up = 1;
		}		
		delay_ms(10);	
		if(++timer == 50)
		{
			LCD_ShowChar(60+PWD_Index*10,80,'|',16,0);
		}
		if(timer == 100)
		{
			timer = 0;
			LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);
		}
	}		
}

// ���ּ���UI 
u8 KbNum_UI(u8 index)
{
	volatile u8 i = 0, j = 0;
	
	LCD_Fill(0,102,239,319,0xEEEE);	
	BACK_COLOR = 0xEEEE;
	POINT_COLOR = BLACK;
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(Kb_Num[i][j] >= '0' && Kb_Num[i][j] <= '9')
			{
				LCD_ShowChar(36+80*j,120+50*i,Kb_Num[i][j],16,0);
			}
			else
			{
				if(Kb_Num[i][j] == 'A') 
				{
					if(index) LCD_ShowString(28+80*j,120+50*i,200,16,16, (u8*)" . ");
					else      LCD_ShowString(28+80*j,120+50*i,200,16,16, (u8*)"ABC");
				}
				else                    LCD_ShowString(28+80*j,120+50*i,200,16,16, (u8*)"del");   
			}
		}
	}
	for(i = 0; i < 3; i++) LCD_DrawLine(0, 153+50*i, 239, 153+50*i);     // �����̺���
	for(i = 0; i < 2; i++) LCD_DrawLine(80+80*i, 101, 80+80*i, 319);     // ����������
	BACK_COLOR = WHITE;
	return 0;
}

// ��ĸ����UI  shift��0--Сд  else--��д
u8 KbChar_UI(u8 shift)
{
	volatile u8 i = 0, j = 0;
	
	// ������ĸ����
	LCD_Fill(0,102,239,319,0xEEEE);	 // �����������
	BACK_COLOR = 0xEEEE;
	POINT_COLOR = BLACK;
	if(shift)             // ��д��ĸ
	{
		for(i = 0; i < 5; i++)
		{
			for(j = 0; j < 7; j++)
			{
				if(i < 3)       // 7 * 3 
				{
					LCD_ShowChar(14+34*j,120+40*i,Kb_Char[i][j],16,0);
				}
				else if(i == 3) // 5
				{
					if(j == 0 || j == 6) continue;
					LCD_ShowChar(14+34*j,120+40*i,Kb_Char[i][j],16,0);
				}
				else            // 3
				{
					if(Kb_char[i][j] == '1') LCD_ShowString(28,     120+40*i,200,16,16, (u8*)"123");    // char/num�л�
					if(Kb_char[i][j] == '+') LCD_ShowString(20+80*1,120+40*i,200,16,16, (u8*)"shift");  // ��Сд�л�
					if(Kb_char[i][j] == '2') LCD_ShowString(28+80*2,120+40*i,200,16,16, (u8*)"del");    // ɾ��
				}
			}
		}
	}
	else                   // Сд��ĸ
	{
		for(i = 0; i < 5; i++)
		{
			for(j = 0; j < 7; j++)
			{
				if(i < 3)       // 7 * 3 
				{
					LCD_ShowChar(14+34*j,120+40*i,Kb_char[i][j],16,0);
				}
				else if(i == 3) // 5
				{
					if(j == 0 || j == 6) continue;
					LCD_ShowChar(14+34*j,120+40*i,Kb_char[i][j],16,0);
				}
				else            // 3
				{
					if(Kb_char[i][j] == '1') LCD_ShowString(28,     120+40*i,200,16,16, (u8*)"123");    // char/num�л�
					if(Kb_char[i][j] == '+') LCD_ShowString(20+80*1,120+40*i,200,16,16, (u8*)"Shift");  // ��Сд�л�
					if(Kb_char[i][j] == '2') LCD_ShowString(28+80*2,120+40*i,200,16,16, (u8*)"del");    // ɾ��
				}
			}
		}
	}
	BACK_COLOR = WHITE;
	return 0;
}

// ����wifi���룬�������ּ���
// ����ֵ�� 0--����IP�ɹ�    1--Cancel ����ɨ��wifi
u8 Enter_TCP_IP(void)
{
	volatile u8 i = 0, j = 0, Touch_Up = 1, timer = 0, dot_timer = 0;
	typedef enum{Enter_IPAddr = 0, Enter_IPCom,} Enter_TCP_Sta;
	volatile Enter_TCP_Sta my_tcp_sta = Enter_IPAddr; 
		
	LCD_Clear(WHITE);	 								
	
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_ShowString(64,15,200,16,16, (u8*)"Enter The TCP IP");  // 30
	
	POINT_COLOR = RED;
	BACK_COLOR = 0xEEEE;
	LCD_Fill(0,10,52,35,0xEEEE);	
	LCD_ShowString(2,15,50,16,16, (u8*)"Cancel"); 
	
	POINT_COLOR = GRAY;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
	LCD_Fill(203,10,239,35,0xEEEE);	
	LCD_ShowString(205,15,35,16,16, (u8*)"Done"); 
	
	
	POINT_COLOR = BLACK;
	BACK_COLOR = WHITE;
	LCD_DrawLine(0, 49, 239, 49);
	LCD_ShowString(12,54,200,16,16, (u8*)"  IP:"); 
	
	LCD_DrawLine(0, 75, 239, 75);
	LCD_ShowString(12,80,200,16,16, (u8*)"Port:"); 
	LCD_DrawLine(0, 101, 239, 101);
	
	// �������ּ���UI
	KbNum_UI(1);      // 1--TCP_IP����
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// ������������
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 49 && tp_dev.y[0] <= 75)      my_tcp_sta = Enter_IPAddr;     // ������IP��������
				else if(tp_dev.y[0] > 75 && tp_dev.y[0] <= 101) my_tcp_sta = Enter_IPCom;      // ������Com��������
				printf("my_tcp_sta:%d\r\n", (int)my_tcp_sta);
				
				if(tp_dev.y[0] >= 10 && tp_dev.y[0] <= 35)
				{
					if(tp_dev.x[0] < 52)                // Cancel 
					{
						delay_ms(400);
						LCD_Clear(WHITE);
						LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
						return 1;
					}
					if(tp_dev.x[0] > 203)               // Done
					{
						if(dot_timer >= 3 && IP_Temp[IP_Index-1] != '.')            
						{
							if(Pnum_Index >= 1 && Pnum_Index <= 5)      // �˿ںų���Ϊ 1~5
							{
								printf("IP:%s Port:%s\r\n", IP_Temp, Pnum_Temp);
								delay_ms(300);
								return 0;
							}
						}
					}
				}
				if(tp_dev.y[0] > 120)        // ���ּ�����ʾ������
				{
					Touch_Up = 0;
					i = (tp_dev.y[0] - 120) / 50;
					j = tp_dev.x[0] / 80;
					if(Kb_Num[i][j] >= '0' && Kb_Num[i][j] <= '9')    // ����0~9
					{
						LCD_ShowChar(60+IP_Index*10,80,Kb_Num[i][j],16,0);  // LCD��ʾ
						IP_Temp[IP_Index++] = Kb_Num[i][j];
						IP_Temp[IP_Index] = 0;     // ����ַ���������
						printf("IP_Temp:%s\r\n", IP_Temp);
						
						if(dot_timer >= 3 && IP_Temp[IP_Index-1] != '.')           // �������볤�� >= 15
						{
							POINT_COLOR = RED;                               // ���������볤�� > 8 ʱ�� ��GRAY -> RED, ������Ч
							BACK_COLOR = 0xEEEE;
							LCD_Fill(203,25,239,50,0xEEEE);	
							LCD_ShowString(205,30,35,16,16, (u8*)"Done"); 
							POINT_COLOR = BLACK;
							BACK_COLOR = WHITE;
						}
						delay_ms(500);
					}
					else
					{
						if(Kb_Num[i][j] == 'A')  // '.'
						{
							dot_timer++;
							LCD_ShowChar(60+IP_Index*10,80,'.',16,0);  // LCD��ʾ
							IP_Temp[IP_Index++] = '.';
							IP_Temp[IP_Index] = 0;     // ����ַ���������
							printf("PWD_Temp:%s dot_timer = %d IP_Temp[IP_Index-1] = %d\r\n", IP_Temp, dot_timer, IP_Temp[IP_Index-1]);
							
							delay_ms(500);
						} 
						else                     // ɾ��ǰһ���ַ�            
						{
							if(IP_Index > 0) 
							{
								IP_Index--;
								if(IP_Temp[IP_Index-1] == '.') 
								{
									if(dot_timer>0) dot_timer--;
								}
							}
							LCD_ShowChar(60+IP_Index*10,80,' ',16,0);  // LCD��ʾ
							LCD_ShowChar(70+IP_Index*10,80,' ',16,0);  // LCD��ʾ
							IP_Temp[IP_Index] = 0;
							printf("PWD_Temp:%s\r\n", IP_Temp);
							if( (dot_timer < 3) || ((dot_timer == 3)&&(IP_Temp[IP_Index-1] == '.')) )          // '.'���� < 3
							{
								POINT_COLOR = GRAY;                                  // ���������볤�� > 15 ʱ�� ��GRAY -> RED, ������Ч
								BACK_COLOR = 0xEEEE; 
								LCD_Fill(203,25,239,50,0xEEEE);	
								LCD_ShowString(205,30,35,16,16, (u8*)"Done"); 
								POINT_COLOR = BLACK;
								BACK_COLOR = WHITE;
							}
							delay_ms(500);
						}							
					}
				}	
			}
		}
		else                              //û�а���	
		{
			Touch_Up = 1;
		}		
		delay_ms(10);	
		if(++timer == 50)
		{
			if(my_tcp_sta == Enter_IPAddr)              // IP
			{
				LCD_ShowChar(60+IP_Index*10,54,'|',16,0);      
				LCD_ShowChar(60+Pnum_Index*10,80,' ',16,0);
			}
			else                                        // Port
			{
				LCD_ShowChar(60+Pnum_Index*10,80,'|',16,0);  
				LCD_ShowChar(60+IP_Index*10,54,' ',16,0);
			}
		}
		if(timer == 100)
		{
			timer = 0;
			if(my_tcp_sta == Enter_IPAddr) 
			{
				LCD_ShowChar(60+IP_Index*10,54,' ',16,0);
				LCD_ShowChar(60+Pnum_Index*10,80,' ',16,0);
			}
			else                          
			{
				LCD_ShowChar(60+Pnum_Index*10,80,' ',16,0);
				LCD_ShowChar(60+IP_Index*10,54,' ',16,0);
			}
		}
	}		
}








































