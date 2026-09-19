# TAS3251_EVM — 基于 PY32F002B 的 TAS3251 功放控制库

> TAS3251 数字输入 D 类功放评估板 · 板载 PY32F002B 通过 I2C 控制
> **只需简单填写几个 `#define` 参数，即可自动计算配置并写入功放**

## 简介

本项目是 TAS3251 评估板及其配套简易驱动库。PY32F002B 作为主控，通过 I2C
对 TAS3251 进行配置与监控。

传统方式配置 TAS3251 需要翻阅数据手册、手工计算寄存器值（尤其是繁琐的
TDM 时钟偏移）。本库将这些全部自动化——**修改宏定义，剩下的交给库**。

## 功能特性

- ✅ 宏定义式配置，自动计算寄存器参数并写入
- ✅ 支持 I2S / 左对齐 / 右对齐格式，多种数据位宽
- ✅ 支持 TDM 模式，**自动计算 TDM 时钟偏移值（slot offset）**
- ✅ 左右声道数据交换（Swap L/R）开关
- ✅ dB 单位的数字音量控制
- ✅ 功放故障检测 + 电源电压监测 + 智能 mute 时序，有效消除 pop 音
- ✅ 代码轻量，适合资源有限的 Cortex-M0+ MCU

## 硬件

| 器件     | 说明                                       |
| -------- | ------------------------------------------ |
| 功放     | TI **TAS3251**，数字输入立体声 D 类功放    |
| 主控     | Puya **PY32F002B**（ARM Cortex-M0+）       |
| 控制接口 | I2C                                        |

<!-- 建议在此处插入板子照片或系统框图
![board](docs/board.png)
-->

## 快速上手

### 1️⃣ 修改配置

在头文件中根据实际应用填写参数：

```c
#define SwapDataLR        false               // 左右声道交换
#define Input_Format      Format_I2S          // 音频格式
#define Channel_Length    Channel_32bit       // 时隙位宽
#define Input_Word_Length Word_Lengh_24bit    // 有效数据位宽

#define TDM_MODE          TDM_MODE_8          // TDM 通道数
#define TDM_ROLE          TDM_ROLE_3          // 通道角色，库据此自动计算偏移
```

| 宏                  | 说明                                             |
| ------------------- | ------------------------------------------------ |
| `SwapDataLR`        | `true` / `false`，交换左右声道输入               |
| `Input_Format`      | 音频数据格式（I2S、左对齐、右对齐等）            |
| `Channel_Length`    | 时隙（slot）位宽                                 |
| `Input_Word_Length` | 有效数据位宽                                     |
| `TDM_MODE`          | TDM 模式通道数（2/4/6/8），标准 I2S 模式可忽略   |
| `TDM_ROLE`          | 本芯片占用的通道角色，自动计算 TDM 时钟偏移值    |

> 具体可选枚举值以头文件 `tas3251.h` 中的定义和注释为准。

### 2️⃣ 初始化功放

```c
TAS3251_Init();   // 初始化DSP → 配置音频格式 → 设置音量 → 启动功放
```

### 3️⃣ 主循环中运行保护

```c
while (1)
{
    AMP_Fault_Handler();
}
```

## API 说明

| 函数                            | 说明                                     |
| ------------------------------- | ---------------------------------------- |
| `void TAS3251_Init(void)`       | 初始化功放 DSP，配置音频格式，设置通道音量，启动功放 |
| `void TAS3251_Set_Vol_dB(int8_t Volume)` | 以 **dB** 为单位设置数字音量    |
| `void AMP_Fault_Handler(void)`  | 主循环中调用：检测功放状态、监测电源电压，在恰当时机 mute，防止 pop 音 |

## 完整使用示例

```c
int main(void)
{
    /* 时钟 / GPIO / I2C 初始化 ... */

    TAS3251_Init();             // 初始化并启动功放
    TAS3251_Set_Vol_dB(-20);    // 设置 -20dB 音量

    while (1)
    {
        AMP_Fault_Handler();    // 故障检测与防爆音处理
    }
}
```

## 目录结构

```
TAS3251_EVM/
├── Core/                     # 用户代码及驱动库（按实际工程调整）
├── MDK-ARM/                  # Keil MDK 工程
│   └── TAS3251_EVM.uvprojx
└── ...
```

## 移植说明

- 库通过 I2C 与 TAS3251 通信，移植到其他平台只需对接自己的 I2C 读写接口
- mute / 功放使能控制脚需映射到对应 GPIO
- 当前基于 PY32F002B 标准库实现

## TODO

- [ ] 支持更多 DSP 高级功能（EQ、DRC 等）
- [ ] 英文文档

