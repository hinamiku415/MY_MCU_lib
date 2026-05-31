/**
  ******************************************************************************
  * @file    tjc_hmi.c
  * @brief   TJC/Nextion 串口屏库 - 核心逻辑实现
  *
  * 【学习笔记】
  * 这个文件是库的核心实现，包含：
  * 1. 内部变量 - 存储状态和缓冲区
  * 2. 内部函数 - 触发回调、解析帧
  * 3. 接收处理 - 状态机解析字节流
  * 4. 初始化接口 - 初始化库和注册回调
  * 5. 发送接口 - 发送指令和数据
  * 6. 控件操作 - 设置文本、数字、页面等
  * 7. 系统指令 - 设置 bkcmd 等
  * 8. 调试接口 - 获取调试信息
  ******************************************************************************
  */

#include "tjc_hmi.h"
#include "tjc_hmi_port.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  内部变量
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * 【学习笔记】事件回调函数指针
 * 
 * 用户调用 TJC_HMI_RegisterCallback() 时，会把函数指针保存到这里
 * 库在解析完帧后，会通过这个指针调用用户的函数
 */
static TJC_HMI_Callback_t s_callback = NULL;

/**
 * 【学习笔记】接收状态机相关变量
 * 
 * s_rx_state: 当前状态（空闲/接收中/跳过Nextion包）
 * s_rx_buf: 接收缓冲区，存储收到的字节
 * s_rx_pos: 当前接收到的位置
 */
static TJC_RxState_t s_rx_state = RX_STATE_IDLE;
static uint8_t s_rx_buf[TJC_HMI_RX_BUF_SIZE];
static uint8_t s_rx_pos = 0;

/**
 * 【学习笔记】发送缓冲区
 * 
 * 用于拼接要发送的指令
 * 比如 SetText() 会拼接 "D.txt=\"120.5cm\""
 */
static char s_tx_buf[TJC_HMI_TX_BUF_SIZE];

/**
 * 【学习笔记】Nextion 返回包计数
 * 
 * 用于跳过 Nextion 触摸事件包
 * 收到 0x65 后，需要跳过直到收到 3 个连续 0xFF
 */
static uint8_t s_nextion_ff_count = 0;

/**
 * 【学习笔记】调试缓冲区
 * 
 * 记录最近接收到的字节，用于调试
 * 需要在 tjc_hmi_config.h 中启用 TJC_HMI_DEBUG_RX
 */
#ifdef TJC_HMI_DEBUG_RX
static uint8_t s_debug_buf[TJC_HMI_DEBUG_BUF_SIZE];
static uint8_t s_debug_idx = 0;
#endif


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  内部函数
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
  * @brief  触发事件回调（调用用户注册的回调函数）
  * @param  event: 事件数据
  * 
  * 【学习笔记】
  * 这个函数是"回调"的核心：
  * 1. 检查用户是否注册了回调函数
  * 2. 如果注册了，就调用用户的函数，把事件传给用户
  * 
  * 调用链：
  * _TriggerEvent(&event) → s_callback(event) → MyHandler(&event)
  */
static void _TriggerEvent(const TJC_Event_t *event)
{
    if (s_callback != NULL) {
        s_callback(event);  // 调用用户的回调函数
    }
}

/**
  * @brief  解析通用协议帧（A5 ID DATA 5A）
  * 
  * 【学习笔记】
  * 这个函数解析收到的帧数据：
  * 
  * 帧格式：[0xA5] [ID] [DATA...] [0x5A]
  * 
  * 解析过程：
  * 1. 检查帧长度是否合法
  * 2. 提取 ID（第二个字节）
  * 3. 如果有数据，解析出数值和文本
  * 4. 生成事件，调用 _TriggerEvent()
  * 
  * 示例：
  * 收到 A5 01 31 35 30 5A
  * - s_rx_buf[0] = 0xA5 (帧头)
  * - s_rx_buf[1] = 0x01 (ID)
  * - s_rx_buf[2..4] = 0x31,0x35,0x30 (数据 "150")
  * - s_rx_buf[5] = 0x5A (帧尾)
  * 
  * 解析结果：
  * - event.id = 1
  * - event.value = 150
  * - event.text = "150"
  * - event.type = TJC_EVENT_DATA
  */
