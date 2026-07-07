#include "ssd1306.h"

//---------------------------------------------------------------------------------------------------------------------------
// internal framebuffer : 128x64, 1 bit per pixel, page-addressed (8 rows of pixels per byte)
//   ---> layout matches SSD1306 native GDDRAM format, so UpdateScreen can push it directly
//---------------------------------------------------------------------------------------------------------------------------
static uint8_t framebuf[SSD1306_WIDTH * SSD1306_PAGES];

static I2C_HandleTypeDef *ssd_hi2c;
static uint8_t ssd_addr;

//---------------------------------------------------------------------------------------------------------------------------
// control byte prefixes, per SSD1306 datasheet I2C protocol
//---------------------------------------------------------------------------------------------------------------------------
#define CTRL_CMD            0x00   // ---> next byte(s) are command(s)
#define CTRL_DATA           0x40   // ---> next byte(s) are data (GDDRAM)

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306 command bytes used by this driver
//---------------------------------------------------------------------------------------------------------------------------
#define CMD_DISPLAY_OFF          0xAE
#define CMD_DISPLAY_ON           0xAF
#define CMD_SET_CLOCK_DIV        0xD5
#define CMD_SET_MULTIPLEX        0xA8
#define CMD_SET_DISPLAY_OFFSET   0xD3
#define CMD_SET_START_LINE       0x40
#define CMD_CHARGE_PUMP          0x8D
#define CMD_MEMORY_MODE          0x20
#define CMD_SEGRE_MAP            0xA1   // ---> column 127 mapped to SEG0 (mirror)
#define CMD_COM_SCAN_DEC         0xC8   // ---> scan from COM[N-1] to COM0
#define CMD_SET_COM_PINS         0xDA
#define CMD_SET_CONTRAST         0x81
#define CMD_SET_PRECHARGE        0xD9
#define CMD_SET_VCOM_DETECT      0xDB
#define CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define CMD_NORMAL_DISPLAY       0xA6
#define CMD_COLUMN_ADDR          0x21
#define CMD_PAGE_ADDR            0x22

//---------------------------------------------------------------------------------------------------------------------------
// init register table
//   ---> standard SSD1306 128x64 bring-up sequence, values are the widely-used defaults
//        for this exact panel size (matches the working Marlin/SSD1306 bring-up pattern)
//   ---> format : each entry is one command byte, terminated by END_MARKER
//        entries needing a parameter are listed as two consecutive bytes (cmd, param)
//---------------------------------------------------------------------------------------------------------------------------
#define END_MARKER          0xFF

static const uint8_t init_table[] = {
    CMD_DISPLAY_OFF,
    CMD_SET_CLOCK_DIV,        0x80,
    CMD_SET_MULTIPLEX,        0x3F,   // ---> 64 rows - 1
    CMD_SET_DISPLAY_OFFSET,   0x00,
    CMD_SET_START_LINE | 0x00,
    CMD_CHARGE_PUMP,          0x14,   // ---> enable internal charge pump (needed for most 3.3V modules)
    CMD_MEMORY_MODE,          0x00,   // ---> horizontal addressing mode
    CMD_SEGRE_MAP,
    CMD_COM_SCAN_DEC,
    CMD_SET_COM_PINS,         0x12,
    CMD_SET_CONTRAST,         0xCF,
    CMD_SET_PRECHARGE,        0xF1,
    CMD_SET_VCOM_DETECT,      0x40,
    CMD_DISPLAY_ALL_ON_RESUME,
    CMD_NORMAL_DISPLAY,
    CMD_DISPLAY_ON,
    END_MARKER,
};

//---------------------------------------------------------------------------------------------------------------------------
// write_cmd :
//   ---> sends a single command byte with the I2C control-byte prefix
//   ---> inputs : command byte
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
static HAL_StatusTypeDef write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {CTRL_CMD, cmd};
    return HAL_I2C_Master_Transmit(ssd_hi2c, ssd_addr, buf, 2, 100);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// run_init_table :
