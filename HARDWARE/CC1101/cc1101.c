#include "cc1101.h"
#include "stdio.h"
#include "spi.h"
#include "delay.h"

// IDLE状态：不发送、不接收时的缺省状态，电流：1.9mA
// Sleep状态：电流不足1mA
// 发送：14mA@-10dBm  16mA@0dBm 19mA@+5dBm 29mA@+10dBm
// 接收：14.2mA@500kbps  15.4mA@2.4kbps    RSSI能连续读取

                     //10,    7,    5,    0,   -10,  -15, -20, -30dbm
uint8_t PaTabel[] = {0xc0, 0xC8, 0x84, 0x60, 0x34, 0x1D, 0x0E, 0x12};   // 433MHz

// Sync word qualifier mode = 30/32 sync word bits detected 
// CRC autoflush = false 
// Channel spacing = 199.951172 
// Data format = Normal mode 
// Data rate = 2.00224 
// RX filter BW = 58.035714 
// PA ramping = false 
// Preamble count = 4 
// Whitening = false 
// Address config = No address check 
// Carrier frequency = 400.199890 
// Device address = 0 
// TX power = 10 
// Manchester enable = false 
// CRC enable = true 
// Deviation = 5.157471 
// Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word 
// Packet length = 255 
// Modulation format = GFSK 
// Base frequency = 399.999939 
// Modulated = true 
// Channel number = 1 
// PA table 
#define PA_TABLE {0xc2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}

static const uint8_t CC1101InitData[22][2]= 
{
  {CC1101_IOCFG0,   0x06},
  {CC1101_FIFOTHR,  0x47},
  {CC1101_PKTCTRL0, 0x05},
  {CC1101_CHANNR,   0x01},
  {CC1101_FSCTRL1,  0x06},
  {CC1101_FREQ2,    0x0F},
  {CC1101_FREQ1,    0x62},
  {CC1101_FREQ0,    0x76},
  {CC1101_MDMCFG4,  0xF6},
  {CC1101_MDMCFG3,  0x43},
  {CC1101_MDMCFG2,  0x13},
  {CC1101_DEVIATN,  0x15},
  {CC1101_MCSM0,    0x18},
  {CC1101_FOCCFG,   0x16},
  {CC1101_WORCTRL,  0xFB},
  {CC1101_FSCAL3,   0xE9},
  {CC1101_FSCAL2,   0x2A},
  {CC1101_FSCAL1,   0x00},
  {CC1101_FSCAL0,   0x1F},
  {CC1101_TEST2,    0x81},
  {CC1101_TEST1,    0x35},
  {CC1101_MCSM1,    0x3B},

//  {CC1101_IOCFG0,      0x06},
//  {CC1101_FIFOTHR,     0x47},
//  {CC1101_PKTCTRL0,    0x05},
//  {CC1101_CHANNR,      0x00},
//  {CC1101_FSCTRL1,     0x08},
//  {CC1101_FREQ2,       0x10},
//  {CC1101_FREQ1,       0xA7},
//  {CC1101_FREQ0,       0x62},
//  {CC1101_MDMCFG4,     0x5B},
//  {CC1101_MDMCFG3,     0xF8},
//  {CC1101_MDMCFG2,     0x03},
//  {CC1101_DEVIATN,     0x47},
//  {CC1101_MCSM0,       0x18},
//  {CC1101_FOCCFG,      0x1D},
//  {CC1101_WORCTRL,     0xFB},
//  {CC1101_FSCAL3,      0xEA},
//  {CC1101_FSCAL2,      0x2A},
//  {CC1101_FSCAL1,      0x00},
//  {CC1101_FSCAL0,      0x11},
//  {CC1101_TEST2,       0x81},
//  {CC1101_TEST1,       0x35},
//  {CC1101_MCSM1,       0x3B},
};

uint8_t CC1101ReadReg(uint8_t addr);                                  // read a byte from the specified register
void CC1101ReadMultiReg(uint8_t addr, uint8_t *buff, uint8_t size);   // Read some bytes from the rigisters continously
void CC1101WriteReg(uint8_t addr, uint8_t value);                     // Write a byte to the specified register
void CC1101ClrTXBuff(void);                                           // Flush the TX buffer of CC1101
void CC1101ClrRXBuff(void);                                           // Flush the RX buffer of CC1101
uint8_t CC1101GetRXCnt(void);                                         // Get received count of CC1101
void CC1101Reset(void);                                               // Reset the CC1101 device
void CC1101WriteMultiReg(uint8_t addr, uint8_t *buff, uint8_t size);  // Write some bytes to the specified register

