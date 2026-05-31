/**
  ******************************************************************************
  * @file    tjc_hmi_port_stm32f10x.h
  * @brief   TJC/Nextion 串口屏库 - STM32F103 标准库适配层
  *
  * 【学习笔记】
  * 这个文件是 STM32F103 标准库的适配层头文件
  * 
  * 用户可以修改这些宏定义来更改硬件配置：
  * - TJC_HMI_USART: 使用哪个串口（USART1/USART2/USART3）
  * - TJC_HMI_GPIO_PORT: 使用哪个 GPIO 端口（GPIOA/GPIOB）
  * - TJC_HMI_TX_PIN: TX 引脚
  * - TJC_HMI_RX_PIN: RX 引脚
  * - TJC_HMI_NVIC_PREEMPTION: 抢占优先级
  * - TJC_HMI_NVIC_SUB: 响应优先级
  ******************************************************************************
  */

#ifndef __TJC_HMI_PORT_STM32F10X_H
#define __TJC_HMI_PORT_STM32F10X_H

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 硬件配置（用户可修改） ===================== */

/**
 * 【学习笔记】串口外设选择
 * 
 * 默认使用 USART1，如果需要使用其他串口，修改这些宏：
 * - USART1: TJC_HMI_USART = USART1, TJC_HMI_USART_IRQn = USART1_IRQn
 * - USART2: TJC_HMI_USART = USART2, TJC_HMI_USART_IRQn = USART2_IRQn
 * - USART3: TJC_HMI_USART = USART3, TJC_HMI_USART_IRQn = USART3_IRQn
 */
#define TJC_HMI_USART                  USART1
#define TJC_HMI_USART_IRQn             USART1_IRQn
#define TJC_HMI_USART_IRQHandler       USART1_IRQHandler

/**
 * 【学习笔记】GPIO 配置
 * 
 * 默认使用 PA9(TX)/PA10(RX)，如果需要使用其他引脚，修改这些宏：
 * - USART1: PA9(TX)/PA10(RX) 或 PB6(TX)/PB7(RX)
 * - USART2: PA2(TX)/PA3(RX) 或 PD5(TX)/PD6(RX)
 * - USART3: PB10(TX)/PB11(RX) 或 PC10(TX)/PC11(RX) 或 PD8(TX)/PD9(RX)
 */
#define TJC_HMI_GPIO_PORT              GPIOA
#define TJC_HMI_TX_PIN                 GPIO_Pin_9
#define TJC_HMI_RX_PIN                 GPIO_Pin_10

/**
 * 【学习笔记】时钟配置
 * 
 * 需要开启串口和 GPIO 的时钟
 * - USART1: RCC_APB2Periph_USART1
 * - USART2: RCC_APB1Periph_USART2
 * - USART3: RCC_APB1Periph_USART3
 */
#define TJC_HMI_RCC_USART              RCC_APB2Periph_USART1
#define TJC_HMI_RCC_GPIO               RCC_APB2Periph_GPIOA

/**
 * 【学习笔记】NVIC 优先级配置
 * 
 * 抢占优先级和响应优先级
 * 如果系统中有多个中断，需要合理设置优先级
 * 
 * 优先级规则：
 * - 抢占优先级高的可以打断抢占优先级低的
 * - 抢占优先级相同的，响应优先级高的先执行
 * - 抢占优先级和响应优先级都相同的，按硬件默认顺序
 */
#define TJC_HMI_NVIC_PREEMPTION        1
#define TJC_HMI_NVIC_SUB               1

/* ===================== 函数声明 ===================== */

/**
  * @brief  初始化 STM32F103 串口硬件
  * @param  baudrate: 波特率
  * @retval 0=成功
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Init() 接口
  * 负责初始化 USART1 的时钟、GPIO、NVIC 等
  */
uint8_t TJC_Port_Init(uint32_t baudrate);

/**
  * @brief  发送一个字节
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_SendByte() 接口
  * 负责发送一个字节，阻塞等待发送完成
  */
void TJC_Port_SendByte(uint8_t byte);

/**
  * @brief  发送缓冲区数据
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_SendBuffer() 接口
  * 负责发送缓冲区数据，逐个字节发送
  */
void TJC_Port_SendBuffer(const uint8_t *buf, uint16_t len);

/**
  * @brief  获取系统时间戳（毫秒）
  * @note   需要外部提供 g_tick 变量
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_GetTick() 接口
  * 需要用户提供一个毫秒计数器
  */
uint32_t TJC_Port_GetTick(void);

/**
  * @brief  进入临界区（关中断）
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Lock() 接口
  * 调用 __disable_irq() 关闭全局中断
  */
void TJC_Port_Lock(void);

/**
  * @brief  退出临界区（开中断）
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Unlock() 接口
  * 调用 __enable_irq() 开启全局中断
  */
void TJC_Port_Unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __TJC_HMI_PORT_STM32F10X_H */
