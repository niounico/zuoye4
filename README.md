# 电控组第四次培训作业

龚宏伟

STM32F103C8T6 通过 TIM2_CH1 控制 SG90 舵机，依次发出 0°、45°、90°、135°、180° 的目标指令。OLED 和 USART1 同步输出目标角度、CCR 和高电平脉宽。每步更新后约等待 1 秒，等待期间主循环读取串口接收缓冲区并回显。

## TIM2 参数

板载 8 MHz 外部晶振经过 PLL ×9 得到 72 MHz 系统时钟。APB1 二分频为 36 MHz，TIM2 的时钟为 36 MHz ×2 = 72 MHz。

| 参数 | 取值 | 推导 |
| --- | --- | --- |
| TIM2 输入时钟 | 72 MHz | APB1 为 36 MHz，定时器时钟 ×2 |
| PSC | 71 | 72 MHz ÷ (71 + 1) = 1 MHz，每计数一次为 1 µs |
| ARR | 19999 | 1 MHz ÷ (19999 + 1) = 50 Hz，PWM 周期为 20 ms |
| 90° 的 CCR | 1500 | 500 + 90 ÷ 180 × 2000 = 1500 µs |

## 舵机两端为什么这样取

按作业的线性关系，0° 对应 500 µs，180° 对应 2500 µs。程序把参与换算的角度限制为 2°～178°，给两端各留 2° 的机械余量，降低舵机顶到限位后持续抖动、发热的风险。整数截断后，两个端点对应的 CCR 分别为 522 和 2477。屏幕和串口显示的是原始目标角度，以及实际写入的 CCR 和脉宽。

## 接线

| 外设 | STM32 接口 |
| --- | --- |
| SG90 信号 | PA0（TIM2_CH1） |
| OLED SCL / SDA | PB8 / PB9（I2C1，地址 0x3C） |
| USB 转串口 RX / TX | PA9 / PA10（USART1） |

舵机使用符合其额定电压的外部电源，电源地、STM32 和串口模块共地。

## 编译

使用 CMake 4.3 或更新版本、Ninja 和 Arm GNU Toolchain。把 Arm GNU Toolchain 的 bin 目录加入 PATH，在仓库根目录执行：

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

构建产物为 build/zz4.elf、build/zz4.hex 和 build/zz4.bin。也可以在 CLion 中打开工程根目录，用 OpenOCD 和 ST-Link 烧录。

## 串口运行结果

USART1 配置为 115200、8 数据位、无校验、1 停止位，接收使用文本模式。每条记录包含 Target、CCR 和 Pulse。90° 时预期输出如下：

```text
Target= 90  CCR=1500  Pulse=1.500ms
```

![串口助手接收区显示五个目标角度及各自的 CCR 与脉宽](docs/uart-output.jpg)

[串口实测记录](docs/README.md)
