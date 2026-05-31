# TJC_HMI_Lib_Annotated - 带注释的学习版本

这是 TJC_HMI_Lib 的带注释版本，专门为学习设计。

## 目录结构

```
TJC_HMI_Lib_Annotated/
├── Core/
│   ├── tjc_hmi.c           # 核心逻辑（带详细注释）
│   ├── tjc_hmi.h           # 对外接口（带详细注释）
│   ├── tjc_hmi_config.h    # 用户配置（带详细注释）
│   ├── tjc_hmi_port.h      # 平台适配接口（带详细注释）
│   └── tjc_hmi_types.h     # 数据类型（带详细注释）
├── Port/
│   ├── tjc_hmi_port_stm32f10x.c    # STM32F103 适配（带详细注释）
│   └── tjc_hmi_port_stm32f10x.h    # STM32F103 头文件（带详细注释）
└── README.md
```

## 学习建议

### 第1步：理解分层架构

```
应用层（你的代码）     → 定义 ID 含义、处理业务
    ↓ 调用
中间层（tjc_hmi.c）   → 协议解析、生成事件
    ↓ 调用
底层（tjc_hmi_port.c）→ 硬件操作、收发字节
```

### 第2步：理解回调函数

```c
// 1. 你写回调函数
void MyHandler(const TJC_Event_t *event)
{
    target_D = event->value;
}

// 2. 注册回调
TJC_HMI_RegisterCallback(MyHandler);

// 3. 库有数据时自动调用你的函数
```

### 第3步：理解接收状态机

```
IDLE → 收到 0xA5 → DATA 状态
DATA → 收到 0x5A → 解析帧 → 回到 IDLE
```

### 第4步：理解发送流程

```
SetText("D", "120.5cm")
    ↓
拼接指令：D.txt="120.5cm"
    ↓
追加结束符：FF FF FF
    ↓
SendByte() 逐个发送
```

## 关键代码说明

### tjc_hmi.c 核心函数

| 函数 | 作用 |
|------|------|
| `_TriggerEvent()` | 触发回调，把事件传给用户 |
| `_ParseGenericFrame()` | 解析 A5...5A 帧 |
| `TJC_HMI_RxByte()` | 接收状态机，处理字节流 |
| `TJC_HMI_SetText()` | 设置文本控件 |
| `TJC_HMI_Init()` | 初始化库 |

### tjc_hmi_port_stm32f10x.c 硬件操作

| 函数 | 作用 |
|------|------|
| `TJC_Port_Init()` | 初始化 USART1 |
| `TJC_Port_SendByte()` | 发送一个字节 |
| `TJC_HMI_USART_IRQHandler()` | 中断处理 |

## 学习顺序

1. 先看 `tjc_hmi_types.h` - 理解数据类型
2. 再看 `tjc_hmi_config.h` - 理解配置项
3. 然后看 `tjc_hmi_port.h` - 理解接口定义
4. 接着看 `tjc_hmi_port_stm32f10x.c` - 理解硬件操作
5. 最后看 `tjc_hmi.c` - 理解核心逻辑

## 注意事项

1. 注释以 `【学习笔记】` 开头，方便搜索
2. 每个函数都有详细的使用示例
3. 关键代码都有逐行解释
4. 状态机有完整的流程说明