static void _ParseGenericFrame(void)
{
    if (s_rx_pos < 3) return;  /* 至少需要：帧头 + ID + 帧尾 */

    TJC_Event_t event;
    memset(&event, 0, sizeof(event));

    /* 第二个字节是 ID */
    event.id = s_rx_buf[1];

    /* 检查是否有数据 */
    if (s_rx_pos > 3) {
        /**
         * 【学习笔记】ASCII 字符串转整数算法
         * 
         * 问题：屏幕发送的是 ASCII 字符，需要转成数字
         * 比如 "150" 的 ASCII 是 0x31 0x35 0x30，要转成数字 150
         * 
         * 算法：value = value * 10 + (字符 - '0')
         * 
         * 逐步推导（以 "150" 为例）：
         * 第1步：value = 0 * 10 + ('1' - '0') = 0 + 1 = 1
         * 第2步：value = 1 * 10 + ('5' - '0') = 10 + 5 = 15
         * 第3步：value = 15 * 10 + ('0' - '0') = 150 + 0 = 150
         * 
         * 关键点：
         * 1. value * 10：把已有数字左移一位（1→10，15→150）
         * 2. 字符 - '0'：把 ASCII 字符转成数字（'0'=48, '1'=49, ...）
         * 3. 两者相加：把新数字拼到末尾
         * 
         * 循环范围：i = 2 到 s_rx_pos - 1
         *   - 从 2 开始：跳过帧头(0xA5)和ID
         *   - 到 s_rx_pos - 1：跳过帧尾(0x5A)
         * 
         * 过滤非数字字符：
         *   if (s_rx_buf[i] >= '0' && s_rx_buf[i] <= '9')
         *   - 只处理 '0'~'9' 的字符
         *   - 忽略其他字符（如 \r \n 等）
         */
        uint16_t value = 0;
        for (uint8_t i = 2; i < s_rx_pos - 1; i++) {
            if (s_rx_buf[i] >= '0' && s_rx_buf[i] <= '9') {
                value = value * 10 + (s_rx_buf[i] - '0');
            }
        }
        event.value = value;

        /* 同时保存文本 */
        uint8_t data_len = s_rx_pos - 3;  /* 减去帧头、ID、帧尾 */
        if (data_len >= sizeof(event.text)) {
            data_len = sizeof(event.text) - 1;
        }
        memcpy(event.text, &s_rx_buf[2], data_len);
        event.text[data_len] = '\0';

        /* 有数据的帧 */
        event.type = TJC_EVENT_DATA;
    } else {
        /* 无数据，是按钮事件 */
        event.type = TJC_EVENT_BUTTON;
    }

    /* 保存原始数据 */
    uint8_t raw_len = s_rx_pos;
    if (raw_len >= sizeof(event.raw)) {
        raw_len = sizeof(event.raw) - 1;
    }
    memcpy(event.raw, s_rx_buf, raw_len);
    event.raw_len = raw_len;

    /* 触发回调，把事件传给用户 */
    _TriggerEvent(&event);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  接收处理（在中断中调用）
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
  * @brief  喂入接收到的字节（在串口接收中断中调用）
  * @param  byte: 从串口接收到的字节
  * 
  * 【学习笔记】
  * 这个函数是接收的核心，使用状态机解析字节流：
  * 
  * 状态机：
  * IDLE -> 收到 0xA5 -> 进入 DATA 状态
  * IDLE -> 收到 0x65 -> 进入 SKIP_NEXTION 状态
  * DATA -> 收到 0x5A -> 解析帧，回到 IDLE
  * DATA -> 收到其他字节 -> 存入缓冲区
  * SKIP_NEXTION -> 收到 3 个 0xFF -> 回到 IDLE
  * 
  * 调用方式：
  * void USART1_IRQHandler(void)
  * {
  *     if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
  *         uint8_t byte = USART_ReceiveData(USART1);
  *         TJC_HMI_RxByte(byte);  // 喂给库
  *         USART_ClearITPendingBit(USART1, USART_IT_RXNE);
  *     }
  * }
  */
