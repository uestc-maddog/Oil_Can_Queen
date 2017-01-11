#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"

//按键驱动代码	   	 

#define BL_SWITCH  PCin(11)	     // LCD 背光开关

void KEY_Init(void);    // IO初始化
u8 KEY_Scan(u8 mode);  	// 按键扫描函数					    
#endif
