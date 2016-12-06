#include "common.h"
#include "stdlib.h"
#include "stdio.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//用户配置区

//连接端口号:8086,可自行修改为其他端口.
const u8* remote_ip = (u8*)"192.168.191.1";	
//const u8* remote_ip = "222.212.204.66";	

const u8* portnum   = (u8*)"8086";	 

//WIFI STA模式,设置要去连接的路由器无线参数,请根据你自己的路由器设置,自行修改.
const u8* wifista_ssid=(u8*)"Growl";			//路由器SSID号
const u8* wifista_encryption=(u8*)"wpawpa2_aes";	//wpa/wpa2 aes加密方式
const u8* wifista_password=(u8*)"147258369"; 	//连接密码

//WIFI AP模式,模块对外的无线参数,可自行修改.
const u8* wifiap_ssid=(u8*)"ATK-ESP8266";			//对外SSID号
const u8* wifiap_encryption=(u8*)"wpawpa2_aes";	//wpa/wpa2 aes加密方式
const u8* wifiap_password=(u8*)"12345678"; 		//连接密码 

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//4个网络模式
const u8 *ATK_ESP8266_CWMODE_TBL[3]={(u8*)"STA模式 ",(u8*)"AP模式 ",(u8*)"AP&STA模式 "};	//ATK-ESP8266,3种网络模式,默认为路由器(ROUTER)模式 
//4种工作模式
const u8 *ATK_ESP8266_WORKMODE_TBL[3]={(u8*)"TCP服务器",(u8*)"TCP客户端",(u8*)"UDP模式"};	//ATK-ESP8266,4种工作模式
//5种加密方式
const u8 *ATK_ESP8266_ECN_TBL[5]={(u8*)"OPEN",(u8*)"WEP",(u8*)"WPA_PSK",(u8*)"WPA2_PSK",(u8*)"WPA_WAP2_PSK"};
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

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
	u2_printf("%s\r\n",cmd);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
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
	//设置连接到的WIFI网络名称/加密方式/密码,这几个参数需要根据您自己的路由器设置进行修改!! 
	sprintf((char*)p,"AT+CWJAP=\"%s\",\"%s\"",wifista_ssid,wifista_password);//设置无线参数:ssid,密码
	while(atk_8266_send_cmd(p,(u8*)"WIFI GOT IP",300))    //连接目标路由器,并且获得IP
	{
		printf("尝试连接目标wifi：%s，请确认该wifi已开启！！！\r\n", wifista_ssid);
		delay_ms(300);
	}		
	printf("成功连接目标wifi：%s\r\n", wifista_ssid);
	delay_ms(650);delay_ms(650);
	//TCP
	atk_8266_send_cmd((u8*)"AT+CIPMUX=0",(u8*)"OK",20);   //0：单连接，1：多连接
	sprintf((char*)p,(const char*)"AT+CIPSTART=\"TCP\",\"%s\",%s",remote_ip,(u8*)portnum);    //配置目标TCP服务器
	while(atk_8266_send_cmd(p,(u8*)"OK",300))
	{
		printf("TCP连接失败，请确认服务器是否打开\r\n");
		delay_ms(600);
	}	
	printf("TCP连接成功\r\n");
	atk_8266_send_cmd((u8*)"AT+CIPMODE=1",(u8*)"OK",200);      // 传输模式为：透传		
		
		
	atk_8266_get_wanip(ipbuf);                       // 服务器模式,获取WAN IP
	sprintf((char*)p,"IP地址:%s 端口:%s",ipbuf,(u8*)portnum);
	printf("%s\r\n", p);

	//atk_8266_wificonf_show(30,180,"请设置路由器无线参数为:",(u8*)wifista_ssid,(u8*)wifista_encryption,(u8*)wifista_password);
	printf("工作模式：%s\r\n", (u8*)ATK_ESP8266_WORKMODE_TBL[1]);      // TCP客户端
	
	myfree(p);		//释放内存 
	USART2_RX_STA=0;
}

















































