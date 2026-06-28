# MNIST Digit Recognizer on STM32H750 via OV7670 + emlearn

A fully embedded ML system — an OV7670 camera captures a handwritten digit on paper, a lightweight MLP neural network runs inference directly on an STM32H750VBT6 microcontroller, and the predicted digit is sent to a PC terminal over UART. No OS, no external ML runtime, no host computation after boot.

<!-- ![System Overview](assets/system_overview.jpg) -->

---

## Repository Structure

```
ai_mnist_stm32_ov2640/
├── code/
│   ├── c/
│   │   └── ai_mnist_stm32_ov2640/         # STM32CubeIDE project
│   │       ├── Core/
│   │       │   ├── Inc/
│   │       │   │   ├── main.h
│   │       │   │   ├── ov7670.h            # OV7670 camera driver header
│   │       │   │   ├── mnist_784_8_10.h    # emlearn generated MLP model
│   │       │   │   ├── eml_net.h           # emlearn runtime headers
│   │       │   │   ├── eml_common.h
│   │       │   │   └── eml_net_common.h
│   │       │   └── Src/
│   │       │       ├── main.c              # capture + preprocess + inference + UART
│   │       │       └── ov7670.c            # OV7670 register init driver
│   │       ├── Drivers/
│   │       └── ai_mnist_stm32_ov2640.ioc   # CubeMX project file
│   │
│   └── python/
│       ├── train_mnist.py                  # MLP training + emlearn export
│       └── uart_viewer.py                  # PC live viewer + prediction display
│
└── README.md
```

---

## Hardware

| Component | Part | Notes |
|---|---|---|
| MCU | STM32H750VBT6 | DevEBox v2.0 board @ 480MHz |
| Camera | OV7670 | Pin-header format, manually wired |
| USB-UART | CH340N (HW-234) | 1Mbaud for live streaming |
| Pull-ups | 2× 4.7KΩ | On SDA and SCL lines |

---

## Wiring

| OV7670 Pin | STM32 Pin | Function |
|---|---|---|
| D0 | PC6 | DCMI data |
| D1 | PC7 | DCMI data |
| D2 | PC8 | DCMI data |
| D3 | PC9 | DCMI data |
| D4 | PE4 | DCMI data |
| D5 | PD3 | DCMI data |
| D6 | PE5 | DCMI data |
| D7 | PE6 | DCMI data |
| PCLK | PA6 | DCMI pixel clock |
| HREF | PA4 | DCMI HSYNC |
| VSYNC | PB7 | DCMI VSYNC |
| SIOC | PB6 | I2C1 SCL (SCCB) |
| SIOD | PB9 | I2C1 SDA (SCCB) |
| XCLK | PA8 | MCO1 12.5MHz clock |
| RESET | PC4 | GPIO output |
| PWDN | PA7 | GPIO output |
| VCC | 3.3V | |
| GND | GND | |

> **Note:** Pull-up resistors (4.7KΩ) are required between 3.3V and both PB6 (SCL) and PB9 (SDA). Without them I2C will not work.

> **Note:** DevEBox v2.0 has a non-standard 24-pin camera connector — do NOT use it with OV2640 flex cable modules. The pin 1 alignment is inverted and will permanently damage the camera. Use pin-header format cameras with manual wiring instead.

---

## How It Works

```
OV7670 streams YUV422 pixels via DCMI + DMA → frame_buf (38KB in RAM)
        ↓
Extract Y channel (odd bytes) → gray_buf (160x120 grayscale)
        ↓
Center crop 90x120 → adaptive threshold + contrast stretch + invert
        ↓
Downsample to 28x28 → small_buf (784 bytes)
        ↓
Normalize to float[784] → emlearn MLP forward pass
        ↓
Predicted digit 0-9 → UART TX → PC Python viewer
```

**Why invert?** MNIST has white digits on black background. Paper has black digits on white. Inversion is applied during preprocessing so the model sees what it was trained on.

**Why adaptive threshold?** A fixed threshold fails under different lighting. The mean brightness of the center region of the frame is computed per-frame and used as the threshold.

---

## ML Model

