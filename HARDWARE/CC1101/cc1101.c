#include "cc1101.h"
#include "stdio.h"
#include "spi.h"
#include "delay.h"
#include "sys.h"

// IDLE״̬�������͡�������ʱ��ȱʡ״̬��������1.9mA
// Sleep״̬����������1mA
// ���ͣ�14mA@-10dBm  16mA@0dBm 19mA@+5dBm 29mA@+10dBm
// ���գ�14.2mA@500kbps  15.4mA@2.4kbps    RSSI��������ȡ

//                     //10,    7,    5,    0,   -10,  -15, -20, -30dbm
//uint8_t PaTabel[] = {0xc0, 0xC8, 0x84, 0x60, 0x34, 0x1D, 0x0E, 0x12};   // 433M  

                   
u8 PaTabel[8] = {0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0};     // 915MHz   10dBm

// RF = 915MHz
// RF_SETTINGS is a data structure which contains all relevant CCxxx0 registers
typedef struct S_RF_SETTINGS
{
    u8 FSCTRL2;	  // �Լ��ӵ�
    u8 FSCTRL1;   // Frequency synthesizer control.
    u8 FSCTRL0;   // Frequency synthesizer control.
    u8 FREQ2;     // Frequency control word, high u8.
    u8 FREQ1;     // Frequency control word, middle u8.
    u8 FREQ0;     // Frequency control word, low u8.
    u8 MDMCFG4;   // Modem configuration.
    u8 MDMCFG3;   // Modem configuration.
    u8 MDMCFG2;   // Modem configuration.
    u8 MDMCFG1;   // Modem configuration.
    u8 MDMCFG0;   // Modem configuration.
    u8 CHANNR;    // Channel number.
    u8 DEVIATN;   // Modem deviation setting (when FSK modulation is enabled).
    u8 FREND1;    // Front end RX configuration.
    u8 FREND0;    // Front end RX configuration.
    u8 MCSM0;     // Main Radio Control State Machine configuration.
    u8 FOCCFG;    // Frequency Offset Compensation Configuration.
    u8 BSCFG;     // Bit synchronization Configuration.
    u8 AGCCTRL2;  // AGC control.
    u8 AGCCTRL1;  // AGC control.
    u8 AGCCTRL0;  // AGC control.
    u8 FSCAL3;    // Frequency synthesizer calibration.
    u8 FSCAL2;    // Frequency synthesizer calibration.
    u8 FSCAL1;    // Frequency synthesizer calibration.
    u8 FSCAL0;    // Frequency synthesizer calibration.
    u8 FSTEST;    // Frequency synthesizer calibration control
    u8 TEST2;     // Various test settings.
    u8 TEST1;     // Various test settings.
    u8 TEST0;     // Various test settings.
    u8 IOCFG2;    // GDO2 output pin configuration
    u8 IOCFG0;    // GDO0 output pin configuration
    u8 PKTCTRL1;  // Packet automation control.
    u8 PKTCTRL0;  // Packet automation control.
    u8 ADDR;      // Device address.
    u8 PKTLEN;    // Packet length.
} RF_SETTINGS;

/////////////////////////////////////////////////////////////////
const RF_SETTINGS rfSettings = 
{
    0x00,
    0x08,   // FSCTRL1   Frequency synthesizer control.
    0x00,   // FSCTRL0   Frequency synthesizer control.
    0x23,   // FREQ2     Frequency control word, high byte.
    0x31,   // FREQ1     Frequency control word, middle byte.
    0x3B,   // FREQ0     Frequency control word, low byte.
    0x5B,   // MDMCFG4   Modem configuration.
    0xF8,   // MDMCFG3   Modem configuration.  ??????100K
    0x03,   // MDMCFG2   Modem configuration.
    0x22,   // MDMCFG1   Modem configuration.
    0xF8,   // MDMCFG0   Modem configuration.

    0x00,   // CHANNR    Channel number.
    0x47,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
    0xB6,   // FREND1    Front end RX configuration.
    0x10,   // FREND0    Front end RX configuration.
    0x18,   // MCSM0     Main Radio Control State Machine configuration.
    0x1D,   // FOCCFG    Frequency Offset Compensation Configuration.
    0x1C,   // BSCFG     Bit synchronization Configuration.
    0xC7,   // AGCCTRL2  AGC control.
    0x00,   // AGCCTRL1  AGC control.
    0xB2,   // AGCCTRL0  AGC control.

    0xEA,   // FSCAL3    Frequency synthesizer calibration.
    0x2A,   // FSCAL2    Frequency synthesizer calibration.
    0x00,   // FSCAL1    Frequency synthesizer calibration.
    0x11,   // FSCAL0    Frequency synthesizer calibration.
    0x59,   // FSTEST    Frequency synthesizer calibration.
    0x81,   // TEST2     Various test settings.
    0x35,   // TEST1     Various test settings.
    0x09,   // TEST0     Various test settings.
    0x0B,   // IOCFG2    GDO2 output pin configuration.
    0x06,   // IOCFG0D   GDO0 output pin configuration. Refer to SmartRF?Studio User Manual for detailed pseudo register explanation.

    0x04,   // PKTCTRL1  Packet automation control.
    0x45,   // PKTCTRL0  Packet automation control.
    0x00,   // ADDR      Device address.
    0x0c    // PKTLEN    Packet length.
};


