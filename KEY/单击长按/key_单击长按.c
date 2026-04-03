#include "key.h"
#include "delay.h" 
#include "stm32f4xx.h"
#include "usart.h"



typedef struct
{
	uint32_t rcc; 				//时钟
	GPIO_TypeDef* gpio;  	    //GPIO
	uint16_t pin;  		    	//引脚
	GPIOPuPd_TypeDef pupd; 		//选择上拉下拉
	uint8_t  pressed_level;     //有效电平，可能是0或1
} Key_GPIO_t; 				//按键初始化结构体

static Key_GPIO_t key_gpio_List[] = {
    {RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_PuPd_DOWN,1}, 	//KEY1
    {RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_3, GPIO_PuPd_UP,0}, 	//KEY2
    {RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_4, GPIO_PuPd_UP,0}, 	//KEY3
};

#define KEY_NUM_MAX (sizeof(key_gpio_List)/sizeof(key_gpio_List[0])) 	//获取按键数量


/**
***********************************************************
* @brief 按键硬件初始化
* @param
* @return 
***********************************************************
*/
void Key_DRV_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	for(uint8_t i = 0; i < KEY_NUM_MAX; i++)
	{
		RCC_AHB1PeriphClockCmd(key_gpio_List[i].rcc, ENABLE);  	//使能GPIO时钟
		GPIO_InitStructure.GPIO_Pin = key_gpio_List[i].pin; 
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 			//普通输入模式
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 
		GPIO_InitStructure.GPIO_PuPd = key_gpio_List[i].pupd; 

		GPIO_Init(key_gpio_List[i].gpio, &GPIO_InitStructure); 	//初始化GPIO
	}
}

//状态机状态定义
typedef enum
{
	KEY_RELEASE = 0, 	//按键松开
	KEY_CONFIRM,        //按键确认
	KEY_SHORTPRESS,		//短按
	KEY_LONGPRESS,		//长按
} KEY_STATE;			

//按键信息结构体
typedef struct
{
	KEY_STATE state;	       		//按键状态
	uint32_t prvSysTick;			//按键计时器
	uint8_t  singleClickNum;	  	//短按次数
	uint32_t firstReleseSysTick;	//第一次松开时间
} Key_Info_t;						

static Key_Info_t key_Info_List[KEY_NUM_MAX];		//按键信息数组

#define CONFIRM_TIME   10		//按键消抖.判断短按的时间长度
#define LONGPRESS_TIME 100	    //按键长按，判断长按的时间长度
#define DOUBLE_CLICK_TIME 30	//双击时间间隔,两次抬起的时间间隔



static uint8_t Key_Scan(uint8_t keyIndex)
{
	volatile uint32_t curSysTick = 0; 
	uint8_t KeyPress = 0;
	KeyPress = GPIO_ReadInputDataBit(key_gpio_List[keyIndex].gpio, key_gpio_List[keyIndex].pin);  //读取按键引脚电平    
    
	switch(key_Info_List[keyIndex].state)
	{   
		case KEY_RELEASE:		//按键松开
			if(KeyPress == key_gpio_List[keyIndex].pressed_level)
			{
				key_Info_List[keyIndex].state = KEY_CONFIRM;		//按键确认
				key_Info_List[keyIndex].prvSysTick = g_msTicks;	//记录按键按下时间
			}
			break;
            
		case KEY_CONFIRM:		//按键确认
			if(KeyPress == key_gpio_List[keyIndex].pressed_level)
			{
                curSysTick = g_msTicks;
				if(curSysTick - key_Info_List[keyIndex].prvSysTick >= CONFIRM_TIME)	//消抖
				{
					key_Info_List[keyIndex].state = KEY_SHORTPRESS;		//按键短按
					key_Info_List[keyIndex].prvSysTick = curSysTick;
				}         
			}
			else 
			{
				key_Info_List[keyIndex].state = KEY_RELEASE;		//按键松开
			}
			break;
            
		case KEY_SHORTPRESS:	//按键短按
			if(KeyPress != key_gpio_List[keyIndex].pressed_level) //没有按住
			{
			    key_Info_List[keyIndex].state = KEY_RELEASE;		
				return (uint8_t)(keyIndex + 1);					 //返回按键值,对应0x01 0x02 0x03
			}
			else
			{
			    curSysTick = g_msTicks;
				if(curSysTick - key_Info_List[keyIndex].prvSysTick >= LONGPRESS_TIME)	//长按时间窗
				{
					key_Info_List[keyIndex].state = KEY_LONGPRESS;                  
				}
			}
			break;
            
		case KEY_LONGPRESS:		//按键长按
			if(KeyPress != key_gpio_List[keyIndex].pressed_level)	//没有按住
			{
			    key_Info_List[keyIndex].state = KEY_RELEASE;		//按键松开
				return (uint8_t)(keyIndex + 0x81);					//返回按键值,对应0x81 0x82 0x83
			}
			break;
            
		default:
			key_Info_List[keyIndex].state = KEY_RELEASE;		//按键松开
			break;
	}
	return 0;
}



/**
***********************************************************
* @brief 获取按键码值
* @param
* @return 三个按键码值，短按0x01 0x02 0x03，
			长按0x81 0x82 0x83，没有按下为0
***********************************************************
*/
uint8_t GetKeyVal(void)
{
	uint8_t res = 0;
	for (uint8_t i = 0; i < KEY_NUM_MAX; i++)
	{
		res = Key_Scan(i);
		if(res != 0)
		{
			break;		//检测到按键，跳出循环
		}
	}
	return res;			//返回按键值
}





















