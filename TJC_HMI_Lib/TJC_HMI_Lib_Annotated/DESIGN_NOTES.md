# TJC_HMI_Lib 值得学习的设计思想与模块

## 目录

1. [分层架构设计](#1-分层架构设计)
2. [回调函数机制](#2-回调函数机制)
3. [状态机设计](#3-状态机设计)
4. [接口抽象（依赖倒置）](#4-接口抽象依赖倒置)
5. [配置分离](#5-配置分离)
6. [缓冲区管理](#6-缓冲区管理)
7. [ASCII 转数字算法](#7-ascii-转数字算法)
8. [防溢出设计](#8-防溢出设计)
9. [函数指针的妙用](#9-函数指针的妙用)
10. [volatile 关键字](#10-volatile-关键字)

---

## 1. 分层架构设计

### 设计思想

把系统分成多个层次，每层只关心自己的职责，层与层之间通过接口通信。

### 代码实现

```
┌─────────────────────────────────────────────┐
│  应用层（你的代码）                           │  ← 定义 ID 含义、处理业务
│  - target_D = event->value;                 │
├─────────────────────────────────────────────┤
│  中间层（tjc_hmi.c）                         │  ← 协议解析、生成事件
│  - 解析 A5...5A 帧                          │
│  - _TriggerEvent(&event);                   │
├─────────────────────────────────────────────┤
│  底层（tjc_hmi_port.c）                      │  ← 硬件操作、收发字节
│  - USART_SendData();                        │
└─────────────────────────────────────────────┘
```

### 优点

- **解耦**：每层独立，修改一层不影响其他层
- **可移植**：换 MCU 只改底层，中间层和应用层不用动
- **可测试**：每层可以单独测试

### 学习要点

```c
// 中间层不直接调用硬件，通过接口调用
void TJC_HMI_SendCmd(const char *cmd)
{
    TJC_Port_SendBuffer((const uint8_t *)cmd, len);  // 调用底层接口
    TJC_HMI_SendEnd();
}
```

---

## 2. 回调函数机制

### 设计思想

库在特定时机"通知"用户，用户在自己的函数中处理业务。这是**事件驱动**的核心。

### 类比理解

```
你（应用层）：告诉快递柜"有快递打电话给我" → RegisterCallback(MyHandler)
快递柜（中间层）：收到快递 → 解析信息 → 打电话给你 → _TriggerEvent()
你：接到电话，去取快递 → MyHandler() 执行
```

### 代码实现

```c
// 第1步：定义回调函数类型
typedef void (*TJC_HMI_Callback_t)(const TJC_Event_t *event);

// 第2步：保存函数指针
static TJC_HMI_Callback_t s_callback = NULL;

// 第3步：注册回调
void TJC_HMI_RegisterCallback(TJC_HMI_Callback_t callback)
{
    s_callback = callback;
}

// 第4步：触发回调
static void _TriggerEvent(const TJC_Event_t *event)
{
    if (s_callback != NULL) {
        s_callback(event);  // 调用用户的函数
    }
}

// 第5步：用户的回调函数
void MyHandler(const TJC_Event_t *event)
{
    target_D = event->value;
}

// 第6步：注册
TJC_HMI_RegisterCallback(MyHandler);
```

### 执行流程

```
屏幕发数据 → 中断 → 库解析 → _TriggerEvent() → MyHandler() → 你的代码
```

### 学习要点

- 函数指针可以实现"反向调用"
- 库不需要知道用户怎么处理数据，只负责通知
- 这是 Linux 内核、驱动框架的常用模式

---

## 3. 状态机设计

### 设计思想

用状态机解析字节流，每个字节到来时根据当前状态决定如何处理。

### 状态转换图

```
IDLE ──────┬── 收到 0xA5 ──→ DATA
           └── 收到 0x65 ──→ SKIP_NEXTION

DATA ──────┬── 收到 0x5A ──→ 解析帧 → IDLE
           ├── 收到其他 ──→ 存入缓冲区
           └── 缓冲区满 ──→ 丢弃 → IDLE

SKIP_NEXTION ── 收到 3 个 0xFF ──→ IDLE
```

### 代码实现

```c
void TJC_HMI_RxByte(uint8_t byte)
{
    // 状态1：空闲状态
    if (byte == 0xA5 && s_rx_state == RX_STATE_IDLE) {
        s_rx_state = RX_STATE_DATA;  // 切换到接收状态
        s_rx_pos = 0;
        return;
    }

    // 状态2：接收数据状态
    if (s_rx_state == RX_STATE_DATA) {
        if (byte == 0x5A) {
            _ParseGenericFrame();    // 解析帧
            s_rx_state = RX_STATE_IDLE;  // 回到空闲
        } else {
            s_rx_buf[s_rx_pos++] = byte;  // 存入缓冲区
        }
        return;
    }
}
```

### 学习要点

- 状态机是解析协议的利器
- 每个状态只处理自己关心的字节
- 状态转换要清晰，避免死循环

---

## 4. 接口抽象（依赖倒置）

### 设计思想

高层模块不依赖低层模块，两者都依赖抽象。库不直接调用硬件，通过函数指针调用。

### 代码实现

```c
// 接口定义（tjc_hmi_port.h）
void TJC_Port_SendByte(uint8_t byte);

// 库调用接口（tjc_hmi.c）
void TJC_HMI_SendCmd(const char *cmd)
{
    TJC_Port_SendBuffer((const uint8_t *)cmd, len);  // 调用接口
}

// 用户实现接口（tjc_hmi_port_stm32f10x.c）
void TJC_Port_SendByte(uint8_t byte)
{
    USART_SendData(USART1, byte);  // 具体硬件操作
}
```

### 优点

- 换 MCU 只需要实现接口，库代码不用改
- 可以在不同平台复用库

### 学习要点

- 接口是"契约"，实现是"细节"
- 依赖抽象，不依赖具体实现
- 这是面向对象设计的核心原则

---

## 5. 配置分离

### 设计思想

把配置项集中到一个文件，用户按需修改，不用改库代码。

### 代码实现

```c
// tjc_hmi_config.h（用户配置）
#define TJC_HMI_RX_BUF_SIZE    64      // 接收缓冲区大小
#define TJC_HMI_TX_BUF_SIZE    96      // 发送缓冲区大小
#define TJC_HMI_FRAME_HEAD     0xA5    // 帧头
#define TJC_HMI_FRAME_TAIL     0x5A    // 帧尾

// tjc_hmi.c（使用配置）
static uint8_t s_rx_buf[TJC_HMI_RX_BUF_SIZE];
static char s_tx_buf[TJC_HMI_TX_BUF_SIZE];
```

### 优点

- 配置集中，一目了然
- 用户不用改库代码
- 可以按需裁剪功能

### 学习要点

- 配置和代码分离
- 用宏定义控制功能开关
- 这是嵌入式项目的最佳实践

---

## 6. 缓冲区管理

### 设计思想

用数组做缓冲区，用指针或索引管理读写位置。

### 代码实现

```c
// 接收缓冲区
static uint8_t s_rx_buf[TJC_HMI_RX_BUF_SIZE];  // 缓冲区
static uint8_t s_rx_pos = 0;                     // 当前位置

// 写入缓冲区
if (s_rx_pos < TJC_HMI_RX_BUF_SIZE - 1) {
    s_rx_buf[s_rx_pos++] = byte;  // 存入，位置+1
} else {
    s_rx_pos = 0;  // 溢出，重置
}

// 发送缓冲区
static char s_tx_buf[TJC_HMI_TX_BUF_SIZE];
snprintf(s_tx_buf, sizeof(s_tx_buf), "%s.txt=\"%s\"", obj, text);
```

### 学习要点

- 缓冲区大小要留余量
- 检查边界，防止溢出
- 用 sizeof 获取实际大小

---

## 7. ASCII 转数字算法

### 设计思想

把 ASCII 字符串转成整数，这是嵌入式常用算法。

### 代码实现

```c
uint16_t value = 0;
for (uint8_t i = 2; i < s_rx_pos - 1; i++) {
    if (s_rx_buf[i] >= '0' && s_rx_buf[i] <= '9') {
        value = value * 10 + (s_rx_buf[i] - '0');
    }
}
```

### 算法推导

以 "150" 为例：

```
第1步：value = 0 * 10 + ('1' - '0') = 0 + 1 = 1
第2步：value = 1 * 10 + ('5' - '0') = 10 + 5 = 15
第3步：value = 15 * 10 + ('0' - '0') = 150 + 0 = 150
```

### 核心公式

```
value = value * 10 + (字符 - '0')
```

### 学习要点

- `字符 - '0'` 把 ASCII 转成数字
- `value * 10` 把已有数字左移一位
- 这是 C 语言最经典的算法之一

---

## 8. 防溢出设计

### 设计思想

任何写入操作都要检查边界，防止缓冲区溢出。

### 代码实现

```c
// 写入缓冲区前检查
if (s_rx_pos < TJC_HMI_RX_BUF_SIZE - 1) {
    s_rx_buf[s_rx_pos++] = byte;
} else {
    s_rx_pos = 0;  // 溢出，重置
}

// 复制数据前检查
uint8_t raw_len = s_rx_pos;
if (raw_len >= sizeof(event.raw)) {
    raw_len = sizeof(event.raw) - 1;  // 截断
}
memcpy(event.raw, s_rx_buf, raw_len);

// 检查指针
if (cmd == NULL) return;
if (obj == NULL || text == NULL) return;
```

### 学习要点

- 永远检查边界
- 用 `sizeof` 获取实际大小
- 空指针检查是必须的

---

## 9. 函数指针的妙用

### 设计思想

函数指针可以实现"回调"、"策略模式"、"接口抽象"等高级功能。

### 代码实现

```c
// 定义函数指针类型
typedef void (*TJC_HMI_Callback_t)(const TJC_Event_t *event);

// 保存函数指针
static TJC_HMI_Callback_t s_callback = NULL;

// 注册函数
void TJC_HMI_RegisterCallback(TJC_HMI_Callback_t callback)
{
    s_callback = callback;
}

// 调用函数指针
static void _TriggerEvent(const TJC_Event_t *event)
{
    if (s_callback != NULL) {
        s_callback(event);  // 调用用户的函数
    }
}
```

### 应用场景

- **回调函数**：库通知用户
- **策略模式**：运行时切换算法
- **接口抽象**：底层实现可替换

### 学习要点

- 函数指针是 C 语言的高级特性
- 可以实现面向对象的多态
- Linux 内核大量使用函数指针

---

## 10. volatile 关键字

### 设计思想

告诉编译器这个变量可能被意外修改，不要优化对它的访问。

### 代码实现

```c
// 中断和主循环共享的变量必须加 volatile
extern volatile uint32_t g_tick;  // SysTick 中断中递增

// 中断中修改
void SysTick_Handler(void)
{
    g_tick++;  // 中断中修改
}

// 主循环中读取
uint32_t TJC_Port_GetTick(void)
{
    return g_tick;  // 主循环中读取
}
```

### 为什么需要

```c
// 没有 volatile，编译器可能优化成：
uint32_t temp = g_tick;  // 读一次
while (temp == g_tick);   // 永远循环，因为用的是缓存值

// 有 volatile，每次都从内存读取：
while (g_tick == old_value);  // 正确等待
```

### 学习要点

- 中断共享变量必须加 volatile
- 防止编译器优化
- 这是嵌入式编程的基本功

---

## 总结

| 设计思想 | 应用场景 | 核心要点 |
|---------|---------|---------|
| 分层架构 | 整体设计 | 解耦、可移植 |
| 回调函数 | 事件通知 | 函数指针、事件驱动 |
| 状态机 | 协议解析 | 状态转换、字节处理 |
| 接口抽象 | 硬件无关 | 依赖倒置、可替换 |
| 配置分离 | 用户配置 | 宏定义、集中管理 |
| 缓冲区管理 | 数据存储 | 边界检查、防溢出 |
| ASCII 转数字 | 数据解析 | 经典算法 |
| 防溢出设计 | 安全编程 | 边界检查、空指针 |
| 函数指针 | 高级特性 | 回调、策略、接口 |
| volatile | 中断编程 | 防止优化、内存可见 |

这些设计思想不仅适用于这个库，也是嵌入式编程的通用技巧，值得深入理解和掌握。