uint8_t CC1101ReadReg(uint8_t addr);                                  // read a byte from the specified register
void CC1101ReadMultiReg(uint8_t addr, uint8_t *buff, uint8_t size);   // Read some bytes from the rigisters continously
void CC1101WriteReg(uint8_t addr, uint8_t value);                     // Write a byte to the specified register
void CC1101ClrTXBuff(void);                                           // Flush the TX buffer of CC1101
void CC1101ClrRXBuff(void);                                           // Flush the RX buffer of CC1101
uint8_t CC1101GetRXCnt(void);                                         // Get received count of CC1101
void CC1101Reset(void);                                               // Reset the CC1101 device
void CC1101WriteMultiReg(uint8_t addr, uint8_t *buff, uint8_t size);  // Write some bytes to the specified register

extern volatile int RecvWaitTime;                    // ���յȴ���ʱʱ��
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
	delay_us(80);                  // ����40us
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
	else if(mode == ADDRESS_CHECK) address = CC1101ReadReg(CC1101_ADDR);  // ������ַ

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
	uint16_t temp = 0;

	if(CC1101GetRXCnt()!= 0)        // ���յ�����
	{
		pktLen = CC1101ReadReg(CC1101_RXFIFO) & 0xff;           // Read length byte
		if((CC1101ReadReg(CC1101_PKTCTRL1) & ~0x03)!= 0)
		{
			temp = CC1101ReadReg(CC1101_RXFIFO);
		}
		if(pktLen <= 0 || pktLen > 10) return 0;
		else                           pktLen--;
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
	volatile uint8_t i, j;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);      //�ⲿ�ж�,��Ҫʹ��AFIOʱ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);    //ʹ��PORTCʱ��
	
	/*Configure GPIO pins : PC4 CC_IRQ */
	GPIO_InitStructure.GPIO_Pin = PIN_CC_IRQ;	    
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PORT_CC_IRQ, &GPIO_InitStructure); 	
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource4);
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;		  
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	// ��ռ���ȼ�2
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;			// 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					// ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure); 

	EXTI4_Set(0);             // ����EXTI4�ж�
	
	/*Configure GPIO pins : PA4 CSN*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/*Configure GPIO pin Output Level */
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	delay_ms(5);
	
	CC1101Reset();    
	
	CC1101_Settings();
    printf("%d ", (int)CC1101ReadReg(CC1101_FSCTRL1));   // ����1�ֽڣ��ж�SPIͨѶ�Ƿ�����
	
#if (WORK_MODE == TX)	
	CC1101SetAddress(TX_Address, BROAD_0AND255);
#else 
	CC1101SetAddress(RX_Address, BROAD_0AND255);
#endif
	
	CC1101SetSYNC(0xD391);                 // 8799
	CC1101WriteReg(CC1101_MDMCFG1, 0x22);  // Modem Configuration    0x22
	CC1101WriteReg(CC1101_MDMCFG0, 0xF8);

	CC1101WriteMultiReg(CC1101_PATABLE, PaTabel, 8);

	delay_ms(5);
}

