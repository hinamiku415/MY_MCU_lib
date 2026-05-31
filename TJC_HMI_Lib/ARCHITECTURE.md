# TJC_HMI_Lib 详细架构图

---

## 1. 库整体架构（三层结构）

```mermaid
flowchart TB
    subgraph APP["应用层 (用户代码)"]
        direction TB
        A1["定义 ID 含义<br/>#define MY_ID_DISTANCE 1"]
        A2["写回调函数<br/>void MyHandler(const TJC_Event_t *event)"]
        A3["注册回调<br/>TJC_HMI_RegisterCallback(MyHandler)"]
        A4["处理业务<br/>target_D = event->value"]
    end

    subgraph CORE["中间层 (tjc_hmi.c)"]
        direction TB
        C1["发送函数<br/>SetText / SetNumber / Page / Show / Hide"]
        C2["接收函数<br/>RxByte / _ParseGenericFrame / _TriggerEvent"]
        C3["初始化函数<br/>Init / RegisterCallback / SetBkcmd"]
        C4["内部变量<br/>s_callback / s_rx_state / s_rx_buf / s_tx_buf"]
    end

    subgraph PORT["底层 (tjc_hmi_port.c)"]
        direction TB
        P1["Port_Init()<br/>初始化 USART1"]
        P2["Port_SendByte()<br/>发送一个字节"]
        P3["Port_SendBuffer()<br/>发送缓冲区"]
        P4["USART1_IRQHandler()<br/>中断处理"]
    end

    subgraph SCREEN["串口屏"]
        direction TB
        S1["显示控件<br/>t0.txt / D.txt"]
        S2["按钮/输入框"]
    end

    APP --> CORE
    CORE --> PORT
    PORT --> SCREEN
    SCREEN --> PORT
    PORT --> CORE
    CORE --> APP

    style APP fill:#e1f5fe,stroke:#01579b,stroke-width:3px
    style CORE fill:#fff3e0,stroke:#e65100,stroke-width:3px
    style PORT fill:#e8f5e8,stroke:#1b5e20,stroke-width:3px
    style SCREEN fill:#fce4ec,stroke:#880e4f,stroke-width:3px
```

---

## 2. 接收状态机详解

```mermaid
flowchart TB
    IDLE["IDLE = 0<br/>空闲状态<br/>等待帧头"]
    DATA["DATA = 1<br/>接收数据状态<br/>存入 s_rx_buf"]
    SKIP["SKIP = 2<br/>跳过Nextion包<br/>计数 0xFF"]
    PARSE["解析帧<br/>_ParseGenericFrame()<br/>触发回调"]

    IDLE -->|"收到 0xA5 (帧头)"| DATA
    IDLE -->|"收到 0x65"| SKIP
    IDLE -->|"其他字节/0xFF 忽略"| IDLE

    DATA -->|"收到数据字节"| DATA
    DATA -->|"收到 0x5A (帧尾)"| PARSE
    DATA -->|"缓冲区溢出 丢弃"| IDLE

    SKIP -->|"收到非0xFF 计数清零"| SKIP
    SKIP -->|"收到0xFF 计数+1"| SKIP
    SKIP -->|"3个连续0xFF"| IDLE

    PARSE --> IDLE

    linkStyle 0 stroke:#1976d2,stroke-width:3px
    linkStyle 1 stroke:#7b1fa2,stroke-width:3px
    linkStyle 2 stroke:#9e9e9e,stroke-width:2px,stroke-dasharray:5

    linkStyle 3 stroke:#e65100,stroke-width:3px
    linkStyle 4 stroke:#1b5e20,stroke-width:3px
    linkStyle 5 stroke:#b71c1c,stroke-width:2px,stroke-dasharray:5

    linkStyle 6 stroke:#7b1fa2,stroke-width:2px
    linkStyle 7 stroke:#7b1fa2,stroke-width:2px
    linkStyle 8 stroke:#1b5e20,stroke-width:3px

    linkStyle 9 stroke:#1b5e20,stroke-width:2px
```

---

## 3. 帧格式与解析详解

```mermaid
flowchart LR
    subgraph FRAME["通用帧格式"]
        direction LR
        F1["0xA5<br/>帧头"]
        F2["ID<br/>1字节<br/>0x00~0xFF"]
        F3["DATA<br/>0~N字节<br/>ASCII字符"]
        F4["0x5A<br/>帧尾"]
    end
```

```mermaid
flowchart TB
    subgraph EX1["示例1: 按钮事件 (无数据)"]
        direction LR
        E1["A5"] --> E2["01"] --> E3["5A"]
    end

    subgraph EX2["示例2: 数据事件 (有数据)"]
        direction LR
        E4["A5"] --> E5["01"] --> E6["31 35 30"] --> E7["5A"]
    end

    subgraph R1["解析结果1"]
        R1A["event.type = BUTTON"]
        R1B["event.id = 1"]
        R1C["event.value = 0 (无效)"]
    end

    subgraph R2["解析结果2"]
        R2A["event.type = DATA"]
        R2B["event.id = 1"]
        R2C["event.value = 150"]
        R2D["event.text = '150'"]
    end

    EX1 --> R1
    EX2 --> R2
```

---

## 4. 事件结构体详解