void TJC_HMI_RxByte(uint8_t byte)
{
#ifdef TJC_HMI_DEBUG_RX
    /* 记录调试字节 */
    s_debug_buf[s_debug_idx & (TJC_HMI_DEBUG_BUF_SIZE - 1)] = byte;
    s_debug_idx++;
#endif

    /* ---- Nextion 返回包处理 ---- */

    /* 检测到 0x65（触摸事件包头），进入跳过模式 */
    if (byte == TJC_HMI_NEXTION_TOUCH_HEAD && s_rx_state == RX_STATE_IDLE) {
        s_rx_state = RX_STATE_SKIP_NEXTION;
        s_nextion_ff_count = 0;
        return;
    }

    /* 跳过 Nextion 返回包，直到收到 3 个连续 FF */
    if (s_rx_state == RX_STATE_SKIP_NEXTION) {
        if (byte == 0xFF) {
            s_nextion_ff_count++;
            if (s_nextion_ff_count >= 3) {
                s_rx_state = RX_STATE_IDLE;
                s_nextion_ff_count = 0;
            }
        } else {
            s_nextion_ff_count = 0;
        }
        return;
    }

    /* 忽略 0xFF（Nextion 结束符，不属于用户数据） */
    if (byte == 0xFF) return;

    /* ---- 通用协议解析（A5 ID DATA 5A） ---- */

    if (byte == TJC_HMI_FRAME_HEAD) {
        /* 收到帧头，开始新帧 */
        s_rx_state = RX_STATE_DATA;
        s_rx_pos = 0;
        s_rx_buf[s_rx_pos++] = byte;
        return;
    }

    if (s_rx_state == RX_STATE_DATA) {
        if (byte == TJC_HMI_FRAME_TAIL) {
            /* 收到帧尾，解析帧 */
            s_rx_buf[s_rx_pos++] = byte;
            _ParseGenericFrame();
            s_rx_state = RX_STATE_IDLE;
            s_rx_pos = 0;
        } else if (s_rx_pos < TJC_HMI_RX_BUF_SIZE - 1) {
            /* 收到数据，存入缓冲区 */
            s_rx_buf[s_rx_pos++] = byte;
        } else {
            /* 缓冲区溢出，丢弃 */
            s_rx_state = RX_STATE_IDLE;
            s_rx_pos = 0;
        }
        return;
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  初始化接口
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
  * @brief  初始化串口屏库
  * @param  baudrate: 波特率（如 9600、115200）
  * 
  * 【学习笔记】
  * 使用库的第一步，需要先调用这个函数
  * 
  * 初始化过程：
  * 1. 调用 TJC_Port_Init() 初始化串口硬件
  * 2. 重置接收状态机
  * 3. 清空回调函数
  * 4. 发送 bkcmd=0 关闭返回包
  * 
  * 使用示例：
  * TJC_HMI_Init(9600);  // 初始化，波特率 9600
  */
void TJC_HMI_Init(uint32_t baudrate)
{
    /* 初始化硬件 */
    TJC_Port_Init(baudrate);

    /* 重置状态机 */
    s_rx_state = RX_STATE_IDLE;
    s_rx_pos = 0;
    s_callback = NULL;

    /* 关闭 Nextion 返回包 */
    TJC_HMI_SetBkcmd(0);
}

/**
  * @brief  注册事件回调函数
  * @param  callback: 回调函数指针
  * 
  * 【学习笔记】
  * 使用库的第二步，注册回调函数
  * 
  * 回调函数会在收到数据时被自动调用
  * 
  * 使用示例：
  * void MyHandler(const TJC_Event_t *event)
  * {
  *     if (event->id == 1) {
  *         target_D = event->value;
  *     }
  * }
  * 
  * TJC_HMI_RegisterCallback(MyHandler);
  */
void TJC_HMI_RegisterCallback(TJC_HMI_Callback_t callback)
{
    s_callback = callback;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  发送接口
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
  * @brief  发送指令结束符（FF FF FF）
  * 
  * 【学习笔记】
  * Nextion/TJC 屏要求每条指令以 3 个 0xFF 结尾
  * TJC_HMI_SendCmd() 会自动调用这个函数
  */
void TJC_HMI_SendEnd(void)
{
    TJC_Port_SendByte(TJC_HMI_CMD_END_0);
    TJC_Port_SendByte(TJC_HMI_CMD_END_1);
    TJC_Port_SendByte(TJC_HMI_CMD_END_2);
}

/**
  * @brief  发送原始指令（自动追加 FF FF FF 结束符）
  * @param  cmd: 指令字符串，如 "t0.txt=\"hello\""
  * 
  * 【学习笔记】
  * 这是最常用的发送函数
  * 
  * 使用示例：
  * TJC_HMI_SendCmd("t0.txt=\"hello\"");
  * 实际发送：74 30 2E 74 78 74 3D 22 68 65 6C 6C 6F 22 FF FF FF
  */
void TJC_HMI_SendCmd(const char *cmd)
{
    if (cmd == NULL) return;

    uint16_t len = strlen(cmd);
    TJC_Port_SendBuffer((const uint8_t *)cmd, len);
    TJC_HMI_SendEnd();
}

/**
  * @brief  发送原始数据（不追加结束符）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * 
  * 【学习笔记】
  * 这个函数只发送数据，不追加结束符
  * 一般不直接使用，用 TJC_HMI_SendCmd() 更方便
  */
void TJC_HMI_SendRaw(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return;
    TJC_Port_SendBuffer(data, len);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  控件操作接口
 * ═══════════════════════════════════════════════════════════════════════════ */

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
  * 内部实现：
  * 1. 用 snprintf 拼接指令：D.txt="120.5cm"
  * 2. 调用 SendCmd 发送，自动追加 FF FF FF
  * 
  * 使用示例：
  * TJC_HMI_SetText("t0", "Hello");   // 设置 t0 控件显示 "Hello"
  * TJC_HMI_SetText("D", "120.5cm");  // 设置 D 控件显示 "120.5cm"
  */
void TJC_HMI_SetText(const char *obj, const char *text)
{
    if (obj == NULL || text == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "%s.txt=\"%s\"", obj, text);
    TJC_HMI_SendCmd(s_tx_buf);
}

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
  * 
  * 使用示例：
  * TJC_HMI_SetNumber("n0", 123);  // 设置 n0 控件的值为 123
  */
void TJC_HMI_SetNumber(const char *obj, int32_t num)
{
    if (obj == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "%s.val=%ld", obj, (long)num);
    TJC_HMI_SendCmd(s_tx_buf);
}

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
  * 使用示例：
  * TJC_HMI_SetFloatText("D", 120.5f, 1);  // 显示 "120.5"
  * TJC_HMI_SetFloatText("D", 120.5f, 2);  // 显示 "120.50"
  */
void TJC_HMI_SetFloatText(const char *obj, float value, uint8_t decimals)
{
    if (obj == NULL) return;

    /* 根据小数位数生成格式字符串 */
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%.%uf", decimals);

    char num_str[16];
    snprintf(num_str, sizeof(num_str), fmt, value);

    TJC_HMI_SetText(obj, num_str);
}

/**
  * @brief  切换页面（按名称）
  * @param  page: 页面名称
  * @example TJC_HMI_Page("main")
  *          生成: page main FF FF FF
  * 
  * 【学习笔记】
  * 切换屏幕的显示页面
  * 页面名称需要在屏幕工程中定义
  * 
  * 使用示例：
  * TJC_HMI_Page("main");    // 切换到 main 页面
  * TJC_HMI_Page("config");  // 切换到 config 页面
  */
void TJC_HMI_Page(const char *page)
{
    if (page == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "page %s", page);
    TJC_HMI_SendCmd(s_tx_buf);
}

/**
  * @brief  切换页面（按 ID）
  * @param  page_id: 页面 ID
  * @example TJC_HMI_PageId(1)
  *          生成: page 1 FF FF FF
  * 
  * 【学习笔记】
  * 切换屏幕的显示页面
  * 页面 ID 需要在屏幕工程中定义
  * 
  * 使用示例：
  * TJC_HMI_PageId(0);  // 切换到页面 0
  * TJC_HMI_PageId(1);  // 切换到页面 1
  */
void TJC_HMI_PageId(uint8_t page_id)
{
    snprintf(s_tx_buf, sizeof(s_tx_buf), "page %u", page_id);
    TJC_HMI_SendCmd(s_tx_buf);
}

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
  * 
  * 使用示例：
  * TJC_HMI_SetValue("j0", "val", 50);     // 进度条值
  * TJC_HMI_SetValue("t0", "pco", 63488);  // 字体颜色（红色）
  */
void TJC_HMI_SetValue(const char *obj, const char *attr, int32_t value)
{
    if (obj == NULL || attr == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "%s.%s=%ld", obj, attr, (long)value);
    TJC_HMI_SendCmd(s_tx_buf);
}

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
  * 
  * 使用示例：
  * TJC_HMI_SetString("t0", "txt", "OK");  // 设置文本
  */
void TJC_HMI_SetString(const char *obj, const char *attr, const char *value)
{
    if (obj == NULL || attr == NULL || value == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "%s.%s=\"%s\"", obj, attr, value);
    TJC_HMI_SendCmd(s_tx_buf);
}

/**
  * @brief  显示控件
  * @param  obj: 控件名
  * @example TJC_HMI_Show("t0")
  *          生成: vis t0,1
  * 
  * 【学习笔记】
  * 显示一个隐藏的控件
  * 
  * 使用示例：
  * TJC_HMI_Show("t0");  // 显示 t0 控件
  */
void TJC_HMI_Show(const char *obj)
{
    if (obj == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "vis %s,1", obj);
    TJC_HMI_SendCmd(s_tx_buf);
}

/**
  * @brief  隐藏控件
  * @param  obj: 控件名
  * @example TJC_HMI_Hide("t0")
  *          生成: vis t0,0
  * 
  * 【学习笔记】
  * 隐藏一个控件
  * 
  * 使用示例：
  * TJC_HMI_Hide("t0");  // 隐藏 t0 控件
  */
void TJC_HMI_Hide(const char *obj)
{
    if (obj == NULL) return;

    snprintf(s_tx_buf, sizeof(s_tx_buf), "vis %s,0", obj);
    TJC_HMI_SendCmd(s_tx_buf);
}

/**
  * @brief  清空文本控件
  * @param  obj: 控件名
  * 
  * 【学习笔记】
  * 等价于 TJC_HMI_SetText(obj, "")
  * 
  * 使用示例：
  * TJC_HMI_ClearText("t0");  // 清空 t0 控件
  */
void TJC_HMI_ClearText(const char *obj)
{
    TJC_HMI_SetText(obj, "");
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  系统指令接口
 * ═══════════════════════════════════════════════════════════════════════════ */

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
void TJC_HMI_SetBkcmd(uint8_t mode)
{
    snprintf(s_tx_buf, sizeof(s_tx_buf), "bkcmd=%u", mode);
    TJC_HMI_SendCmd(s_tx_buf);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * ██  调试接口
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifdef TJC_HMI_DEBUG_RX

/**
  * @brief  获取最近接收的字节（调试用）
  * @param  out: 输出缓冲区
  * @param  len: 要获取的字节数
  * 
  * 【学习笔记】
  * 调试函数，需要先在 tjc_hmi_config.h 中启用 TJC_HMI_DEBUG_RX
  * 可以查看最近接收到的原始字节，用于排查问题
  * 
  * 使用示例：
  * uint8_t buf[4];
  * TJC_HMI_GetDebugBytes(buf, 4);
  * printf("最近4个字节: %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3]);
  */
void TJC_HMI_GetDebugBytes(uint8_t *out, uint8_t len)
{
    if (out == NULL || len == 0) return;

    for (uint8_t i = 0; i < len; i++) {
        out[i] = s_debug_buf[(s_debug_idx - len + i) & (TJC_HMI_DEBUG_BUF_SIZE - 1)];
    }
}

#endif /* TJC_HMI_DEBUG_RX */
