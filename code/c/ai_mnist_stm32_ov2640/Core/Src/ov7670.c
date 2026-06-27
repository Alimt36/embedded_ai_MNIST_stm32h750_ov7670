#include "ov7670.h"

//---------------------------------------------------------------------------------------------------------------------------
// register init tables
//---------------------------------------------------------------------------------------------------------------------------
static const uint8_t yuv422_regs[][2] = {
    {REG_COM7,  0x00},          // ---> YUV mode
    {REG_RGB444, 0x00},         // ---> no RGB444
    {REG_COM1,  0x00},
    {REG_COM15, COM15_R00FF},   // ---> full output range
    {REG_COM9,  0x6A},          // ---> 128x gain ceiling
    {0x4f,      0x80},          // ---> matrix coefficient 1
    {0x50,      0x80},          // ---> matrix coefficient 2
    {0x51,      0x00},
    {0x52,      0x22},          // ---> matrix coefficient 4
    {0x53,      0x5e},          // ---> matrix coefficient 5
    {0x54,      0x80},          // ---> matrix coefficient 6
    {REG_COM13, COM13_UVSAT},
    {END_MARKER, END_MARKER},
};

static const uint8_t qqvga_regs[][2] = {
    {REG_COM14, 0x1a},          // ---> divide by 4
    {0x72,      0x22},          // ---> downsample by 4
    {0x73,      0xf2},          // ---> divide by 4
    {REG_HSTART,0x16},
    {REG_HSTOP, 0x04},
    {REG_HREF,  0xa4},
    {REG_VSTART,0x02},
    {REG_VSTOP, 0x7a},
    {REG_VREF,  0x0a},
    {END_MARKER, END_MARKER},
};

static const uint8_t default_regs[][2] = {
    {REG_COM7,  COM7_RESET},    // ---> software reset
    {END_MARKER, END_MARKER},
};
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// OV7670_WriteReg :
//   ---> writes one byte to an OV7670 register over I2C (SCCB)
//   ---> inputs : hi2c, reg address, value
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef OV7670_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, OV7670_I2C_ADDR, buf, 2, 100);
    HAL_Delay(1);
    return ret;
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// OV7670_ReadReg :
//   ---> reads one byte from an OV7670 register over I2C (SCCB)
//   ---> inputs : hi2c, reg address, pointer to result
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef OV7670_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *val)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(hi2c, OV7670_I2C_ADDR, &reg, 1, 100);
    if(ret != HAL_OK) return ret;
    ret = HAL_I2C_Master_Receive(hi2c, OV7670_I2C_ADDR | 0x01, val, 1, 100);
    return ret;
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// write_regs :
//   ---> writes a full register table until END_MARKER
//   ---> inputs : hi2c, register table
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
static HAL_StatusTypeDef write_regs(I2C_HandleTypeDef *hi2c, const uint8_t regs[][2])
{
    HAL_StatusTypeDef ret = HAL_OK;
    for(int i = 0; regs[i][0] != END_MARKER; i++)
    {
        ret = OV7670_WriteReg(hi2c, regs[i][0], regs[i][1]);
        if(ret != HAL_OK) return ret;
    }
    return ret;
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// OV7670_Init :
//   ---> full camera initialization sequence
//   ---> resets camera, verifies PID, sets YUV422 + QQVGA mode
//   ---> inputs : hi2c
//   ---> outputs : HAL_OK if successful, HAL_ERROR if camera not found
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef OV7670_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t pid = 0;
    uint8_t ver = 0;

    // ---> power up delay
    HAL_Delay(100);

    // ---> software reset
    write_regs(hi2c, default_regs);
    HAL_Delay(300);

    // ---> verify camera ID
    OV7670_ReadReg(hi2c, REG_PID, &pid);
    OV7670_ReadReg(hi2c, REG_VER, &ver);

    if(pid != 0x76)
    {
        // ---> camera not responding or wrong device
        return HAL_ERROR;
    }

    // ---> set clock divider
    OV7670_WriteReg(hi2c, REG_CLKRC, 0x03);

    // ---> set YUV422 format
    write_regs(hi2c, yuv422_regs);

    // ---> set QQVGA resolution (160x120)
    write_regs(hi2c, qqvga_regs);

    // ---> mirror and flip for correct orientation
    OV7670_WriteReg(hi2c, REG_MVFP, 0x30);

    // ---> brightness and contrast
    OV7670_WriteReg(hi2c, REG_BRIGHT,  0xb0);
    OV7670_WriteReg(hi2c, REG_CONTRAS, 0x60);

    HAL_Delay(300);

    return HAL_OK;
}
//---------------------------------------------------------------------------------------------------------------------------