extern int RecvWaitTime;                    // 接收等待超时时间
/*
================================================================================
Function : CC1101WORInit()
    Initialize the WOR function of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101WORInit(void)
{
	CC1101WriteReg(CC1101_MCSM0,  0x18);
	CC1101WriteReg(CC1101_WORCTRL,0x78); //Wake On Radio Control
	CC1101WriteReg(CC1101_MCSM2,  0x00);
	CC1101WriteReg(CC1101_WOREVT1,0x8C);
	CC1101WriteReg(CC1101_WOREVT0,0xA0);

	CC1101WriteCmd(CC1101_SWORRST);
}
/*
================================================================================
Function : CC1101ReadReg()
    read a byte from the specified register
INPUT    : addr, The address of the register
OUTPUT   : the byte read from the rigister
================================================================================
*/
uint8_t CC1101ReadReg(uint8_t addr)
{
	uint8_t i;
	
	CC_CSN_LOW();
	SPI_ExchangeByte(addr | READ_SINGLE);
	i = SPI_ExchangeByte(0xFF);
	CC_CSN_HIGH();
	
	return i;
}
/*
================================================================================
Function : CC1101ReadMultiReg()
    Read some bytes from the rigisters continously
INPUT    : addr, The address of the register
           buff, The buffer stores the data
           size, How many bytes should be read
OUTPUT   : None
================================================================================
*/
void CC1101ReadMultiReg(uint8_t addr, uint8_t *buff, uint8_t size)
{
	uint8_t i, j;
	
	CC_CSN_LOW();
	SPI_ExchangeByte(addr | READ_BURST);
	for(i = 0; i < size; i ++)
	{
		for(j = 0; j < 20; j ++);
		*(buff + i)= SPI_ExchangeByte(0xFF);
	}
	CC_CSN_HIGH();
}
/*
================================================================================
Function : CC1101ReadStatus()
    Read a status register
INPUT    : addr, The address of the register
OUTPUT   : the value read from the status register
================================================================================
*/
uint8_t CC1101ReadStatus(uint8_t addr)
{
	uint8_t i;
	
	CC_CSN_LOW();
	SPI_ExchangeByte(addr | READ_BURST);
	i = SPI_ExchangeByte(0xFF);
	
	//printf("\r\n%d  ", (int)i);
	CC_CSN_HIGH();
	
	return i;
}
/*
================================================================================
Function : CC1101SetTRMode()
    Set the device as TX mode or RX mode
INPUT    : mode selection
OUTPUT   : None
================================================================================
*/
void CC1101SetTRMode(TRMODE mode)
{
	if(mode == TX_MODE)
	{
		CC1101WriteReg(CC1101_IOCFG0,0x46);
		CC1101WriteCmd(CC1101_STX);
	}
	else if(mode == RX_MODE)
	{
		CC1101WriteReg(CC1101_IOCFG0,0x46);
		CC1101WriteCmd(CC1101_SRX);
	}
}
/*
================================================================================
Function : CC1101WriteReg()
    Write a byte to the specified register
INPUT    : addr, The address of the register
           value, the byte you want to write
OUTPUT   : None
================================================================================
*/
void CC1101WriteReg(uint8_t addr, uint8_t value)
{
	CC_CSN_LOW();
	SPI_ExchangeByte(addr);
	SPI_ExchangeByte(value);
	CC_CSN_HIGH();
}
/*
================================================================================
Function : CC1101WriteMultiReg()
    Write some bytes to the specified register
INPUT    : addr, The address of the register
           buff, a buffer stores the values
           size, How many byte should be written
OUTPUT   : None
================================================================================
*/
void CC1101WriteMultiReg(uint8_t addr, uint8_t *buff, uint8_t size)
{
	uint8_t i;
	
	CC_CSN_LOW();
	SPI_ExchangeByte(addr | WRITE_BURST);
	for(i = 0; i < size; i ++)
	{
		SPI_ExchangeByte(*(buff + i));
	}
	CC_CSN_HIGH();
}
/*
================================================================================
Function : CC1101WriteCmd()
    Write a command byte to the device
INPUT    : command, the byte you want to write
OUTPUT   : None
================================================================================
*/
void CC1101WriteCmd(uint8_t command)
{
	CC_CSN_LOW();
	SPI_ExchangeByte(command);
	CC_CSN_HIGH();
}
/*
================================================================================
Function : CC1101Reset()
    Reset the CC1101 device
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101Reset(void)
{
	CC_CSN_HIGH();
	CC_CSN_LOW();
	CC_CSN_HIGH();
	delay_us(150);                  // 至少40us
	CC1101WriteCmd(CC1101_SRES);
}
/*
================================================================================
Function : CC1101SetIdle()
    Set the CC1101 into IDLE mode
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101SetIdle(void)
{
   CC1101WriteCmd(CC1101_SIDLE);
}
/*
================================================================================
Function : CC1101ClrTXBuff()
    Flush the TX buffer of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101ClrTXBuff(void)
{
	CC1101SetIdle();//MUST BE IDLE MODE
	CC1101WriteCmd(CC1101_SFTX);
}
/*
================================================================================
Function : CC1101ClrRXBuff()
    Flush the RX buffer of CC1101
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101ClrRXBuff(void)
{
	CC1101SetIdle();//MUST BE IDLE MODE
	CC1101WriteCmd(CC1101_SFRX);
}
/*
================================================================================
Function : CC1101SendPacket()
    Send a packet
INPUT    : txbuffer, The buffer stores data to be sent
           size, How many bytes should be sent
           mode, Broadcast or address check packet
OUTPUT   : None
================================================================================
*/
void CC1101SendPacket(uint8_t *txbuffer, uint8_t size, TX_DATA_MODE mode)
{
	uint8_t address;
	
	if(mode == BROADCAST)          address = 0;
	else if(mode == ADDRESS_CHECK) address = CC1101ReadReg(CC1101_ADDR);  // 本机地址

	printf("local_address:%d\r\n", (int)address);
	CC1101ClrTXBuff();
	
	if((CC1101ReadReg(CC1101_PKTCTRL1)& ~0x03)!= 0)
	{
#if (WORK_MODE == TX)    
		address = RX_Address;
#else 
		address = TX_Address;
#endif
		
		CC1101WriteReg(CC1101_TXFIFO, size + 1);
		CC1101WriteReg(CC1101_TXFIFO, address);
	}
	else
	{
		CC1101WriteReg(CC1101_TXFIFO, size);
	}

	CC1101WriteMultiReg(CC1101_TXFIFO, txbuffer, size);
	CC1101SetTRMode(TX_MODE);
	while(CC_IRQ_READ()!= 0);
	while(CC_IRQ_READ()== 0);

	CC1101ClrTXBuff();
}
/*
================================================================================
Function : CC1101GetRXCnt()
    Get received count of CC1101
INPUT    : None
OUTPUT   : How many bytes hae been received
================================================================================
*/
uint8_t CC1101GetRXCnt(void)
{
   return (CC1101ReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO);
}
/*
================================================================================
Function : CC1101SetAddress()
    Set the address and address mode of the CC1101
INPUT    : address, The address byte
           AddressMode, the address check mode
OUTPUT   : None
================================================================================
*/
void CC1101SetAddress(uint8_t address, ADDR_MODE AddressMode)
{
	uint8_t btmp = (CC1101ReadReg(CC1101_PKTCTRL1)) & (~0x03);
	
	CC1101WriteReg(CC1101_ADDR, address);
	if     (AddressMode == BROAD_ALL)     ;
	else if(AddressMode == BROAD_NO)      btmp |= 0x01;
	else if(AddressMode == BROAD_0)       btmp |= 0x02;
	else if(AddressMode == BROAD_0AND255) btmp |= 0x03;   
}
/*
================================================================================
Function : CC1101SetSYNC()
    Set the SYNC bytes of the CC1101
INPUT    : sync, 16bit sync 
OUTPUT   : None
================================================================================
*/
void CC1101SetSYNC(uint16_t sync)
{
	CC1101WriteReg(CC1101_SYNC1, (0xFF & (sync>>8)));
	CC1101WriteReg(CC1101_SYNC0, (0xFF & sync)); 
}
/*
================================================================================
Function : CC1101RecPacket()
    Receive a packet
INPUT    : rxBuffer, A buffer store the received data
OUTPUT   : 1:received count, 0:no data
================================================================================
*/
uint8_t CC1101RecPacket(uint8_t *rxBuffer)
{
	uint8_t status[2], pktLen;
	uint16_t x = 0;

	if(CC1101GetRXCnt()!= 0)
	{
		pktLen = CC1101ReadReg(CC1101_RXFIFO);           // Read length byte
		if((CC1101ReadReg(CC1101_PKTCTRL1) & ~0x03)!= 0)
		{
			x = CC1101ReadReg(CC1101_RXFIFO);
		}
		if(pktLen == 0) return 0;
		else            pktLen --;
		CC1101ReadMultiReg(CC1101_RXFIFO, rxBuffer, pktLen); // Pull data
		CC1101ReadMultiReg(CC1101_RXFIFO, status, 2);        // Read  status bytes

		CC1101ClrRXBuff();

		if(status[1] & CRC_OK ) return pktLen; 
		else                    return 0; 
	}
	else return 0;                               // Error
}
/*
================================================================================
Function : CC1101Init()
    Initialize the CC1101, User can modify it
INPUT    : None
OUTPUT   : None
================================================================================
*/
void CC1101Init(void)
{
	volatile uint8_t i;//, j;
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    //使能PORTA时钟
	
	/*Configure GPIO pins : PA8 CC_IRQ */
	GPIO_InitStructure.GPIO_Pin = PIN_CC_IRQ;	    
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PORT_CC_IRQ, &GPIO_InitStructure); 	
	
	/*Configure GPIO pins : PA4 CSN*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/*Configure GPIO pin Output Level */
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	delay_ms(150);
	
	CC1101Reset();    
	
	for(i = 0; i < 22; i++)
	{
		CC1101WriteReg(CC1101InitData[i][0], CC1101InitData[i][1]);
	}
