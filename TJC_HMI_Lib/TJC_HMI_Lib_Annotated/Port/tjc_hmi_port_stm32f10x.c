/**
  ******************************************************************************
  * @file    tjc_hmi_port_stm32f10x.c
  * @brief   TJC/Nextion 串口屏库 - STM32F103 标准库适配层实现
  *
  * 【学习笔记】
  * 这个文件是 STM32F103 标准库的适配层实现
  * 
  * 默认配置：
  *   USART1, PA9(TX) / PA10(RX), 9600bps
  * 
  * 用户可修改 tjc_hmi_port_stm32f10x.h 中的宏定义来更改配置
  * 
  * 这个文件实现了 6 个平台适配函数：
  * 1. TJC_Port_Init() - 初始化串口硬件
  * 2. TJC_Port_SendByte() - 发送一个字节
  * 3. TJC_Port_SendBuffer() - 发送缓冲区数据
  * 4. TJC_Port_GetTick() - 获取系统时间戳
  * 5. TJC_Port_Lock() - 进入临界区
  * 6. TJC_Port_Unlock() - 退出临界区
  * 
  * 还包含 USART1 中断处理函数
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "tjc_hmi_port_stm32f10x.h"
#include "tjc_hmi.h"

/* ===================== 外部变量 ===================== */

/**
 * 【学习笔记】系统滴答计数器
 * 
 * 这个变量需要由用户提供，通常在 SysTick 中断中递增
 * 用于 TJC_Port_GetTick() 返回系统时间
 * 
 * 典型实现：
 * volatile uint32_t g_tick = 0;
 * 
 * void SysTick_Handler(void)
 * {
 *     g_tick++;
 * }
 */
extern volatile uint32_t g_tick;

/* ===================== 初始化 ===================== */

/**
  * @brief  初始化 STM32F103 串口硬件
  * @param  baudrate: 波特率
  * @retval 0=成功
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Init() 接口
  * 
  * 初始化过程：
  * 1. 开启 USART1 和 GPIOA 的时钟
  * 2. 配置 PA9 为复用推挽输出（TX）
  * 3. 配置 PA10 为浮空输入（RX）
  * 4. 配置 USART1 参数（波特率、数据位、停止位等）
  * 5. 开启 USART1 接收中断
  * 6. 配置 NVIC 中断优先级
  * 7. 使能 USART1
  * 
  * 调用方式：
  * TJC_Port_Init(9600);  // 初始化，波特率 9600
  */
uint8_t TJC_Port_Init(uint32_t baudrate)
{
    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(TJC_HMI_RCC_USART, ENABLE);  // 开启 USART1 时钟
    RCC_APB2PeriphClockCmd(TJC_HMI_RCC_GPIO, ENABLE);   // 开启 GPIOA 时钟

    /* GPIO 配置 */
    GPIO_InitTypeDef GPIO_InitStructure;

    /* TX: 复用推挽输出 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;      // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin   = TJC_HMI_TX_PIN;       // PA9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      // 50MHz
    GPIO_Init(TJC_HMI_GPIO_PORT, &GPIO_InitStructure);

    /* RX: 浮空输入 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入
    GPIO_InitStructure.GPIO_Pin  = TJC_HMI_RX_PIN;        // PA10
    GPIO_Init(TJC_HMI_GPIO_PORT, &GPIO_InitStructure);

    /* USART 配置 */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate            = baudrate;                    // 波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  // 无硬件流控
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;   // 收发模式
    USART_InitStructure.USART_Parity              = USART_Parity_No;             // 无奇偶校验
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;            // 1位停止位
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;         // 8位数据位
    USART_Init(TJC_HMI_USART, &USART_InitStructure);

    /* 开启接收中断 */
    USART_ITConfig(TJC_HMI_USART, USART_IT_RXNE, ENABLE);  // 接收非空中断

    /* NVIC 配置 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel                   = TJC_HMI_USART_IRQn;  // USART1 中断
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;               // 使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TJC_HMI_NVIC_PREEMPTION;  // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = TJC_HMI_NVIC_SUB;         // 响应优先级
    NVIC_Init(&NVIC_InitStructure);

    /* 使能 USART */
    USART_Cmd(TJC_HMI_USART, ENABLE);  // 使能 USART1

    return 0;
}

/* ===================== 发送 ===================== */

