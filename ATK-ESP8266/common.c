#include "common.h"
#include "stdlib.h"
#include "stdio.h"
#include "lcd.h"
#include "touch.h"	
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//用户配置区

//连接端口号:8080,可自行修改为其他端口.
const u8* remote_ip = (u8*)"192.168.191.1";	
//const u8* remote_ip = "222.212.204.66";	
const u8* portnum   = (u8*)"8080";	 

//WIFI STA模式,设置要去连接的路由器无线参数,请根据你自己的路由器设置,自行修改.
u8* wifista_ssid = (u8*)"growl";       // 路由器SSID号
extern u8 Link_Flag;                   // 标记已建立过连接，可直接连入上次选择的wifi
volatile u8 PWD_Temp[15] = {0,};       // 密码输入缓存
volatile u8 PWD_Index = 0;             // 标记当前密码长度

volatile u8 IP_Temp[16] = {0,};        // 密码输入缓存
volatile u8 IP_Index = 0;              // 标记当前IP长度

volatile u8 Pnum_Temp[7] = {0,};       // 密码输入缓存
volatile u8 Pnum_Index = 0;            // 标记当前IP长度

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//4个网络模式
const u8 *ATK_ESP8266_CWMODE_TBL[3]={(u8*)"STA模式 ",(u8*)"AP模式 ",(u8*)"AP&STA模式 "};	//ATK-ESP8266,3种网络模式,默认为路由器(ROUTER)模式 
//4种工作模式
const u8 *ATK_ESP8266_WORKMODE_TBL[3]={(u8*)"TCP服务器",(u8*)"TCP客户端",(u8*)"UDP模式"};	//ATK-ESP8266,4种工作模式
//5种加密方式
const u8 *ATK_ESP8266_ECN_TBL[5]={(u8*)"OPEN",(u8*)"WEP",(u8*)"WPA_PSK",(u8*)"WPA2_PSK",(u8*)"WPA_WAP2_PSK"};
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
volatile u8 Join_Scan = 0;           // Join AT指令标志

const u8 Kb_Num[4][3] = {            // 数字键盘
'7', '8', '9',
'4', '5', '6', 
'1', '2', '3', 
'A', '0', 'C'     // A: char/num切换   C:delete
};

const u8 Kb_char[5][7] = {                 // 字母键盘  小写
'a', 'b', 'c', 'd', 'e', 'f', 'g',
'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u',
' ', 'v', 'w', 'x', 'y', 'z', ' ', 
'1', '+', '2' ,' ', ' ', ' ', ' ',    // 1: char/num切换   +: shift   2:delete
};

const u8 Kb_Char[5][7] = {                 // 字母键盘  小写
'A', 'B', 'C', 'D', 'E', 'F', 'G',
'H', 'I', 'J', 'K', 'L', 'M', 'N',
'O', 'P', 'Q', 'R', 'S', 'T', 'U',
' ', 'V', 'W', 'X', 'Y', 'Z', ' ', 
'1', '+', '2' ,' ', ' ', ' ', ' ',    // 1: char/num切换   +: shift   2:delete
};