//   ---> walks init_table[] sending every byte as a command until END_MARKER
//   ---> inputs : none
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
static HAL_StatusTypeDef run_init_table(void)
{
    HAL_StatusTypeDef ret = HAL_OK;
    for(int i = 0; init_table[i] != END_MARKER; i++)
    {
        ret = write_cmd(init_table[i]);
        if(ret != HAL_OK) return ret;
    }
    return ret;
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_Init :
//   ---> stores the I2C handle/address, runs the panel init sequence, clears and pushes
//        a blank framebuffer
//   ---> inputs : hi2c handle, i2c_addr (already shifted for HAL, e.g. 0x3C << 1)
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c, uint8_t i2c_addr)
{
    ssd_hi2c = hi2c;
    ssd_addr = i2c_addr;

    HAL_StatusTypeDef ret = run_init_table();
    if(ret != HAL_OK) return ret;

    SSD1306_Clear();
    return SSD1306_UpdateScreen();
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_UpdateScreen :
//   ---> sets the full-screen column/page address window, then streams the entire
//        framebuffer to GDDRAM in one shot
//   ---> inputs : none
//   ---> outputs : HAL status
//---------------------------------------------------------------------------------------------------------------------------
HAL_StatusTypeDef SSD1306_UpdateScreen(void)
{
    write_cmd(CMD_COLUMN_ADDR);
    write_cmd(0);
    write_cmd(SSD1306_WIDTH - 1);

    write_cmd(CMD_PAGE_ADDR);
    write_cmd(0);
    write_cmd(SSD1306_PAGES - 1);

    // ---> data control byte, then the whole framebuffer as one transmission
    uint8_t buf[1 + sizeof(framebuf)];
    buf[0] = CTRL_DATA;
    for(uint32_t i = 0; i < sizeof(framebuf); i++)
    {
        buf[1 + i] = framebuf[i];
    }

    return HAL_I2C_Master_Transmit(ssd_hi2c, ssd_addr, buf, sizeof(buf), 1000);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_Clear :
//   ---> zeroes the internal framebuffer (all pixels off)
//   ---> inputs : none
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void SSD1306_Clear(void)
{
    for(uint32_t i = 0; i < sizeof(framebuf); i++)
    {
        framebuf[i] = 0x00;
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_DrawPixel :
//   ---> sets or clears a single pixel in the framebuffer
//   ---> inputs : x (0-127), y (0-63), color (0=off, 1=on)
//   ---> outputs : none
//   ---> how it works : each byte holds 8 vertically-stacked pixels (one page), so the
//        target byte is framebuf[page * WIDTH + x], target bit is y % 8
//---------------------------------------------------------------------------------------------------------------------------
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint16_t index = (y / 8) * SSD1306_WIDTH + x;
    uint8_t  bit   = y % 8;

    if(color)
        framebuf[index] |= (1 << bit);
    else
        framebuf[index] &= ~(1 << bit);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_DrawGrayBuffer :
//   ---> draws the 28x28 preprocessed camera buffer onto the OLED, thresholded to 1-bit
//        (OLED has no grayscale in this simple driver, so anything above mid-gray lights up)
//   ---> inputs : buf28x28 (784 bytes, row-major, 0=black..255=white), x0,y0 top-left,
//        scale (pixels per source pixel)
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void SSD1306_DrawGrayBuffer(const uint8_t *buf28x28, uint8_t x0, uint8_t y0, uint8_t scale)
{
    const uint8_t threshold = 128;

    for(uint8_t row = 0; row < 28; row++)
    {
        for(uint8_t col = 0; col < 28; col++)
        {
            uint8_t gray  = buf28x28[row * 28 + col];
            uint8_t color = (gray > threshold) ? 1 : 0;

            for(uint8_t dy = 0; dy < scale; dy++)
            {
                for(uint8_t dx = 0; dx < scale; dx++)
                {
                    SSD1306_DrawPixel(x0 + col * scale + dx, y0 + row * scale + dy, color);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// digit_segment_map :
//   ---> which of the 7 segments (a,b,c,d,e,f,g) are lit for each digit 0-9
//   ---> segment layout, standard 7-segment naming :
//              _a_
//             f   b
//              _g_
//             e   c
//              _d_
//   ---> bit order in each entry : bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g
//---------------------------------------------------------------------------------------------------------------------------
static const uint8_t digit_segment_map[10] = {
    0x3F, // ---> 0 : a b c d e f
    0x06, // ---> 1 : b c
    0x5B, // ---> 2 : a b d e g
    0x4F, // ---> 3 : a b c d g
    0x66, // ---> 4 : b c f g
    0x6D, // ---> 5 : a c d f g
    0x7D, // ---> 6 : a c d e f g
    0x07, // ---> 7 : a b c
    0x7F, // ---> 8 : a b c d e f g
    0x6F, // ---> 9 : a b c d f g
};
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// fill_rect_fb :
//   ---> fills a rectangular region of the framebuffer with pixels on (used by DrawDigit)
//   ---> inputs : x0,y0,x1,y1 (inclusive box)
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void fill_rect_fb(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    for(uint8_t y = y0; y <= y1; y++)
    {
        for(uint8_t x = x0; x <= x1; x++)
        {
            SSD1306_DrawPixel(x, y, 1);
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// SSD1306_DrawDigit :
//   ---> draws a single digit 0-9 as blocky 7-segment rectangles, no font table needed
//   ---> inputs : digit (0-9), x0,y0 top-left, scale (segment thickness unit)
//   ---> outputs : none
//   ---> how it works : same segment layout as the LCD driver's DrawDigit, just aimed at
//        the 1-bit OLED framebuffer instead of RGB565
//---------------------------------------------------------------------------------------------------------------------------
void SSD1306_DrawDigit(uint8_t digit, uint8_t x0, uint8_t y0, uint8_t scale)
{
    if(digit > 9) return;

    uint8_t segs = digit_segment_map[digit];

    uint8_t thick = scale;
    uint8_t len   = scale * 3;   // ---> shorter run than the LCD version, OLED is much smaller

    // ---> a : top horizontal
    if(segs & 0x01) fill_rect_fb(x0 + thick, y0, x0 + thick + len - 1, y0 + thick - 1);
    // ---> b : top-right vertical
    if(segs & 0x02) fill_rect_fb(x0 + thick + len, y0 + thick, x0 + thick + len + thick - 1, y0 + thick + len - 1);
    // ---> c : bottom-right vertical
    if(segs & 0x04) fill_rect_fb(x0 + thick + len, y0 + 2*thick + len, x0 + thick + len + thick - 1, y0 + 2*thick + 2*len - 1);
    // ---> d : bottom horizontal
    if(segs & 0x08) fill_rect_fb(x0 + thick, y0 + 2*thick + 2*len, x0 + thick + len - 1, y0 + 3*thick + 2*len - 1);
    // ---> e : bottom-left vertical
    if(segs & 0x10) fill_rect_fb(x0, y0 + 2*thick + len, x0 + thick - 1, y0 + 2*thick + 2*len - 1);
    // ---> f : top-left vertical
    if(segs & 0x20) fill_rect_fb(x0, y0 + thick, x0 + thick - 1, y0 + thick + len - 1);
    // ---> g : middle horizontal
    if(segs & 0x40) fill_rect_fb(x0 + thick, y0 + thick + len, x0 + thick + len - 1, y0 + 2*thick + len - 1);
}
//---------------------------------------------------------------------------------------------------------------------------