#ifndef INC_OV7670_H_
#define INC_OV7670_H_

#include "stm32h7xx_hal.h"

//---------------------------------------------------------------------------------------------------------------------------
// OV7670 I2C address
//---------------------------------------------------------------------------------------------------------------------------
// #define OV7670_I2C_ADDR     0x42   // ---> write address (7-bit: 0x21)
#define OV7670_I2C_ADDR     0x21 << 1

//---------------------------------------------------------------------------------------------------------------------------
// register addresses
//---------------------------------------------------------------------------------------------------------------------------
#define REG_GAIN            0x00
#define REG_COM1            0x04
#define REG_COM2            0x09
#define REG_COM3            0x0C
#define REG_COM4            0x0D
#define REG_COM7            0x12
#define REG_COM8            0x13
#define REG_COM9            0x14
#define REG_COM10           0x15
#define REG_COM11           0x3B
#define REG_COM13           0x3D
#define REG_COM14           0x3E
#define REG_COM15           0x40
#define REG_COM17           0x42
#define REG_HSTART          0x17
#define REG_HSTOP           0x18
#define REG_HREF            0x32
#define REG_VSTART          0x19
#define REG_VSTOP           0x1A
#define REG_VREF            0x03
#define REG_CLKRC           0x11
#define REG_RGB444          0x8C
#define REG_MVFP            0x1E
#define REG_BRIGHT          0x55
#define REG_CONTRAS         0x56
#define REG_SCALING_DCWCTR  0x72
#define REG_SCALING_PCLK    0x73
#define REG_PID             0x0A   // ---> product ID high byte  (should read 0x76)
#define REG_VER             0x0B   // ---> product ID low  byte  (should read 0x73)

#define COM7_RESET          0x80   // ---> software reset
#define COM15_R00FF         0xC0   // ---> full output range
#define COM13_UVSAT         0x40   // ---> UV saturation auto adjust
#define END_MARKER          0xFF   // ---> end of register table

//---------------------------------------------------------------------------------------------------------------------------
// exported functions
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef OV7670_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef OV7670_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val);
HAL_StatusTypeDef OV7670_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *val);

#endif /* INC_OV7670_H_ */