//usmart支持部分
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART2_RX_STA;
//     1,清零USART2_RX_STA;
void atk_8266_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART2_RX_BUF);	//发送到串口
		if(mode)USART2_RX_STA=0;
	} 
}
//ATK-ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* atk_8266_check_cmd(u8 *str)
{
	
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//向ATK-ESP8266发送命令
//cmd:发送的命令字符串
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 atk_8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	u2_printf("%s\r\n",cmd);	// 发送命令
	if(ack&&waittime)		    // 需要等待应答
	{
		while(--waittime)	    // 等待倒计时
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(atk_8266_check_cmd(ack))
				{
					printf("ack:%s\r\n",(u8*)ack);
					break;//得到有效数据 
				}
				USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 
//向ATK-ESP8266发送指定数据
//data:发送的数据(不需要添加回车了)
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)luojian
u8 atk_8266_send_data(u8 *data,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	u2_printf("%s",data);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(atk_8266_check_cmd(ack))break;//得到有效数据 
				USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
}
//ATK-ESP8266退出透传模式
//返回值:0,退出成功;
//       1,退出失败
u8 atk_8266_quit_trans(void)
{
	while((USART2->SR&0X40)==0);	//等待发送空
	USART2->DR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART2->SR&0X40)==0);	//等待发送空
	USART2->DR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART2->SR&0X40)==0);	//等待发送空
	USART2->DR='+';      
	delay_ms(500);					//等待500ms
	return atk_8266_send_cmd((u8*)"AT",(u8*)"OK",20);//退出透传判断.
}
//获取ATK-ESP8266模块的AP+STA连接状态
//返回值:0，未连接;1,连接成功
u8 atk_8266_apsta_check(void)
{
	if(atk_8266_quit_trans())return 0;			//退出透传 
	atk_8266_send_cmd((u8*)"AT+CIPSTATUS",(u8*)":",50);	//发送AT+CIPSTATUS指令,查询连接状态
	if(atk_8266_check_cmd((u8*)"+CIPSTATUS:0") &&	atk_8266_check_cmd((u8*)"+CIPSTATUS:1") && atk_8266_check_cmd((u8*)"+CIPSTATUS:2") &&	atk_8266_check_cmd((u8*)"+CIPSTATUS:4"))
				return 0;
	else  return 1;
}
//获取ATK-ESP8266模块的连接状态
//返回值:0,未连接;1,连接成功.
u8 atk_8266_consta_check(void)
{
	u8 *p;
	u8 res;
	if(atk_8266_quit_trans())return 0;			        // 退出透传 
	atk_8266_send_cmd((u8*)"AT+CIPSTATUS",(u8*)":",50);	// 发送AT+CIPSTATUS指令,查询连接状态
	p=atk_8266_check_cmd((u8*)"+CIPSTATUS:");           // +CIPSTATUS:后为id号
	res=*p;									//得到连接状态	
	return res;
}


//获取Client ip地址
//ipbuf:ip地址输出缓存区
void atk_8266_get_wanip(u8* ipbuf)
{
	u8 *p,*p1;
	if(atk_8266_send_cmd((u8*)"AT+CIFSR",(u8*)"OK",50))//获取WAN IP地址失败
	{
		ipbuf[0]=0;
		return;
	}		
	p=atk_8266_check_cmd((u8*)"\"");
	p1=(u8*)strstr((const char*)(p+1),(const char*)"\"");
	*p1=0;
	sprintf((char*)ipbuf,"%s",p+1);	
}


