#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include "stm32h7xx_hal.h"

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306 128x64 OLED, I2C interface
//   ---> use the same I2C_IsDeviceReady scan pattern already used for the OV7670 bring-up
//        to confirm the actual 7-bit address (commonly 0x3C, sometimes 0x3D)
//---------------------------------------------------------------------------------------------------------------------------

#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       (SSD1306_HEIGHT / 8)   // ---> 8 pages of 8 vertical pixels each

//---------------------------------------------------------------------------------------------------------------------------
// exported functions
//---------------------------------------------------------------------------------------------------------------------------

// ---> inputs : hi2c handle, 7-bit I2C address already shifted for HAL (addr << 1)
HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c, uint8_t i2c_addr);

// ---> pushes the internal framebuffer to the panel over I2C
HAL_StatusTypeDef SSD1306_UpdateScreen(void);

// ---> clears the internal framebuffer (does not push to panel, call UpdateScreen after)
void SSD1306_Clear(void);

// ---> sets a single pixel in the framebuffer, color : 0 = off, 1 = on
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

// ---> draws the 28x28 preprocessed camera buffer (grayscale 0-255), thresholded to 1-bit,
//      scaled up into the framebuffer at x0,y0
void SSD1306_DrawGrayBuffer(const uint8_t *buf28x28, uint8_t x0, uint8_t y0, uint8_t scale);

// ---> draws a single digit 0-9 as blocky 7-segment rectangles, no font table
void SSD1306_DrawDigit(uint8_t digit, uint8_t x0, uint8_t y0, uint8_t scale);

#endif /* INC_SSD1306_H_ */