#ifndef INC_RM68140_H_
#define INC_RM68140_H_

#include "stm32h7xx_hal.h"

//---------------------------------------------------------------------------------------------------------------------------
// RM68140 3.5" TFT LCD, 8-bit parallel (8080-style), 320x480 native, MIPI DCS compatible
//---------------------------------------------------------------------------------------------------------------------------
// ---> control pin mapping (fixed to GPIOE, matches project pin budget) :
//          D0-D7 : PE8-PE15  (contiguous data bus, single ODR write)
//          CS    : PE0
//          RS/DC : PE1
//          WR    : PE2
//          RD    : PE3       (driven high always, no readback used)
//          RST   : PE7
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// GPIO pin defines
//---------------------------------------------------------------------------------------------------------------------------
#define LCD_GPIO_PORT       GPIOE

#define LCD_PIN_CS          GPIO_PIN_0
#define LCD_PIN_RS          GPIO_PIN_1
#define LCD_PIN_WR          GPIO_PIN_2
#define LCD_PIN_RD          GPIO_PIN_3
#define LCD_PIN_RST         GPIO_PIN_7

#define LCD_DATA_SHIFT      8            // ---> D0-D7 start at PE8
#define LCD_DATA_MASK       (0xFFU << LCD_DATA_SHIFT)

//---------------------------------------------------------------------------------------------------------------------------
// MIPI DCS command set (only what this driver actually uses)
//---------------------------------------------------------------------------------------------------------------------------
#define CMD_SWRESET         0x01   // ---> software reset
#define CMD_SLPOUT          0x11   // ---> sleep out
#define CMD_DISPON          0x29   // ---> display on
#define CMD_CASET           0x2A   // ---> column address set
#define CMD_PASET           0x2B   // ---> page (row) address set
#define CMD_RAMWR           0x2C   // ---> memory write
#define CMD_MADCTL          0x36   // ---> memory access control (rotation/mirroring)
#define CMD_COLMOD          0x3A   // ---> pixel format set

#define COLMOD_16BIT        0x55   // ---> 16-bit/pixel, RGB565

//---------------------------------------------------------------------------------------------------------------------------
// panel dimensions
//---------------------------------------------------------------------------------------------------------------------------
#define LCD_WIDTH           320
#define LCD_HEIGHT          480

//---------------------------------------------------------------------------------------------------------------------------
// a few basic RGB565 colors, useful for the digit/segment drawing
//---------------------------------------------------------------------------------------------------------------------------
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F

//---------------------------------------------------------------------------------------------------------------------------
// exported functions
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_Init(void);
void RM68140_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void RM68140_PushPixel(uint16_t color);
void RM68140_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void RM68140_FillScreen(uint16_t color);

// ---> app-level drawing, no font table, block/segment based
void RM68140_DrawGrayBuffer(const uint8_t *buf28x28, uint16_t x0, uint16_t y0, uint16_t scale);
void RM68140_DrawDigit(uint8_t digit, uint16_t x0, uint16_t y0, uint16_t scale, uint16_t color);

#endif /* INC_RM68140_H_ */