//ATK-ESP8266模块初始化配置函数
void atk_8266_init(void)
{
	u8 timer = 0;
	u8 ipbuf[16]; 	                          // IP缓存
	u8 *p = mymalloc(32);							        // 申请32字节内存
	
	printf("ATK-ESP8266 WIFI模块\r\n");
	while(atk_8266_send_cmd((u8*)"AT",(u8*)"OK",20))    //检查WIFI模块是否在线
	{
		atk_8266_quit_trans();                       //退出透传
		atk_8266_send_cmd((u8*)"AT+CIPMODE=0",(u8*)"OK",200);  //关闭透传模式	
		printf("未检测到wifi模块!!!\r\n");
		delay_ms(1200);
		printf("尝试连接wifi模块...\r\n");
	} 
	printf("成功检测到wifi模块...\r\n");
	while(atk_8266_send_cmd((u8*)"ATE0",(u8*)"OK",20));      //关闭回显
	
	delay_ms(10); 
	atk_8266_at_response(1);//检查ATK-ESP8266模块发送过来的数据,及时上传给电脑

	printf("正在配置ATK-ESP8266模块，请稍等...\r\n");
	
	atk_8266_send_cmd((u8*)"AT+CWMODE=1",(u8*)"OK",50);		     // 设置WIFI STA模式
	atk_8266_send_cmd((u8*)"AT+RST",(u8*)"OK",20);		         // DHCP服务器关闭(仅AP模式有效) 
	delay_ms(1000);         //延时3S等待重启成功
	delay_ms(1000);
	delay_ms(1000);

	while(1)
	{
		if(Link_Flag == 1) break;  // 已建立过连接，可直接连入上次选择的wifi
		if(AP_Choose() == 0)       // 成功选择wifi
		{
			PWD_Index = 0;                 // 标记当前密码长度
			if(Enter_AP_PWD() == 0) break; // 成功输入密码
		}
	}
	
	//设置连接到的WIFI网络名称/加密方式/密码,这几个参数需要根据您自己的路由器设置进行修改!! 
	//printf("\r\n%s %s\r\n",wifista_ssid,wifista_password); //打印无线参数:ssid,密码
	printf("\r\n%s %s\r\n",wifista_ssid,PWD_Temp);        // 打印无线参数:ssid,密码

	sprintf((char*)p,"AT+CWJAP=\"%s\",\"%s\"",wifista_ssid,PWD_Temp);//设置无线参数:ssid,密码
	timer = 0;
	while(atk_8266_send_cmd(p,(u8*)"WIFI GOT IP",300))     //连接目标路由器,并且获得IP
	{
		printf("尝试连接目标wifi：%s，请确认该wifi已开启！！！\r\n", wifista_ssid);
		LCD_Clear(WHITE);	
		POINT_COLOR = BLUE;
		BACK_COLOR = WHITE;
		LCD_ShowString(25,120,200,24,24,(u8*)"   Connecting");
		if(++timer == 1)    LCD_ShowString(25,150,200,24,24,(u8*)" Please wait.");
		else if(timer == 2) LCD_ShowString(25,150,200,24,24,(u8*)" Please wait..");
		else if(timer == 3) 
		{
			timer = 0;
			LCD_ShowString(25,150,200,24,24,(u8*)" Please wait...");
		}
		delay_ms(300);
	}	
	LCD_Clear(WHITE);	
	POINT_COLOR = BLUE;
	BACK_COLOR = WHITE;
	LCD_ShowString(15,120,225,16,16,(u8*)"wifi Connection successful!");
	
	Link_Flag = 1;                        // 标记已建立过连接，可直接连入上次选择的wifi
	printf("成功连接目标wifi：%s\r\n", wifista_ssid);
	delay_ms(650);delay_ms(650);
	//TCP
//	IP_Temp[0] = 0;        // 密码输入缓存
//    IP_Index = 0;          // 标记当前IP长度
//	if(Enter_TCP_IP())   goto wifi_scan;        // 输入IP及端口失败
	
	printf("正在连接TCP服务器 %s:%s\r\n", (u8*)remote_ip, (u8*)portnum);
	// 成功输入TCP服务器IP
	atk_8266_send_cmd((u8*)"AT+CIPMUX=0",(u8*)"OK",20);   //0：单连接，1：多连接
	sprintf((char*)p,(const char*)"AT+CIPSTART=\"TCP\",\"%s\",%s",(u8*)remote_ip,(u8*)portnum);    //配置目标TCP服务器
	timer = 0;
	while(atk_8266_send_cmd(p,(u8*)"OK",300))
	{
		printf("TCP连接失败，请确认服务器是否打开\r\n");
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
	}	
	printf("TCP连接成功\r\n");
	LCD_Clear(WHITE);	
	LCD_ShowString(15,120,225,16,16,(u8*)"TCP Connection successful!");
	atk_8266_send_cmd((u8*)"AT+CIPMODE=1",(u8*)"OK",200);      // 传输模式为：透传		
		
		
	atk_8266_get_wanip(ipbuf);                       // 服务器模式,获取WAN IP
	sprintf((char*)p,"IP地址:%s 端口:%s",ipbuf,(u8*)portnum);
	printf("%s\r\n", p);

	//atk_8266_wificonf_show(30,180,"请设置路由器无线参数为:",(u8*)wifista_ssid,(u8*)wifista_encryption,(u8*)wifista_password);
	printf("工作模式：%s\r\n", (u8*)ATK_ESP8266_WORKMODE_TBL[1]);      // TCP客户端
	
	myfree(p);		//释放内存 
	USART2_RX_STA=0;
}

