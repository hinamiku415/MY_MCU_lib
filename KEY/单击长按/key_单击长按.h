#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h" 

#define KEY1_SHORT_PRESS   0X01
#define KEY1_LONG_PRESS    0X81
#define KEY2_SHORT_PRESS   0X02
#define KEY2_LONG_PRESS    0X82
#define KEY3_SHORT_PRESS   0X03
#define KEY3_LONG_PRESS    0X83
extern volatile uint32_t g_msTicks;


uint8_t GetKeyVal(void);
void Key_DRV_Init(void);

#endif



