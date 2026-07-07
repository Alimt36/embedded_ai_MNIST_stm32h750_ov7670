#include "rm68140.h"

//---------------------------------------------------------------------------------------------------------------------------
// init register table
//   ---> RM68140 answers MIPI DCS ID 0x6814, so it only needs the generic DCS wake sequence
//        plus one panel-specific register (pixel format) beyond reset/sleep-out/display-on
//   ---> table format : {command, num_params, param0, param1, ...}, terminated by END_MARKER
//   ---> a delay-only entry is marked with CMD_DELAY as the command byte
//---------------------------------------------------------------------------------------------------------------------------
#define END_MARKER          0xFE
#define CMD_DELAY           0xFD

static const uint8_t init_table[] = {
    CMD_SWRESET, 0,                    // ---> software reset, no params
    CMD_DELAY,   120,                  // ---> wait 120ms after reset (panel spec)
    CMD_SLPOUT,  0,                    // ---> sleep out
    CMD_DELAY,   120,                  // ---> wait 120ms after sleep out
    CMD_COLMOD,  1, COLMOD_16BIT,      // ---> 16-bit/pixel, RGB565
    CMD_MADCTL,  1, 0x48,              // ---> row/col exchange off, BGR order (typical for this panel family)
    CMD_DISPON,  0,                    // ---> display on
    CMD_DELAY,   20,
    END_MARKER,
};

//---------------------------------------------------------------------------------------------------------------------------
// low_delay_us :
//   ---> crude busy-wait, used only for WR pulse width and RST timing
//   ---> inputs : microseconds (approximate, not calibrated against a timer)
//   ---> outputs : none
//   ---> how it works : loop count is a rough calibration for H750 core clock, fine for
//        WR pulse width since the panel's minimum pulse spec is a few dozen ns
//---------------------------------------------------------------------------------------------------------------------------
static void low_delay_us(uint32_t us)
{
    volatile uint32_t count = us * 40; // ---> rough loop calibration, not cycle-exact
    while(count--);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// gpio_init :
//   ---> configures PE0-PE3, PE7 (control), PE8-PE13 (D0-D5), and PD0-PD1 (D6-D7)
//        as push-pull outputs
//   ---> inputs : none
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void gpio_init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    // ---> control pins : CS, RS, WR, RD, RST (all on GPIOE)
    gpio.Pin   = LCD_PIN_CS | LCD_PIN_RS | LCD_PIN_WR | LCD_PIN_RD | LCD_PIN_RST;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CTRL_PORT, &gpio);

    // ---> data bus, low 6 bits : D0-D5 on PE8-PE13
    gpio.Pin   = LCD_DATA_E_MASK;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DATA_PORT_E, &gpio);

    // ---> data bus, high 2 bits : D6-D7 on PD0-PD1
    gpio.Pin   = LCD_DATA_D_MASK;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DATA_PORT_D, &gpio);

    // ---> RD is never actually toggled for a read, held high always
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RD, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_CS, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_WR, GPIO_PIN_SET);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// write_bus_byte :
//   ---> puts one byte on D0-D7 and pulses WR, CS held low for the whole transaction
//   ---> inputs : byte to write
//   ---> outputs : none
//   ---> how it works : data byte is split across two ports (D0-D5 on GPIOE, D6-D7 on
//        GPIOD), so this is two masked ODR writes instead of one, but still far cheaper
//        than 8 individual pin toggles
//---------------------------------------------------------------------------------------------------------------------------
static void write_bus_byte(uint8_t data)
{
    // ---> low 6 bits (D0-D5) onto PE8-PE13
    uint32_t odr_e = LCD_DATA_PORT_E->ODR;
    odr_e &= ~LCD_DATA_E_MASK;
    odr_e |= (((uint32_t)data & 0x3FU) << LCD_DATA_E_SHIFT);
    LCD_DATA_PORT_E->ODR = odr_e;

    // ---> top 2 bits (D6-D7) onto PD0-PD1
    uint32_t odr_d = LCD_DATA_PORT_D->ODR;
    odr_d &= ~LCD_DATA_D_MASK;
    odr_d |= ((((uint32_t)data >> 6) & 0x03U) << LCD_DATA_D_SHIFT);
    LCD_DATA_PORT_D->ODR = odr_d;

    // ---> WR pulse, low then high, panel latches data on rising edge
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_WR, GPIO_PIN_RESET);
    low_delay_us(1);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_WR, GPIO_PIN_SET);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// write_cmd :
//   ---> sends a command byte (RS low)
//   ---> inputs : command byte
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void write_cmd(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_CS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_RESET);
    write_bus_byte(cmd);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_CS, GPIO_PIN_SET);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// write_data :
//   ---> sends a data byte (RS high)
//   ---> inputs : data byte
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void write_data(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_CS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_SET);
    write_bus_byte(data);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_CS, GPIO_PIN_SET);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// write_data16 :
