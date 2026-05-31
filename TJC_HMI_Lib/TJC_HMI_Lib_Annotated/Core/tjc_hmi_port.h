/**
  ******************************************************************************
  * @file    tjc_hmi_port.h
  * @brief   TJC/Nextion 串口屏库 - 平台适配接口
  *
  * 【学习笔记】
  * 这个文件定义了平台适配层需要实现的函数
  * 
  * 核心库通过这些函数访问硬件，不直接操作寄存器
  * 这样做的好处是：换 MCU 时只需要实现这些函数，核心库代码不用改
  * 
  * 用户需要在具体的 port 文件中实现这些函数：
  * - TJC_Port_Init(): 初始化串口硬件
  * - TJC_Port_SendByte(): 发送一个字节
  * - TJC_Port_SendBuffer(): 发送缓冲区数据
  * - TJC_Port_GetTick(): 获取系统时间戳
  * - TJC_Port_Lock(): 进入临界区（关中断）
  * - TJC_Port_Unlock(): 退出临界区（开中断）
  ******************************************************************************
  */

#ifndef __TJC_HMI_PORT_H
#define __TJC_HMI_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 平台适配函数声明 ===================== */

/**
  * @brief  初始化串口硬件
  * @param  baudrate: 波特率（如 9600、115200）
  * @retval 0=成功，非0=失败
  * 
  * 【学习笔记】
  * 这个函数需要实现：
  * 1. 开启串口时钟
  * 2. 配置 GPIO 引脚（TX/RX）
  * 3. 配置串口参数（波特率、数据位、停止位等）
  * 4. 配置 NVIC 中断
  * 5. 使能串口
  */
uint8_t TJC_Port_Init(uint32_t baudrate);

/**
  * @brief  发送一个字节
  * @param  byte: 要发送的字节
  * @note   阻塞发送，等待发送完成
  * 
  * 【学习笔记】
  * 这个函数需要实现：
  * 1. 把字节写入串口数据寄存器
  * 2. 等待发送完成（检查 TXE 标志）
  * 
  * 典型实现：
  * void TJC_Port_SendByte(uint8_t byte)
  * {
  *     USART_SendData(USART1, byte);
  *     while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  * }
  */
void TJC_Port_SendByte(uint8_t byte);

/**
  * @brief  发送缓冲区数据
  * @param  buf: 数据缓冲区
  * @param  len: 数据长度
  * @note   阻塞发送，等待发送完成
  * 
  * 【学习笔记】
  * 这个函数可以调用 TJC_Port_SendByte() 逐个发送
  * 
  * 典型实现：
  * void TJC_Port_SendBuffer(const uint8_t *buf, uint16_t len)
  * {
  *     for (uint16_t i = 0; i < len; i++) {
  *         TJC_Port_SendByte(buf[i]);
  *     }
  * }
  */
void TJC_Port_SendBuffer(const uint8_t *buf, uint16_t len);

/**
  * @brief  获取系统时间戳（毫秒）
  * @retval 当前时间戳
  * @note   用于接收超时判断，如果不需要超时功能可返回 0
  * 
  * 【学习笔记】
  * 这个函数需要返回一个递增的毫秒计数器
  * 通常使用 SysTick 中断来实现
  * 
  * 典型实现：
  * extern volatile uint32_t g_tick;  // 在 SysTick 中断中递增
  * 
  * uint32_t TJC_Port_GetTick(void)
  * {
  *     return g_tick;
  * }
  */
uint32_t TJC_Port_GetTick(void);

/**
  * @brief  进入临界区（关中断）
  * @note   用于保护共享资源，如果不需要可为空函数
  * 
  * 【学习笔记】
  * 在操作共享资源时需要关中断，防止中断打断
  * 如果你的系统没有并发问题，可以写空函数
  * 
  * 典型实现：
  * void TJC_Port_Lock(void)
  * {
  *     __disable_irq();
  * }
  */
void TJC_Port_Lock(void);

/**
  * @brief  退出临界区（开中断）
  * @note   用于保护共享资源，如果不需要可为空函数
  * 
  * 【学习笔记】
  * 操作完共享资源后，需要重新开中断
  * 如果你的系统没有并发问题，可以写空函数
  * 
  * 典型实现：
  * void TJC_Port_Unlock(void)
  * {
  *     __enable_irq();
  * }
  */
void TJC_Port_Unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* __TJC_HMI_PORT_H */