// ESP_8266扫描可用AP，并连入
// 返回值： 0--连接AP成功    other--失败，应答超时
u8 AP_Choose(void)
{
	volatile u8 waittime = 200;                   // waittime:应答等待时间   20ms
	volatile u8 AP_TAB[25][30] = {0,};            // wifi 热点名缓存
	volatile u8 Flag = 0;
	volatile u16 i = 0, m = 0, n = 0, len = 0, timer = 0;
	
	
	while(1)
	{	
		waittime = 200; Flag = 0; timer = 0;
		for(m = 0; m < 25; m++)
		{
			for(n = 0; n < 30; n++) AP_TAB[m][n] = 0;
		}
		i = 0; m = 0; n = 0; len = 0;
		USART2_RX_BUF[0] = 0;                // 串口缓存清零
		USART2_RX_STA = 0;
		Join_Scan = 1;
		
		u2_printf("AT+CWLAP=\r\n");	         // 发送命令  扫描可用AP    应答格式：+CWLAP:(4,"MadDog",-42,"e4:d3:32:d0:22:fe",1,31,0)\r\n...
		while(--waittime)	                 // 等待应答  倒计时
		{
			delay_ms(25);
			if(USART2_RX_STA&0X8000)         // 接收到应答
			{
				Join_Scan = 0;
				len = USART2_RX_STA&0X7FFF;
				USART2_RX_BUF[len] = 0;      // 添加字符串结束符
				break;
			} 
		}
		if(waittime == 0) 
		{
			printf("Ack Error...Retrying\r\n");
			continue;     // 应答超时
		}

		// 成功接收应答
	//	printf("%d len=%d\r\n", waittime, USART2_RX_STA&0X7FFF);
	//	printf("%s\r\n", USART2_RX_BUF);
		for(i = 0; i < len; i++)              // 将wifi 热点名缓存到AP_TAB
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
		LCD_Fill(0,48,239,319,0xEEEE);		   				     // 填充单色
		POINT_COLOR = BLACK;
		LCD_DrawLine(0, 48, 239, 48);		         // 画线

		for(i = 0; i < m; i++)
		{
			printf("AP_TAB[%d]:%s\r\n", i, AP_TAB[i]);
			LCD_ShowString(60,60+20*i,240,16,16,(u8*)AP_TAB[i]); 
			if(i != m-1) LCD_DrawLine(60, 78+20*i, 239, 78+20*i);		     //画线
		}
		LCD_DrawLine(0, 68+20*i, 239, 68+20*i);		         // 画线
		while(1)    // 等待用户触摸选择
		{
			tp_dev.scan(0); 		 
			if(tp_dev.sta&TP_PRES_DOWN)			// 触摸屏被按下
			{	
				if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
				{	
					if(tp_dev.y[0] > 60)        // AP显示区域内
					{
						int Index = (tp_dev.y[0] - 60) / 20;
						wifista_ssid = (u8*)AP_TAB[Index];
						printf("wifista_ssid:%s\r\n", wifista_ssid);
						delay_ms(300);
						return 0;
					}		   
				}
			}
			else 
			{
				delay_ms(10);	//没有按键按下的时候 	 
				if(++timer == 850) break;            // 10s内未选择网络，则刷新网络
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
	
	POINT_COLOR = GRAY;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
	LCD_Fill(203,25,239,50,0xEEEE);	
	LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
	
	
	POINT_COLOR = BLACK;
	BACK_COLOR = WHITE;
	LCD_DrawLine(0, 75, 239, 75);
	LCD_ShowString(20,80,200,16,16, (u8*)"PWD:"); 
	LCD_DrawLine(0, 101, 239, 101);
	
	// 加载数字键盘UI
	KbNum_UI(0);      // 0--密码输入 
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// 触摸屏被按下
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 25 && tp_dev.y[0] <= 50)
				{
					if(tp_dev.x[0] < 52)                // Cancel 
					{
						delay_ms(400);
						LCD_Clear(WHITE);
						LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
						return 1;
					}
					if(tp_dev.x[0] > 203)               // Join
					{
						if(PWD_Index >= 8)           // 输入密码长度 > 8
						{
//							wifista_password = (u8*)PWD_Temp;  // 获取到密码
							printf("wifista_password:%s\r\n", PWD_Temp);
							delay_ms(300);
							return 0;
						}
					}
				}
				if(tp_dev.y[0] > 120)        // 数字键盘显示区域内
				{
					Touch_Up = 0;
					i = (tp_dev.y[0] - 120) / 50;
					j = tp_dev.x[0] / 80;
					if(Kb_Num[i][j] >= '0' && Kb_Num[i][j] <= '9')    // 输入0~9
					{
						LCD_ShowChar(60+PWD_Index*10,80,Kb_Num[i][j],16,0);  // LCD显示
						PWD_Temp[PWD_Index++] = Kb_Num[i][j];
						PWD_Temp[PWD_Index] = 0;     // 添加字符串结束符
						printf("PWD_Temp:%s\r\n", PWD_Temp);
						
						if(PWD_Index >= 8)           // 输入密码长度 >= 8
						{
							POINT_COLOR = RED;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
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
						if(Kb_Num[i][j] == 'A')  // 切换到字母键盘
						{
							u8 res = Load_Kb_Char();
							if(res == 0)      return 0;      // 密码输入成功
							else if(res == 1) KbNum_UI(0);   // 加载数字键盘  0--密码输入 
							else              return 1;      // Cancel
						} 
						else                     // 删除前一个字符            
						{
							if(PWD_Index > 0) PWD_Index--;
							LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);  // LCD显示
							LCD_ShowChar(70+PWD_Index*10,80,' ',16,0);  // LCD显示
							PWD_Temp[PWD_Index] = 0;
							printf("PWD_Temp:%s\r\n", PWD_Temp);
							if(PWD_Index < 8)           // 输入密码长度 < 8
							{
								POINT_COLOR = GRAY;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
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
		else                              //没有按下	
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

// 加载字母键盘   
// 返回值：0--密码输入成功    1--返回数字键盘    2--取消输入密码，重新扫描wifi
u8 Load_Kb_Char(void)  
{
	volatile u8 Char_Shitf = 0;         // 大/小写字母标志
	volatile u8 i = 0, j = 0, Touch_Up = 1, timer = 0;

	delay_ms(500);
	KbChar_UI(Char_Shitf);
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// 触摸屏被按下
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 25 && tp_dev.y[0] <= 50)
				{
					if(tp_dev.x[0] < 52)                // Cancel 
					{
						delay_ms(400);
						LCD_Clear(WHITE);
						LCD_ShowString(25,150,200,24,24,(u8*)"wifi scanning..."); 
						return 2;
					}
					if(tp_dev.x[0] > 203)               // Join
					{
						if(PWD_Index >= 8)           // 输入密码长度 > 8
						{
//							wifista_password = (u8*)PWD_Temp;  // 获取到密码
							printf("wifista_password:%s\r\n", PWD_Temp);
							delay_ms(300);
							return 0;
						}
					}
				}
				if(tp_dev.y[0] > 120)        // 键盘显示区域内
				{
					Touch_Up = 0;
					
					if(tp_dev.y[0] < 280)    // 26个字母区
					{
						i = (tp_dev.y[0] - 120) / 40;
						j = tp_dev.x[0] / 34;
						printf("%c ", Kb_char[i][j]);
						if(Char_Shitf)    // 大写
						{
							if(Kb_Char[i][j] >= 'A' && Kb_Char[i][j] <= 'Z')    // 输入A~Z
							{
								LCD_ShowChar(60+PWD_Index*10,80,Kb_Char[i][j],16,0);  // LCD显示
								PWD_Temp[PWD_Index++] = Kb_Char[i][j];
								PWD_Temp[PWD_Index] = 0;     // 添加字符串结束符
								printf("PWD_Temp:%s\r\n", PWD_Temp);
								
								if(PWD_Index >= 8)           // 输入密码长度 >= 8
								{
									POINT_COLOR = RED;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
									BACK_COLOR = 0xEEEE;
									LCD_Fill(203,25,239,50,0xEEEE);	
									LCD_ShowString(205,30,35,16,16, (u8*)"Join"); 
									POINT_COLOR = BLACK;
									BACK_COLOR = WHITE;
								}
								delay_ms(300);
							}
						}
						else         // 小写模式
						{
							if(Kb_char[i][j] >= 'a' && Kb_char[i][j] <= 'z')    // 输入a~z
							{													
								LCD_ShowChar(60+PWD_Index*10,80,Kb_char[i][j],16,0);  // LCD显示
								PWD_Temp[PWD_Index++] = Kb_char[i][j];
								PWD_Temp[PWD_Index] = 0;     // 添加字符串结束符
								printf("PWD_Temp:%s\r\n", PWD_Temp);
								
								if(PWD_Index >= 8)           // 输入密码长度 >= 8
								{
									POINT_COLOR = RED;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
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
						
						if(Kb_char[i][j] == '1')       // 切换到数字键盘
						{
							delay_ms(800);
							return 1;         
						} 
						else if(Kb_char[i][j] == '+')  // 切换大写字母键盘
						{
							if(Char_Shitf == 0) Char_Shitf = 1;   
							else                Char_Shitf = 0; 
							delay_ms(700);
							KbChar_UI(Char_Shitf);
						} 
						else if(Kb_char[i][j] == '2')  // 删除前一个字符            
						{
							if(PWD_Index > 0) PWD_Index--;
							LCD_ShowString(60+PWD_Index*10,80,200,16,16, (u8*)"                   "); 
//							LCD_ShowChar(60+PWD_Index*10,80,' ',16,0);  // LCD显示
//							LCD_ShowChar(70+PWD_Index*10,80,' ',16,0);  // LCD显示
							PWD_Temp[PWD_Index] = 0;
							printf("PWD_Temp:%s\r\n", PWD_Temp);
							if(PWD_Index < 8)           // 输入密码长度 < 8
							{
								POINT_COLOR = GRAY;                             // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
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
		else                              //没有按下	
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

// 数字键盘UI 
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
	for(i = 0; i < 3; i++) LCD_DrawLine(0, 153+50*i, 239, 153+50*i);     // 画键盘横线
	for(i = 0; i < 2; i++) LCD_DrawLine(80+80*i, 101, 80+80*i, 319);     // 画键盘竖线
	BACK_COLOR = WHITE;
	return 0;
}

// 字母键盘UI  shift：0--小写  else--大写
u8 KbChar_UI(u8 shift)
{
	volatile u8 i = 0, j = 0;
	
	// 加载字母键盘
	LCD_Fill(0,102,239,319,0xEEEE);	 // 清除键盘区域
	BACK_COLOR = 0xEEEE;
	POINT_COLOR = BLACK;
	if(shift)             // 大写字母
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
					if(Kb_char[i][j] == '1') LCD_ShowString(28,     120+40*i,200,16,16, (u8*)"123");    // char/num切换
					if(Kb_char[i][j] == '+') LCD_ShowString(20+80*1,120+40*i,200,16,16, (u8*)"shift");  // 大小写切换
					if(Kb_char[i][j] == '2') LCD_ShowString(28+80*2,120+40*i,200,16,16, (u8*)"del");    // 删除
				}
			}
		}
	}
	else                   // 小写字母
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
					if(Kb_char[i][j] == '1') LCD_ShowString(28,     120+40*i,200,16,16, (u8*)"123");    // char/num切换
					if(Kb_char[i][j] == '+') LCD_ShowString(20+80*1,120+40*i,200,16,16, (u8*)"Shift");  // 大小写切换
					if(Kb_char[i][j] == '2') LCD_ShowString(28+80*2,120+40*i,200,16,16, (u8*)"del");    // 删除
				}
			}
		}
	}
	BACK_COLOR = WHITE;
	return 0;
}

// 输入wifi密码，加载数字键盘
// 返回值： 0--输入IP成功    1--Cancel 重新扫描wifi
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
	
	POINT_COLOR = GRAY;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
	LCD_Fill(203,10,239,35,0xEEEE);	
	LCD_ShowString(205,15,35,16,16, (u8*)"Done"); 
	
	
	POINT_COLOR = BLACK;
	BACK_COLOR = WHITE;
	LCD_DrawLine(0, 49, 239, 49);
	LCD_ShowString(12,54,200,16,16, (u8*)"  IP:"); 
	
	LCD_DrawLine(0, 75, 239, 75);
	LCD_ShowString(12,80,200,16,16, (u8*)"Port:"); 
	LCD_DrawLine(0, 101, 239, 101);
	
	// 加载数字键盘UI
	KbNum_UI(1);      // 1--TCP_IP输入
	BACK_COLOR = WHITE;
	
	while(1) 
	{
		tp_dev.scan(0); 		 
		if( (tp_dev.sta&TP_PRES_DOWN) && Touch_Up )			// 触摸屏被按下
		{	
			if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.y[0] >= 49 && tp_dev.y[0] <= 75)      my_tcp_sta = Enter_IPAddr;     // 触摸到IP输入区域
				else if(tp_dev.y[0] > 75 && tp_dev.y[0] <= 101) my_tcp_sta = Enter_IPCom;      // 触摸到Com输入区域
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
							if(Pnum_Index >= 1 && Pnum_Index <= 5)      // 端口号长度为 1~5
							{
								printf("IP:%s Port:%s\r\n", IP_Temp, Pnum_Temp);
								delay_ms(300);
								return 0;
							}
						}
					}
				}
				if(tp_dev.y[0] > 120)        // 数字键盘显示区域内
				{
					Touch_Up = 0;
					i = (tp_dev.y[0] - 120) / 50;
					j = tp_dev.x[0] / 80;
					if(Kb_Num[i][j] >= '0' && Kb_Num[i][j] <= '9')    // 输入0~9
					{
						LCD_ShowChar(60+IP_Index*10,80,Kb_Num[i][j],16,0);  // LCD显示
						IP_Temp[IP_Index++] = Kb_Num[i][j];
						IP_Temp[IP_Index] = 0;     // 添加字符串结束符
						printf("IP_Temp:%s\r\n", IP_Temp);
						
						if(dot_timer >= 3 && IP_Temp[IP_Index-1] != '.')           // 输入密码长度 >= 15
						{
							POINT_COLOR = RED;                               // 当输入密码长度 > 8 时， 由GRAY -> RED, 触摸有效
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
							LCD_ShowChar(60+IP_Index*10,80,'.',16,0);  // LCD显示
							IP_Temp[IP_Index++] = '.';
							IP_Temp[IP_Index] = 0;     // 添加字符串结束符
							printf("PWD_Temp:%s dot_timer = %d IP_Temp[IP_Index-1] = %d\r\n", IP_Temp, dot_timer, IP_Temp[IP_Index-1]);
							
							delay_ms(500);
						} 
						else                     // 删除前一个字符            
						{
							if(IP_Index > 0) 
							{
								IP_Index--;
								if(IP_Temp[IP_Index-1] == '.') 
								{
									if(dot_timer>0) dot_timer--;
								}
							}
							LCD_ShowChar(60+IP_Index*10,80,' ',16,0);  // LCD显示
							LCD_ShowChar(70+IP_Index*10,80,' ',16,0);  // LCD显示
							IP_Temp[IP_Index] = 0;
							printf("PWD_Temp:%s\r\n", IP_Temp);
							if( (dot_timer < 3) || ((dot_timer == 3)&&(IP_Temp[IP_Index-1] == '.')) )          // '.'个数 < 3
							{
								POINT_COLOR = GRAY;                                  // 当输入密码长度 > 15 时， 由GRAY -> RED, 触摸有效
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
		else                              //没有按下	
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








































