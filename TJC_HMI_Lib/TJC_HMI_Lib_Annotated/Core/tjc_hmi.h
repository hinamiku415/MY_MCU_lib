/**
  ******************************************************************************
  * @file    tjc_hmi.h
  * @brief   TJC/Nextion 串口屏库 - 对外主接口
  *
  * 【学习笔记】
  * 这个文件是库的对外接口，用户只需要 include 这个文件
  * 
  * 使用步骤：
  * 1. 调用 TJC_HMI_Init() 初始化
  * 2. 调用 TJC_HMI_RegisterCallback() 注册事件回调
  * 3. 在串口接收中断中调用 TJC_HMI_RxByte() 喂数据
  * 4. 使用 TJC_HMI_SetText()、TJC_HMI_SetNumber() 等发送指令
  ******************************************************************************
  */

#ifndef __TJC_HMI_H
#define __TJC_HMI_H

#include <stdint.h>
#include "tjc_hmi_types.h"
#include "tjc_hmi_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 初始化接口 ===================== */

/**
  * @brief  初始化串口屏库
  * @param  baudrate: 波特率（如 9600、115200）
  * @note   会调用 TJC_Port_Init() 初始化硬件
  * 
  * 【学习笔记】
  * 调用这个函数后，库会：
  * 1. 调用 TJC_Port_Init() 初始化串口硬件
  * 2. 重置接收状态机
  * 3. 发送 bkcmd=0 关闭 Nextion 返回包
  */
void TJC_HMI_Init(uint32_t baudrate);

/**
  * @brief  注册事件回调函数
  * @param  callback: 回调函数指针
  * @note   收到屏幕数据后会调用此回调
  * 
  * 【学习笔记】
  * 用户需要自己写一个回调函数，然后注册给库：
  * 
  * void MyHandler(const TJC_Event_t *event)
  * {
  *     if (event->id == 1) {
  *         target_D = event->value;
  *     }
  * }
  * 
  * TJC_HMI_RegisterCallback(MyHandler);
  */
void TJC_HMI_RegisterCallback(TJC_HMI_Callback_t callback);

/* ===================== 接收接口 ===================== */

/**
  * @brief  喂入接收到的字节
  * @param  byte: 从串口接收到的字节
  * @note   在串口接收中断中调用此函数
  * 
  * 【学习笔记】
  * 在 USART1_IRQHandler 中调用：
  * 
  * void USART1_IRQHandler(void)
  * {
  *     if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
  *         uint8_t byte = USART_ReceiveData(USART1);
  *         TJC_HMI_RxByte(byte);  // 喂给库
  *         USART_ClearITPendingBit(USART1, USART_IT_RXNE);
  *     }
  * }
  */
void TJC_HMI_RxByte(uint8_t byte);

/* ===================== 发送接口 ===================== */

/**
  * @brief  发送原始指令（自动追加 FF FF FF 结束符）
  * @param  cmd: 指令字符串，如 "t0.txt=\"hello\""
  * 
  * 【学习笔记】
  * 这个函数会：
  * 1. 发送指令字符串
  * 2. 自动追加 3 个 0xFF 结束符
  * 
  * 示例：
  * TJC_HMI_SendCmd("t0.txt=\"hello\"");
  * 实际发送：74 30 2E 74 78 74 3D 22 68 65 6C 6C 6F 22 FF FF FF
  */
void TJC_HMI_SendCmd(const char *cmd);

/**
  * @brief  发送原始数据（不追加结束符）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * 
  * 【学习笔记】
  * 这个函数只发送数据，不追加结束符
  * 一般不直接使用，用 TJC_HMI_SendCmd() 更方便
  */
void TJC_HMI_SendRaw(const uint8_t *data, uint16_t len);

/**
  * @brief  发送指令结束符（FF FF FF）
  * 
  * 【学习笔记】
  * Nextion/TJC 屏要求每条指令以 3 个 0xFF 结尾
  * TJC_HMI_SendCmd() 会自动调用这个函数
  */
void TJC_HMI_SendEnd(void);

/* ===================== 控件操作接口 ===================== */

/**
  * @brief  设置文本控件内容
  * @param  obj: 控件名，如 "t0"、"D"
  * @param  text: 文本内容
  * @example TJC_HMI_SetText("D", "120.5cm")
  *          生成: D.txt="120.5cm" FF FF FF
  * 
  * 【学习笔记】
  * 这是最常用的函数，用于更新屏幕上的文本显示
  * 
  * 示例：
  * TJC_HMI_SetText("t0", "Hello");        // 设置 t0 控件显示 "Hello"
  * TJC_HMI_SetText("D", "120.5cm");       // 设置 D 控件显示 "120.5cm"
  */
void TJC_HMI_SetText(const char *obj, const char *text);