//   ---> sends a 16-bit RGB565 pixel as two data bytes, high byte first
//   ---> inputs : 16-bit color
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void write_data16(uint16_t color)
{
    write_data((uint8_t)(color >> 8));
    write_data((uint8_t)(color & 0xFF));
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// hw_reset :
//   ---> pulses RST low, panel-spec timing
//   ---> inputs : none
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void hw_reset(void)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RST, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RST, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RST, GPIO_PIN_SET);
    HAL_Delay(120); // ---> panel needs time after reset before accepting commands
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// run_init_table :
//   ---> walks init_table[], dispatching commands, params, and delay entries
//   ---> inputs : none
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
static void run_init_table(void)
{
    uint32_t i = 0;

    while(init_table[i] != END_MARKER)
    {
        uint8_t cmd = init_table[i++];

        if(cmd == CMD_DELAY)
        {
            HAL_Delay(init_table[i++]);
            continue;
        }

        uint8_t num_params = init_table[i++];
        write_cmd(cmd);
        for(uint8_t p = 0; p < num_params; p++)
        {
            write_data(init_table[i++]);
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_Init :
//   ---> full LCD bring-up : GPIO config, hardware reset, MIPI DCS init sequence
//   ---> inputs : none
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_Init(void)
{
    gpio_init();
    hw_reset();
    run_init_table();
    RM68140_FillScreen(COLOR_BLACK);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_SetWindow :
//   ---> sets the active drawing window (column + row address), RAM write pointer
//        will auto-increment inside this box on subsequent PushPixel calls
//   ---> inputs : x0,y0 (top-left), x1,y1 (bottom-right), inclusive
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_cmd(CMD_CASET);
    write_data16(x0);
    write_data16(x1);

    write_cmd(CMD_PASET);
    write_data16(y0);
    write_data16(y1);

    write_cmd(CMD_RAMWR);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_PushPixel :
//   ---> writes one pixel into the currently active window (call SetWindow first)
//   ---> inputs : 16-bit RGB565 color
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_PushPixel(uint16_t color)
{
    write_data16(color);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_FillRect :
//   ---> fills a rectangular region with a solid color
//   ---> inputs : x0,y0,x1,y1 (inclusive box), color
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    RM68140_SetWindow(x0, y0, x1, y1);

    uint32_t num_pixels = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);
    for(uint32_t i = 0; i < num_pixels; i++)
    {
        RM68140_PushPixel(color);
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_FillScreen :
//   ---> fills the entire panel with a solid color
//   ---> inputs : color
//   ---> outputs : none
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_FillScreen(uint16_t color)
{
    RM68140_FillRect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// RM68140_DrawGrayBuffer :
//   ---> draws the 28x28 preprocessed camera buffer (grayscale, 0=black..255=white) onto
//        the panel, each source pixel scaled up into a scale x scale block
//   ---> inputs : buf28x28 (784 bytes, row-major), x0,y0 top-left on screen, scale (pixels
//        per source pixel, e.g. scale=8 draws a 224x224 box)
//   ---> outputs : none
//   ---> how it works : converts each 8-bit gray value to RGB565 (R=G=B ramp), fills one
//        scale x scale block per source pixel with a single SetWindow+PushPixel run
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_DrawGrayBuffer(const uint8_t *buf28x28, uint16_t x0, uint16_t y0, uint16_t scale)
{
    for(uint16_t row = 0; row < 28; row++)
    {
        for(uint16_t col = 0; col < 28; col++)
        {
            uint8_t gray = buf28x28[row * 28 + col];

            // ---> 8-bit gray to RGB565 : R,B get top 5 bits, G gets top 6 bits
            uint16_t r = (gray >> 3) & 0x1F;
            uint16_t g = (gray >> 2) & 0x3F;
            uint16_t b = (gray >> 3) & 0x1F;
            uint16_t color565 = (r << 11) | (g << 5) | b;

            uint16_t px = x0 + col * scale;
            uint16_t py = y0 + row * scale;

            RM68140_FillRect(px, py, px + scale - 1, py + scale - 1, color565);
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
// RM68140_DrawDigit :
//   ---> draws a single digit 0-9 as blocky 7-segment rectangles, no font table needed
//   ---> inputs : digit (0-9), x0,y0 top-left, scale (segment thickness unit), color
//   ---> outputs : none
//   ---> how it works : each segment is one FillRect, sized/positioned relative to scale,
//        only segments set in digit_segment_map are drawn
//---------------------------------------------------------------------------------------------------------------------------
void RM68140_DrawDigit(uint8_t digit, uint16_t x0, uint16_t y0, uint16_t scale, uint16_t color)
{
    if(digit > 9) return;

    uint8_t segs = digit_segment_map[digit];

    uint16_t thick = scale;          // ---> segment thickness
    uint16_t len    = scale * 4;      // ---> segment length (horizontal/vertical run)

    // ---> a : top horizontal
    if(segs & 0x01) RM68140_FillRect(x0 + thick, y0, x0 + thick + len - 1, y0 + thick - 1, color);
    // ---> b : top-right vertical
    if(segs & 0x02) RM68140_FillRect(x0 + thick + len, y0 + thick, x0 + thick + len + thick - 1, y0 + thick + len - 1, color);
    // ---> c : bottom-right vertical
    if(segs & 0x04) RM68140_FillRect(x0 + thick + len, y0 + 2*thick + len, x0 + thick + len + thick - 1, y0 + 2*thick + 2*len - 1, color);
    // ---> d : bottom horizontal
    if(segs & 0x08) RM68140_FillRect(x0 + thick, y0 + 2*thick + 2*len, x0 + thick + len - 1, y0 + 3*thick + 2*len - 1, color);
    // ---> e : bottom-left vertical
    if(segs & 0x10) RM68140_FillRect(x0, y0 + 2*thick + len, x0 + thick - 1, y0 + 2*thick + 2*len - 1, color);
    // ---> f : top-left vertical
    if(segs & 0x20) RM68140_FillRect(x0, y0 + thick, x0 + thick - 1, y0 + thick + len - 1, color);
    // ---> g : middle horizontal
    if(segs & 0x40) RM68140_FillRect(x0 + thick, y0 + thick + len, x0 + thick + len - 1, y0 + 2*thick + len - 1, color);
}
//---------------------------------------------------------------------------------------------------------------------------