// 测试CC1101 SPI通信是否正常
//	for(i = 0; i < 22; i++)
//	{
//		j = CC1101ReadReg(CC1101InitData[i][0]);
//		printf(" %d  ", (int)j);
//	}
	
#if (WORK_MODE == TX)	
	CC1101SetAddress(TX_Address, BROAD_0AND255);
#else 
	CC1101SetAddress(RX_Address, BROAD_0AND255);
#endif
	
	CC1101SetSYNC(0x8799);
	CC1101WriteReg(CC1101_MDMCFG1, 0x72); //Modem Configuration

	CC1101WriteMultiReg(CC1101_PATABLE, PaTabel+1, 1);  // 功率
//	CC1101WriteMultiReg(CC1101_PATABLE, PaTabel, 8);

	i = CC1101ReadStatus(CC1101_PARTNUM);//for test, must be 0x80
	printf("\r\n%d  ", (int)i);
	
	i = CC1101ReadStatus(CC1101_VERSION);//for test, refer to the datasheet
	printf("%d\r\n", (int)i);
}

// 获取RSSI值
uint8_t Get_1101RSSI(void)
{
	return (CC1101ReadStatus(CC1101_RSSI));
}


uint8_t SPI_ExchangeByte(uint8_t Data)
{
	uint8_t Re_Data = 0;
	
	//while(HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)(&Data), (uint8_t *)(&Re_Data), 1, 1000) != HAL_OK) HAL_Delay(10);
	//HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)(&Data), (uint8_t *)(&Re_Data), 1, 1000);
	Re_Data = SPI1_ReadWriteByte(Data);
	
	return Re_Data;
}

// 定时器3中断服务程序		    
void TIM3_IRQHandler(void)
{ 	
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)//是更新中断
	{	 			   
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx更新中断标志 
		if(RecvWaitTime > 0) RecvWaitTime--;
	}	    
}
// 设置TIM3的开关
// sta:0，关闭; 1,开启;
void TIM3_Set(u8 sta)
{
	if(sta)
	{  
		TIM_SetCounter(TIM3,0);     // 计数器清空
		TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);  // 使能指定的TIM3中断,允许更新中断
		TIM_Cmd(TIM3, ENABLE);      // 使能TIM3	
	}
	else 
	{
		TIM_Cmd(TIM3, DISABLE);  // 关闭定时器3	   
		TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);                // 关闭指定的TIM3中断,关闭更新中断
	}
}
//通用定时器中断初始化
//这里始终选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数		 
void TIM3_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);     // TIM3时钟使能    
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr;                  // 设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler = psc;               // 设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);          // 根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);                // 关闭TIM3更新中断

	 	  
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		   // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			   // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	  // 根据指定的参数初始化VIC寄存器
	
	TIM3_Set(0);                      // 关闭TIM3
}