/**
  * @brief  设置数字控件值
  * @param  obj: 控件名，如 "n0"
  * @param  num: 数值
  * @example TJC_HMI_SetNumber("n0", 123)
  *          生成: n0.val=123 FF FF FF
  * 
  * 【学习笔记】
  * 用于更新屏幕上的数字控件
  * 注意：这个函数设置的是控件的 val 属性，不是 txt 属性
  */
void TJC_HMI_SetNumber(const char *obj, int32_t num);

/**
  * @brief  设置浮点数（转为文本显示）
  * @param  obj: 控件名
  * @param  value: 浮点数值
  * @param  decimals: 小数位数
  * @example TJC_HMI_SetFloatText("D", 120.5f, 1)
  *          生成: D.txt="120.5" FF FF FF
  * 
  * 【学习笔记】
  * Nextion/TJC 屏没有原生的浮点控件
  * 这个函数把浮点数转为文本，然后用 SetText() 发送
  * 
  * 示例：
  * TJC_HMI_SetFloatText("D", 120.5f, 1);  // 显示 "120.5"
  * TJC_HMI_SetFloatText("D", 120.5f, 2);  // 显示 "120.50"
  */
void TJC_HMI_SetFloatText(const char *obj, float value, uint8_t decimals);

/**
  * @brief  切换页面（按名称）
  * @param  page: 页面名称
  * @example TJC_HMI_Page("main")
  *          生成: page main FF FF FF
  * 
  * 【学习笔记】
  * 切换屏幕的显示页面
  * 页面名称需要在屏幕工程中定义
  */
void TJC_HMI_Page(const char *page);

/**
  * @brief  切换页面（按 ID）
  * @param  page_id: 页面 ID
  * @example TJC_HMI_PageId(1)
  *          生成: page 1 FF FF FF
  * 
  * 【学习笔记】
  * 切换屏幕的显示页面
  * 页面 ID 需要在屏幕工程中定义
  */
void TJC_HMI_PageId(uint8_t page_id);

/**
  * @brief  设置控件数值属性
  * @param  obj: 控件名
  * @param  attr: 属性名，如 "val"、"pco"
  * @param  value: 数值
  * @example TJC_HMI_SetValue("j0", "val", 50)
  *          生成: j0.val=50 FF FF FF
  * 
  * 【学习笔记】
  * 通用的数值属性设置函数
  * 常用属性：
  * - val: 数值
  * - pco: 字体颜色
  * - bco: 背景颜色
  * - x/y: 位置
  * - w/h: 尺寸
  */
void TJC_HMI_SetValue(const char *obj, const char *attr, int32_t value);

/**
  * @brief  设置控件字符串属性
  * @param  obj: 控件名
  * @param  attr: 属性名，如 "txt"
  * @param  value: 字符串值
  * @example TJC_HMI_SetString("t0", "txt", "OK")
  *          生成: t0.txt="OK" FF FF FF
  * 
  * 【学习笔记】
  * 通用的字符串属性设置函数
  * 常用属性：
  * - txt: 文本内容
  */
void TJC_HMI_SetString(const char *obj, const char *attr, const char *value);

/**
  * @brief  显示控件
  * @param  obj: 控件名
  * @example TJC_HMI_Show("t0")
  *          生成: vis t0,1
  * 
  * 【学习笔记】
  * 显示一个隐藏的控件
  */
void TJC_HMI_Show(const char *obj);

/**
  * @brief  隐藏控件
  * @param  obj: 控件名
  * @example TJC_HMI_Hide("t0")
  *          生成: vis t0,0
  * 
  * 【学习笔记】
  * 隐藏一个控件
  */
void TJC_HMI_Hide(const char *obj);

/**
  * @brief  清空文本控件
  * @param  obj: 控件名
  * 
  * 【学习笔记】
  * 等价于 TJC_HMI_SetText(obj, "")
  */
void TJC_HMI_ClearText(const char *obj);

/* ===================== 系统指令接口 ===================== */

/**
  * @brief  设置指令返回模式
  * @param  mode: 0=不返回, 1=成功时返回, 2=失败时返回, 3=总是返回
  * @example TJC_HMI_SetBkcmd(0)
  *          生成: bkcmd=0 FF FF FF
  * 
  * 【学习笔记】
  * Nextion/TJC 屏执行指令后会返回执行结果
  * 设置 bkcmd=0 可以关闭返回包，避免干扰接收
  * 
  * 建议在初始化时调用：TJC_HMI_SetBkcmd(0);
  */
void TJC_HMI_SetBkcmd(uint8_t mode);

/* ===================== 调试接口 ===================== */

#ifdef TJC_HMI_DEBUG_RX

/**
  * @brief  获取最近接收的字节（调试用）
  * @param  out: 输出缓冲区
  * @param  len: 要获取的字节数
  * 
  * 【学习笔记】
  * 调试函数，需要先在 tjc_hmi_config.h 中启用 TJC_HMI_DEBUG_RX
  * 可以查看最近接收到的原始字节，用于排查问题
  */
void TJC_HMI_GetDebugBytes(uint8_t *out, uint8_t len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __TJC_HMI_H */