```mermaid
flowchart TB
    subgraph EVENT["TJC_Event_t 事件结构体"]
        direction TB
        E1["type: TJC_EventType_t<br/>事件类型枚举"]
        E2["id: uint8_t<br/>事件ID (用户自定义含义)"]
        E3["value: uint16_t<br/>数值 (DATA事件时有效)<br/>从ASCII转换: '1','5','0' → 150"]
        E4["text[32]: char<br/>文本 (DATA事件时有效)<br/>原始ASCII字符串"]
        E5["raw[32]: uint8_t<br/>原始帧数据 (调试用)"]
        E6["raw_len: uint8_t<br/>原始数据长度"]
    end

    subgraph TYPE["事件类型枚举"]
        direction TB
        T1["TJC_EVENT_NONE = 0<br/>无事件"]
        T2["TJC_EVENT_BUTTON = 1<br/>按钮按下<br/>帧: A5 ID 5A (无数据)"]
        T3["TJC_EVENT_DATA = 2<br/>数据输入<br/>帧: A5 ID DATA 5A"]
        T4["TJC_EVENT_ERROR = 3<br/>错误"]
    end

    E1 --> TYPE
```

---

## 5. 回调函数机制详解

```mermaid
flowchart TB
    subgraph REG["第1步: 注册回调"]
        direction LR
        R1["用户写函数<br/>void MyHandler(const TJC_Event_t *event)<br/>{ target_D = event->value; }"]
        R2["注册<br/>TJC_HMI_RegisterCallback(MyHandler)"]
        R3["保存指针<br/>s_callback = MyHandler"]
        R1 --> R2 --> R3
    end

    subgraph TRIGGER["第2步: 触发回调"]
        direction LR
        T1["解析帧<br/>_ParseGenericFrame()"]
        T2["填充事件<br/>event.id=1, event.value=150"]
        T3["触发<br/>_TriggerEvent(&event)"]
        T4["调用指针<br/>s_callback(event)"]
        T5["用户代码<br/>target_D = 150"]
        T1 --> T2 --> T3 --> T4 --> T5
    end

    REG --> TRIGGER
```

---

## 6. 发送流程详解

```mermaid
flowchart LR
    U1["SetText('D', '120.5cm')"]
    C1["snprintf 拼接<br/>D.txt='120.5cm'"]
    C2["SendCmd()"]
    P1["SendBuffer()<br/>逐个字节发送"]
    P2["SendEnd()<br/>FF FF FF"]
    S1["屏幕显示<br/>120.5cm"]

    U1 --> C1 --> C2 --> P1 --> P2 --> S1
```

---

## 7. 接收流程详解

```mermaid
flowchart RL
    S1["串口屏<br/>发送 A5 01 31 35 30 5A"]
    I1["USART1 中断<br/>读取字节"]
    SM1["状态机<br/>逐字节处理"]
    P1["_ParseGenericFrame()<br/>解析出 id=1, value=150"]
    CB1["_TriggerEvent()<br/>调用回调"]
    R1["MyHandler()<br/>target_D = 150"]

    S1 --> I1 --> SM1 --> P1 --> CB1 --> R1
```

---

## 8. 内部变量详解

```mermaid
flowchart TB
    subgraph VAR["内部变量 (static)"]
        direction TB
        V1["s_callback: TJC_HMI_Callback_t<br/>回调函数指针<br/>用户调用 RegisterCallback() 时保存"]
        V2["s_rx_state: TJC_RxState_t<br/>接收状态机状态<br/>IDLE(0) / DATA(1) / SKIP_NEXTION(2)"]
        V3["s_rx_buf[64]: uint8_t<br/>接收缓冲区<br/>存储收到的帧数据 (不含帧头帧尾)"]
        V4["s_rx_pos: uint8_t<br/>接收位置<br/>当前写入到 s_rx_buf 的位置"]
        V5["s_tx_buf[96]: char<br/>发送缓冲区<br/>snprintf 拼接指令用"]
        V6["s_nextion_ff_count: uint8_t<br/>Nextion包计数<br/>收到0x65后计数连续0xFF"]
    end

    subgraph STATE["状态机状态"]
        direction TB
        S1["RX_STATE_IDLE = 0<br/>空闲，等待帧头 0xA5"]
        S2["RX_STATE_DATA = 1<br/>接收数据中，等待帧尾 0x5A"]
        S3["RX_STATE_SKIP_NEXTION = 2<br/>跳过Nextion返回包，等待3个0xFF"]
    end

    V2 --> STATE
```

---

## 9. 函数调用关系

```mermaid
flowchart TD
    subgraph API["用户API"]
        A1["Init()"]
        A2["RegisterCallback()"]
        A3["SetText() / SetNumber() / Page()"]
        A4["RxByte()"]
    end

    subgraph INTERNAL["内部函数"]
        I1["SendCmd() → SendEnd()"]
        I2["_ParseGenericFrame()"]
        I3["_TriggerEvent()"]
    end

    subgraph PORT["底层接口"]
        P1["Port_Init()"]
        P2["Port_SendByte() / Port_SendBuffer()"]
        P3["Port_GetTick() / Lock() / Unlock()"]
    end

    A1 --> P1
    A3 --> I1 --> P2
    A4 --> I2 --> I3
```

---

## 10. 配置项详解 (tjc_hmi_config.h)

```mermaid
flowchart TB
    subgraph CONFIG["配置项"]
        direction TB
        C1["缓冲区大小<br/>TJC_HMI_RX_BUF_SIZE = 64<br/>TJC_HMI_TX_BUF_SIZE = 96"]
        C2["功能开关<br/>TJC_HMI_ENABLE_FLOAT = 1<br/>TJC_HMI_ENABLE_PRINTF_FMT = 1"]
        C3["协议配置<br/>TJC_HMI_FRAME_HEAD = 0xA5<br/>TJC_HMI_FRAME_TAIL = 0x5A<br/>TJC_HMI_CMD_END = 0xFF 0xFF 0xFF"]
        C4["Nextion配置<br/>TJC_HMI_NEXTION_TOUCH_HEAD = 0x65"]
        C5["调试配置<br/>TJC_HMI_DEBUG_RX (可选)<br/>TJC_HMI_DEBUG_BUF_SIZE = 16"]
    end
```
