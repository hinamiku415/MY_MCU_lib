/**
  ******************************************************************************
  * @file    tjc_hmi_types.h
  * @brief   TJC/Nextion 串口屏库 - 数据类型定义
  *
  * 【学习笔记】
  * 这个文件定义了库中用到的所有数据类型，包括：
  * 1. 事件类型枚举 - 用来区分按钮事件和数据事件
  * 2. 事件结构体 - 存储解析后的事件数据
  * 3. 回调函数类型 - 定义用户回调函数的签名
  * 4. 接收状态机状态 - 用于解析接收到的数据
  ******************************************************************************
  */

#ifndef __TJC_HMI_TYPES_H
#define __TJC_HMI_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 事件类型 ===================== */
/**
 * 【学习笔记】事件类型枚举
 * 
 * TJC_EVENT_BUTTON: 按钮事件
 *   - 屏幕发送：A5 01 5A（只有帧头+ID+帧尾，没有数据）
 *   - 含义：用户按下了按钮，event->id 是按钮编号
 * 
 * TJC_EVENT_DATA: 数据事件
 *   - 屏幕发送：A5 01 31 35 30 5A（帧头+ID+数据+帧尾）
 *   - 含义：用户输入了数据，event->id 是数据类型，event->value 是数值
 */
typedef enum {
    TJC_EVENT_NONE = 0,         /* 无事件 */
    TJC_EVENT_BUTTON,           /* 按钮按下（无数据，只有 ID） */
    TJC_EVENT_DATA,             /* 数据输入（有 ID + 数据） */
    TJC_EVENT_ERROR             /* 错误 */
} TJC_EventType_t;

/* ===================== 事件结构体 ===================== */
/**
 * 【学习笔记】事件结构体
 * 
 * 这个结构体存储解析后的事件数据，会作为参数传给用户的回调函数
 * 
 * type: 事件类型（BUTTON 或 DATA）
 *   - BUTTON: event->id 是按钮编号，event->value 无效
 *   - DATA: event->id 是数据类型，event->value 是数值
 * 
 * id: 事件 ID（用户自定义含义）
 *   - 用户在自己的代码中定义 ID 含义，比如：
 *     #define MY_ID_DISTANCE  1
 *     #define MY_ID_NUMBER    2
 * 
 * value: 数值（DATA 事件时有效）
 *   - 从屏幕发送的数据中解析出来的数字
 *   - 比如屏幕发送 A5 01 31 35 30 5A，解析出 value=150
 * 
 * text: 文本（DATA 事件时有效）
 *   - 从屏幕发送的数据中解析出来的原始文本
 *   - 比如屏幕发送 A5 01 31 35 30 5A，解析出 text="150"
 * 
 * raw: 原始数据
 *   - 完整的帧数据，用于调试或自定义解析
 *   - 比如屏幕发送 A5 01 31 35 30 5A，raw 就是这6个字节
 * 
 * raw_len: 原始数据长度
 *   - raw 数组中的有效字节数
 */
typedef struct {
    TJC_EventType_t type;       /* 事件类型 */
    uint8_t id;                 /* 事件 ID（用户自定义含义） */
    uint16_t value;             /* 数值（DATA 事件时有效） */
    char text[32];              /* 文本（DATA 事件时有效） */
    uint8_t raw[32];            /* 原始数据 */
    uint8_t raw_len;            /* 原始数据长度 */
} TJC_Event_t;

/* ===================== 回调函数类型 ===================== */
/**
 * 【学习笔记】回调函数类型
 * 
 * 这是一个函数指针类型，定义了用户回调函数的签名
 * 
 * 用户需要写一个这样的函数：
 *   void MyHandler(const TJC_Event_t *event)
 *   {
 *       // 处理业务逻辑
 *       if (event->id == 1) {
 *           target_D = event->value;
 *       }
 *   }
 * 
 * 然后注册给库：
 *   TJC_HMI_RegisterCallback(MyHandler);
 * 
 * 库在收到数据时会自动调用这个函数
 */
typedef void (*TJC_HMI_Callback_t)(const TJC_Event_t *event);

/* ===================== 接收状态机状态 ===================== */
/**
 * 【学习笔记】接收状态机
 * 
 * 用于解析接收到的字节流，状态转换：
 * 
 * RX_STATE_IDLE -> RX_STATE_DATA: 收到帧头 0xA5
 * RX_STATE_DATA -> RX_STATE_IDLE: 收到帧尾 0x5A，解析帧
 * RX_STATE_IDLE -> RX_STATE_SKIP_NEXTION: 收到 Nextion 返回包头 0x65
 * RX_STATE_SKIP_NEXTION -> RX_STATE_IDLE: 收到 3 个连续 0xFF
 */
typedef enum {
    RX_STATE_IDLE = 0,          /* 空闲，等待帧头 */
    RX_STATE_DATA,              /* 接收数据中 */
    RX_STATE_SKIP_NEXTION       /* 跳过 Nextion 返回包 */
} TJC_RxState_t;

#ifdef __cplusplus
}
#endif

#endif /* __TJC_HMI_TYPES_H */