---

如果这个项目对你有帮助，欢迎 Star ⭐


# TAS3251_EVM — TAS3251 Amplifier Control Library based on PY32F002B

> TAS3251 digital-input Class-D amplifier evaluation board · On-board PY32F002B
> controls it via I2C. **Just fill in a few `#define` parameters — the library
> calculates the register values and writes them automatically.**

## Introduction

This project is a TAS3251 evaluation board together with its lightweight driver
library. The PY32F002B acts as the master controller and configures/monitors
the TAS3251 over I2C.

Configuring the TAS3251 the traditional way means digging through the
datasheet and hand-calculating register values (especially the tedious TDM
slot offsets). This library automates all of it —
**change the macros, and let the library do the rest.**

## Features

- ✅ Macro-based configuration — register values are calculated and written automatically
- ✅ Supports I2S / Left-Justified / Right-Justified formats with various word lengths
- ✅ TDM mode support with **automatic TDM slot-offset calculation**
- ✅ Left/Right channel swap (Swap L/R) switch
- ✅ Digital volume control in dB
- ✅ Amplifier fault detection + supply voltage monitoring + smart mute timing for pop-noise suppression
- ✅ Lightweight code, suitable for resource-constrained Cortex-M0+ MCUs

## Hardware

| Component   | Description                                            |
| ----------- | ------------------------------------------------------ |
| Amplifier   | TI **TAS3251** — digital-input stereo Class-D amplifier |
| MCU         | Puya **PY32F002B** (ARM Cortex-M0+)                     |
| Control bus | I2C                                                     |

<!-- Insert a board photo or system diagram here
![board](docs/board.png)
-->

## Quick Start

### 1️⃣ Edit the configuration

Fill in the parameters in the header file according to your application:

```c
#define SwapDataLR        false               // Swap left/right channels
#define Input_Format      Format_I2S          // Audio data format
#define Channel_Length    Channel_32bit       // Slot width
#define Input_Word_Length Word_Lengh_24bit    // Effective data width

#define TDM_MODE          TDM_MODE_8          // Number of TDM slots
#define TDM_ROLE          TDM_ROLE_3          // Slot role; offset is calculated automatically
```

| Macro                | Description                                                      |
| -------------------- | ---------------------------------------------------------------- |
| `SwapDataLR`         | `true` / `false` — swap the left and right input channels        |
| `Input_Format`       | Audio data format (I2S, left-justified, right-justified, etc.)   |
| `Channel_Length`     | Slot width                                                       |
| `Input_Word_Length`  | Effective data width                                             |
| `TDM_MODE`           | Number of TDM slots (2/4/6/8); ignored in standard I2S mode      |
| `TDM_ROLE`           | Slot role of this chip; the TDM offset is computed automatically |

> See the definitions and comments in `tas3251.h` for all available enum values.

### 2️⃣ Initialize the amplifier

```c
TAS3251_Init();   // Init DSP → configure audio format → set volume → power up
```

### 3️⃣ Run the protection handler in the main loop

```c
while (1)
{
    AMP_Fault_Handler();
}
```

## API Reference

| Function                                 | Description                                              |
| ---------------------------------------- | -------------------------------------------------------- |
| `void TAS3251_Init(void)`                | Initializes the DSP, configures the audio format, sets channel volume, and starts the amplifier |
| `void TAS3251_Set_Vol_dB(int8_t Volume)` | Sets the digital volume in **dB**                        |
| `void AMP_Fault_Handler(void)`           | Call in the main loop: detects amplifier faults, monitors the supply voltage, and mutes at the right moment to prevent pop noise |

## Full Usage Example

```c
int main(void)
{
    /* Clock / GPIO / I2C initialization ... */

    TAS3251_Init();             // Initialize and start the amplifier
    TAS3251_Set_Vol_dB(-20);    // Set volume to -20 dB

    while (1)
    {
        AMP_Fault_Handler();    // Fault detection & pop-noise suppression
    }
}
```

## Directory Structure

```
TAS3251_EVM/
├── Core/                     # User code and driver library (adjust to your project)
├── MDK-ARM/                  # Keil MDK project
│   └── TAS3251_EVM.uvprojx
└── ...
```

## Porting Notes

- The library communicates with the TAS3251 over I2C — to port to another
  platform, simply hook up your own I2C read/write functions
- The mute / amplifier-enable pins need to be mapped to the corresponding GPIOs
- Currently implemented on the PY32F002B standard peripheral library

## TODO

- [ ] Support more advanced DSP features (EQ, DRC, etc.)
- [ ] Chinese documentation

---

If this project helps you, please give it a Star ⭐
