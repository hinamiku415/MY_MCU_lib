/**
  ******************************************************************************
  * @file    tjc_hmi_config.h
  * @brief   TJC/Nextion 串口屏库 - 用户配置文件
  *
  * 【学习笔记】
  * 这个文件是用户配置文件，可以根据项目需求修改这些宏定义
  * 
  * 配置项分为几类：
  * 1. 缓冲区大小 - 影响内存占用
  * 2. 功能开关 - 启用/禁用某些功能
  * 3. 协议配置 - 帧头帧尾定义
  * 4. 调试配置 - 调试功能开关
  ******************************************************************************
  */

#ifndef __TJC_HMI_CONFIG_H
#define __TJC_HMI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 缓冲区大小 ===================== */
/**
 * 【学习笔记】缓冲区大小配置
 * 
 * TJC_HMI_RX_BUF_SIZE: 接收缓冲区大小
 *   - 用于存储接收到的帧数据
 *   - 如果屏幕发送的数据很长，需要增大这个值
 *   - 默认 64 字节，一般够用
 * 
 * TJC_HMI_TX_BUF_SIZE: 发送缓冲区大小
 *   - 用于拼接要发送的指令
 *   - 如果控件名或文本很长，需要增大这个值
 *   - 默认 96 字节，一般够用
 * 
 * TJC_HMI_TEXT_MAX_LEN: 文本控件内容最大长度
 *   - 限制 SetText() 函数能发送的文本长度
 *   - 默认 32 字节
 */
#define TJC_HMI_RX_BUF_SIZE            64
#define TJC_HMI_TX_BUF_SIZE            96
#define TJC_HMI_TEXT_MAX_LEN            32

/* ===================== 功能开关 ===================== */
/**
 * 【学习笔记】功能开关
 * 
 * TJC_HMI_ENABLE_FLOAT: 启用浮点数显示
 *   - 1: 启用，可以调用 TJC_HMI_SetFloatText()
 *   - 0: 禁用，节省 Flash
 * 
 * TJC_HMI_ENABLE_PRINTF_FMT: 启用格式化发送
 *   - 1: 启用，可以使用 snprintf() 格式化
 *   - 0: 禁用，节省 Flash
 * 
 * TJC_HMI_ENABLE_RX_PARSE: 启用接收解析
 *   - 1: 启用，可以解析接收到的数据
 *   - 0: 禁用，只发送不接收
 */
#define TJC_HMI_ENABLE_FLOAT            1
#define TJC_HMI_ENABLE_PRINTF_FMT       1
#define TJC_HMI_ENABLE_RX_PARSE         1

/* ===================== 协议配置 ===================== */
/**
 * 【学习笔记】协议配置
 * 
 * TJC_HMI_FRAME_HEAD: 帧头字节
 *   - 通用协议使用 0xA5 作为帧头
 *   - 屏幕发送的数据必须以这个字节开头
 * 
 * TJC_HMI_FRAME_TAIL: 帧尾字节
 *   - 通用协议使用 0x5A 作为帧尾
 *   - 屏幕发送的数据必须以这个字节结尾
 * 
 * TJC_HMI_CMD_END_0/1/2: Nextion 指令结束符
 *   - 发送指令时需要追加 3 个 0xFF
 *   - 这是 Nextion/TJC 屏的协议要求
 * 
 * TJC_HMI_NEXTION_TOUCH_HEAD: Nextion 触摸事件包头
 *   - Nextion 屏的触摸事件以 0x65 开头
 *   - 需要跳过这些返回包，避免干扰解析
 */
#define TJC_HMI_FRAME_HEAD              0xA5
#define TJC_HMI_FRAME_TAIL              0x5A
#define TJC_HMI_CMD_END_0               0xFF
#define TJC_HMI_CMD_END_1               0xFF
#define TJC_HMI_CMD_END_2               0xFF
#define TJC_HMI_NEXTION_TOUCH_HEAD      0x65

/* ===================== 超时配置 ===================== */
/**
 * 【学习笔记】超时配置
 * 
 * TJC_HMI_RX_TIMEOUT_MS: 帧接收超时
 *   - 如果一帧数据接收时间超过这个值，会丢弃
 *   - 默认 100ms
 *   - 一般不需要修改
 */
#define TJC_HMI_RX_TIMEOUT_MS           100

/* ===================== 调试配置 ===================== */
/**
 * 【学习笔记】调试配置
 * 
 * TJC_HMI_DEBUG_RX: 启用调试模式
 *   - 取消注释这行，会记录最近接收到的字节
 *   - 可以调用 TJC_HMI_GetDebugBytes() 获取
 *   - 调试完成后建议注释掉，节省内存
 * 
 * TJC_HMI_DEBUG_BUF_SIZE: 调试缓冲区大小
 *   - 记录最近多少个字节
 *   - 默认 16 字节
 */
/* #define TJC_HMI_DEBUG_RX */

#ifdef TJC_HMI_DEBUG_RX
#define TJC_HMI_DEBUG_BUF_SIZE          16
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TJC_HMI_CONFIG_H */