| Property | Value |
|---|---|
| Dataset | MNIST (60K train / 10K test) |
| Architecture | MLP `784 → 8 → 10` |
| Input | 28×28 grayscale (784 features) |
| Framework | scikit-learn MLPClassifier |
| Export | emlearn → pure C header |
| Activation | ReLU |
| Solver | SGD |
| Test Accuracy | ~92.8% |
| Model size | ~50 KB in flash |
| Total flash | ~72 KB / 128 KB |

The model is exported as a C header with no dynamic allocation — weights are `const float` arrays stored directly in flash. Inference is a matrix multiply + ReLU + argmax, nothing more.

---

## CubeMX Configuration

| Peripheral | Config | Purpose |
|---|---|---|
| RCC HSE | 25MHz crystal | Main clock source |
| PLL1 | M=5 N=192 P=2 | SYSCLK = 480MHz |
| MCO1 | HSE / 2 = 12.5MHz on PA8 | OV7670 XCLK |
| DCMI | 8-bit external sync, VSYNC high, HSYNC low, PCLK rising | Camera interface |
| DMA1 Stream0 | DCMI → Memory, word width | Frame transfer |
| I2C1 | Standard mode, PB6/PB9 | Camera register config (SCCB) |
| USART3 | 1Mbaud async, PB10/PB11 | Debug + result output |
| GPIO PA7 | Output PP | PWDN |
| GPIO PC4 | Output PP | RESET |

---

## Software Setup

### 1 — Train the model

```bash
pip install scikit-learn emlearn pyserial numpy matplotlib
python code/python/train_mnist.py
```

Copy the generated `mnist_784_8_10.h` to `Core/Inc/` in the CubeIDE project.

### 2 — Flash the firmware

Open `code/c/ai_mnist_stm32_ov2640/` in STM32CubeIDE, build and flash.

### 3 — Run the live viewer

```bash
python code/python/uart_viewer.py
```

The viewer shows two windows side by side:
- **Left:** 28×28 preprocessed input with predicted digit label
- **Right:** Raw 160×120 grayscale camera feed

---

## Camera Focus

The OV7670 has a manually adjustable lens. Rotate the lens barrel to focus. For best results:
- Place the digit on white paper **15–25cm** from the camera
- Use strong even lighting — avoid shadows
- Write digits large and centered in the frame
- The camera outputs 160×120, the center 90×120 region is used for inference

---

## Key Learnings

- DevEBox v2.0 has a non-standard camera connector — pin 1 does NOT match OV7670 flex cable pin 1. Using the connector killed an OV2640 during development.
- OV7670 I2C address is `0x21` (7-bit) → `0x42` write address. RST and PWDN pins must be initialized before the I2C scan or the camera won't respond.
- DCMI VSYNC polarity must be set to **High** for OV7670 — Low polarity results in `DCMI_SR = 2` (VSYNC flag set) but no frame capture.
- I2C SCL on this board is **PB6** not PB8 — CubeMX assignment is ground truth, not assumptions.
- External pull-up resistors on I2C lines are mandatory. Internal STM32 pull-ups are too weak for reliable SCCB communication.
- OV7670 Y channel is at **odd byte indices** in YUV422 output (U Y V Y ordering), not even.
- Adaptive thresholding per-frame handles lighting variation far better than a fixed threshold.
- emlearn `method='inline'` embeds weights directly as C arrays — no heap, no malloc, runs on bare metal with zero dependencies.

---

## Memory Usage

```
Flash 128KB:
├── HAL + startup + system         ~10 KB
├── OV7670 driver                  ~3  KB
├── Application + preprocessing    ~9  KB
└── emlearn MLP weights            ~50 KB
                                   ──────
                                   ~72 KB / 128 KB

RAM (AXI SRAM):
├── frame_buf  160×120×2 YUV422    ~38 KB
├── gray_buf   160×120 grayscale   ~19 KB
├── small_buf  28×28               ~0.8 KB
├── features   784 floats          ~3  KB
└── Stack + HAL buffers            ~5  KB
                                   ──────
                                   ~66 KB
```

---

## Related Projects

- **simple_ai_on_stm32h750** — Iris flower classifier on the same board. UART RX interrupt pipeline: PC sends comma-separated floats, MCU responds with class name. Built as a side project during camera hardware gap.

---

by [Alimt36](https://github.com/Alimt36)