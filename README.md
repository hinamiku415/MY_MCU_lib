STM32 高性能非阻塞多功能按键驱动库
这是一个基于 STM32F4 标准库开发的按键驱动模块。它采用了有限状态机 (FSM) 架构与数据驱动 (Data-Driven) 的编程思想，实现了对物理按键的深度解耦与高效管理。
🚀 核心特性
⚡ 异步非阻塞设计：完全弃用 delay_ms()，利用系统滴答定时器（SysTick）进行毫秒级异步计时，不占用 CPU 死等资源。
🛠 多功能集成：单个按键支持 单击 (Single Click)、双击 (Double Click) 及 长按 (Long Press) 三种触发模式。
🧩 面向对象思维：通过结构体封装按键的物理属性（GPIO/Pin）与逻辑状态，实现硬件与算法的完全分离。
📈 极易扩展：采用配置数组模式，增加新按键只需在数组中添加一行代码，无需修改核心逻辑。
🛡 稳健消抖：内置软件二阶消抖逻辑，有效过滤机械按键产生的物理噪声。
🏗 状态机架构
本驱动通过五个核心状态维护按键的生命周期：
KEY_RELEASE: 初始松开状态，监听电平变化。
KEY_CONFIRM: 物理消抖阶段，确认按下有效性。
KEY_SHORTPRESS: 识别窗口期，区分“即将长按”还是“准备松手”。
KEY_WAIT_DOUBLE: 关键“观察期”，等待 250ms 内是否有第二次按下（判定双击）。
KEY_LONGPRESS: 长按锁定状态，防止重复触发。
💻 快速上手
1. 硬件配置
在 key.c 的配置数组中填入你的按键信息：
code
C
static Key_GPIO_t key_gpio_List[] = {
    {RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_PuPd_DOWN, 1}, // KEY1: PA0, 高电平触发
    {RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_3, GPIO_PuPd_UP,   0}, // KEY2: PE3, 低电平触发
};
2. 系统时钟初始化
确保你的 main.c 中开启了 1ms 的系统滴答中断：
code
C
// 建议放在初始化列表的最末尾，防止与 delay_init 冲突
SysTick_Config(SystemCoreClock / 1000);
3. 主循环调用
在业务层直接获取键值并处理：
code
C
while (1) {
    uint8_t key = GetKeyVal();
    switch(key) {
        case 0x01: // KEY1 单击
            LED1_Toggle(); break;
        case 0x41: // KEY1 双击
            LED2_Toggle(); break;
        case 0x81: // KEY1 长按
            Beep_On(); break;
    }
}
📊 性能参数
参数项	默认值	说明
消抖时间	20 ms	滤除机械抖动
长按判定	1000 ms	持续按住 1 秒触发
双击窗口	250 ms	两次连击的最大间隔
内存占用	极低	仅消耗少量的 RAM 存储结构体状态
📝 开发感悟
本项目在开发过程中深入探讨了 STM32 时钟树配置、SysTick 中断优先级以及 C 语言中的抽象化设计。通过将“时间轴判断”引入驱动层，成功解决了传统按键扫描导致的系统卡顿问题。
欢迎提交 Issue 或 Pull Request 共同优化！