/**
  * @brief  发送一个字节
  * @param  byte: 要发送的字节
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_SendByte() 接口
  * 
  * 发送过程：
  * 1. 把字节写入 USART 数据寄存器
  * 2. 等待发送完成（检查 TXE 标志）
  * 
  * TXE 标志：
  * - 0: 数据寄存器满，不能写入
  * - 1: 数据寄存器空，可以写入
  * 
  * 调用方式：
  * TJC_Port_SendByte(0x41);  // 发送 'A'
  */
void TJC_Port_SendByte(uint8_t byte)
{
    USART_SendData(TJC_HMI_USART, byte);  // 写入数据寄存器
    while (USART_GetFlagStatus(TJC_HMI_USART, USART_FLAG_TXE) == RESET);  // 等待发送完成
}

/**
  * @brief  发送缓冲区数据
  * @param  buf: 数据缓冲区
  * @param  len: 数据长度
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_SendBuffer() 接口
  * 
  * 发送过程：
  * 遍历缓冲区，逐个字节调用 TJC_Port_SendByte() 发送
  * 
  * 调用方式：
  * uint8_t data[] = {0x41, 0x42, 0x43};
  * TJC_Port_SendBuffer(data, 3);  // 发送 "ABC"
  */
void TJC_Port_SendBuffer(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        TJC_Port_SendByte(buf[i]);  // 逐个字节发送
    }
}

/* ===================== 系统 ===================== */

/**
  * @brief  获取系统时间戳（毫秒）
  * @retval 当前时间戳
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_GetTick() 接口
  * 
  * 需要用户提供一个毫秒计数器 g_tick
  * 通常在 SysTick 中断中递增
  * 
  * 典型实现：
  * volatile uint32_t g_tick = 0;
  * 
  * void SysTick_Handler(void)
  * {
  *     g_tick++;
  * }
  * 
  * 调用方式：
  * uint32_t now = TJC_Port_GetTick();  // 获取当前时间
  */
uint32_t TJC_Port_GetTick(void)
{
    return g_tick;
}

/**
  * @brief  进入临界区（关中断）
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Lock() 接口
  * 
  * 调用 __disable_irq() 关闭全局中断
  * 用于保护共享资源，防止中断打断
  * 
  * 使用场景：
  * - 操作共享变量时
  * - 操作硬件寄存器时
  * 
  * 调用方式：
  * TJC_Port_Lock();    // 关中断
  * // 操作共享资源
  * TJC_Port_Unlock();  // 开中断
  */
void TJC_Port_Lock(void)
{
    __disable_irq();  // 关闭全局中断
}

/**
  * @brief  退出临界区（开中断）
  * 
  * 【学习笔记】
  * 这个函数实现了 TJC_Port_Unlock() 接口
  * 
  * 调用 __enable_irq() 开启全局中断
  * 用于恢复中断，与 TJC_Port_Lock() 配对使用
  * 
  * 调用方式：
  * TJC_Port_Lock();    // 关中断
  * // 操作共享资源
  * TJC_Port_Unlock();  // 开中断
  */
void TJC_Port_Unlock(void)
{
    __enable_irq();  // 开启全局中断
}

/* ===================== 中断处理 ===================== */

/**
  * @brief  USART1 中断服务函数
  * 
  * 【学习笔记】
  * 这个函数是 USART1 的中断处理函数
  * 
  * 当 USART1 收到数据时，会触发这个中断
  * 中断处理过程：
  * 1. 检查是否是接收中断（RXNE 标志）
  * 2. 读取接收到的字节
  * 3. 调用 TJC_HMI_RxByte() 喂给库
  * 4. 清除中断标志
  * 
  * 调用链：
  * USART1_IRQHandler() → TJC_HMI_RxByte() → 解析帧 → _TriggerEvent() → 用户回调
  * 
  * 注意：
  * 这个函数名必须和启动文件中的中断向量表一致
  * 如果使用其他串口，需要改为对应的中断函数名
  * - USART2: USART2_IRQHandler
  * - USART3: USART3_IRQHandler
  */
void TJC_HMI_USART_IRQHandler(void)
{
    if (USART_GetITStatus(TJC_HMI_USART, USART_IT_RXNE) != RESET) {
        uint8_t byte = USART_ReceiveData(TJC_HMI_USART);  // 读取接收到的字节
        TJC_HMI_RxByte(byte);  // 喂给库
        USART_ClearITPendingBit(TJC_HMI_USART, USART_IT_RXNE);  // 清除中断标志
    }
}