void CC1101_Settings(void) 
{
    CC1101WriteReg(CC1101_FSCTRL0,  rfSettings.FSCTRL2);//????
    // Write register settings
    CC1101WriteReg(CC1101_FSCTRL1,  rfSettings.FSCTRL1);
    CC1101WriteReg(CC1101_FSCTRL0,  rfSettings.FSCTRL0);
    CC1101WriteReg(CC1101_FREQ2,    rfSettings.FREQ2);
    CC1101WriteReg(CC1101_FREQ1,    rfSettings.FREQ1);
    CC1101WriteReg(CC1101_FREQ0,    rfSettings.FREQ0);
    CC1101WriteReg(CC1101_MDMCFG4,  rfSettings.MDMCFG4);
    CC1101WriteReg(CC1101_MDMCFG3,  rfSettings.MDMCFG3);
    CC1101WriteReg(CC1101_MDMCFG2,  rfSettings.MDMCFG2);
    CC1101WriteReg(CC1101_MDMCFG1,  rfSettings.MDMCFG1);
    CC1101WriteReg(CC1101_MDMCFG0,  rfSettings.MDMCFG0);
    CC1101WriteReg(CC1101_CHANNR,   rfSettings.CHANNR);
    CC1101WriteReg(CC1101_DEVIATN,  rfSettings.DEVIATN);
    CC1101WriteReg(CC1101_FREND1,   rfSettings.FREND1);
    CC1101WriteReg(CC1101_FREND0,   rfSettings.FREND0);
    CC1101WriteReg(CC1101_MCSM0 ,   rfSettings.MCSM0 );
    CC1101WriteReg(CC1101_FOCCFG,   rfSettings.FOCCFG);
    CC1101WriteReg(CC1101_BSCFG,    rfSettings.BSCFG);
    CC1101WriteReg(CC1101_AGCCTRL2, rfSettings.AGCCTRL2);
    CC1101WriteReg(CC1101_AGCCTRL1, rfSettings.AGCCTRL1);
    CC1101WriteReg(CC1101_AGCCTRL0, rfSettings.AGCCTRL0);
    CC1101WriteReg(CC1101_FSCAL3,   rfSettings.FSCAL3);
    CC1101WriteReg(CC1101_FSCAL2,   rfSettings.FSCAL2);
    CC1101WriteReg(CC1101_FSCAL1,   rfSettings.FSCAL1);
    CC1101WriteReg(CC1101_FSCAL0,   rfSettings.FSCAL0);
    CC1101WriteReg(CC1101_FSTEST,   rfSettings.FSTEST);
    CC1101WriteReg(CC1101_TEST2,    rfSettings.TEST2);
    CC1101WriteReg(CC1101_TEST1,    rfSettings.TEST1);
    CC1101WriteReg(CC1101_TEST0,    rfSettings.TEST0);
    CC1101WriteReg(CC1101_IOCFG2,   rfSettings.IOCFG2);
    CC1101WriteReg(CC1101_IOCFG0,   rfSettings.IOCFG0);    
    CC1101WriteReg(CC1101_PKTCTRL1, rfSettings.PKTCTRL1);
    CC1101WriteReg(CC1101_PKTCTRL0, rfSettings.PKTCTRL0);
    CC1101WriteReg(CC1101_ADDR,     rfSettings.ADDR);
    CC1101WriteReg(CC1101_PKTLEN,   rfSettings.PKTLEN);
}

volatile u8 CC1101_IRQFlag = 0;
void EXTI4_IRQHandler(void)
{			
	if(EXTI_GetITStatus(EXTI_Line4) == SET)		  // PC4�����½����ж�
	{	
		printf("1\r\n");
		CC1101_IRQFlag = 1;
		EXTI4_Set(0);           // ����line4  
	}		
}

// EXTI4�ⲿ�жϿ���
// en:1,ʹ��; 0,����;  
void EXTI4_Set(u8 en)
{
    EXTI->PR = 1<<4;              // ���LINE4�ϵ��жϱ�־λ
    if(en) EXTI->IMR |= 1<<4;     // ʹ��line4
    else   EXTI->IMR  &= ~(1<<4); // ����line4   
}

// ��ȡRSSIֵ
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

// ��ʱ��3�жϷ������		    
void TIM3_IRQHandler(void)
{ 	
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)//�Ǹ����ж�
	{	 			   
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���TIMx�����жϱ�־ 
		if(RecvWaitTime > 0) RecvWaitTime--;
	}	    
}
// ����TIM3�Ŀ���
// sta:0���ر�; 1,����;
void TIM3_Set(u8 sta)
{
	if(sta)
	{  
		TIM_SetCounter(TIM3,0);     // ���������
		TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);  // ʹ��ָ����TIM3�ж�,��������ж�
		TIM_Cmd(TIM3, ENABLE);      // ʹ��TIM3	
	}
	else 
	{
		TIM_Cmd(TIM3, DISABLE);  // �رն�ʱ��3	   
		TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);                // �ر�ָ����TIM3�ж�,�رո����ж�
	}
}
//ͨ�ö�ʱ���жϳ�ʼ��
//����ʼ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��		 
void TIM3_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);     // TIM3ʱ��ʹ��    
	
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                  // ��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler = psc;               // ����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // ����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);          // ����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);                // �ر�TIM3�����ж�

	 	  
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  // ��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		   // �����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			   // IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	  // ����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	TIM3_Set(0);                      // �ر�TIM3
}
