#pragma once

#include <xc.h> // include processor files - each processor file is guarded.  

#define I2C_OLED_ADDRESS                0x3C

#define ROUND_UP_TO_BYTE_BOUNDARY(n)    ((n + 8 - 1) & ~(8 - 1))
#define ROUND_DOWN_TO_BYTE_BOUNDARY(n)  (n & ~(8 - 1))
#define NUM_LINES_IN_A_PAGE             8
#define NUM_BITS_TO_A_BYTE              8

// Display is 132x64 however only 128x64 is displayable (2 pixel boundary left and right sides).
#define SH1106_REAL_OLED_WIDTH_PIXELS   132
#define SH1106_REAL_OLED_HEIGHT_PIXELS  64

#define SH1106_BUFFER_LINE_WIDTH_BYTES  (ROUND_UP_TO_BYTE_BOUNDARY(SH1106_REAL_OLED_WIDTH_PIXELS) / NUM_BITS_TO_A_BYTE)
#define SH1106_BUFFER_NUM_LINES         (SH1106_REAL_OLED_HEIGHT_PIXELS)

#define SH1106_DISPLAYABLE_WIDTH_PIXELS  ROUND_DOWN_TO_BYTE_BOUNDARY(SH1106_REAL_OLED_WIDTH_PIXELS)
#define SH1106_DISPLAYABLE_HEIGHT_PIXELS ROUND_DOWN_TO_BYTE_BOUNDARY(SH1106_REAL_OLED_HEIGHT_PIXELS)

#define SH1106_SETCONTRAST              0x81
#define SH1106_DISPLAYALLON_RESUME      0xA4
#define SH1106_DISPLAYALLON             0xA5
#define SH1106_NORMALDISPLAY            0xA6
#define SH1106_INVERTDISPLAY            0xA7
#define SH1106_DISPLAYOFF               0xAE
#define SH1106_DISPLAYON                0xAF

#define SH1106_SETDISPLAYOFFSET         0xD3
#define SH1106_SETCOMPINS               0xDA

#define SH1106_SETVCOMDETECT            0xDB

#define SH1106_SETDISPLAYCLOCKDIV       0xD5
#define SH1106_SETPRECHARGE             0xD9

#define SH1106_SETMULTIPLEX             0xA8

#define SH1106_SETLOWCOLUMN             0x00
#define SH1106_SETHIGHCOLUMN            0x10

#define SH1106_SET_PAGEADDRESS          0xB0
#define SH1106_SETSTARTLINE             0x40

#define SH1106_MEMORYMODE               0x20
#define SH1106_COLUMNADDR               0x21
#define SH1106_PAGEADDR                 0x22

#define SH1106_COMSCANINC               0xC0
#define SH1106_COMSCANDEC               0xC8

#define SH1106_SEGREMAP                 0xA0

#define SH1106_CHARGEPUMP               0x8D

#define SH1106_EXTERNALVCC              0x1
#define SH1106_SWITCHCAPVCC             0x2

// Scrolling #defines
#define SH1106_ACTIVATE_SCROLL          0x2F
#define SH1106_DEACTIVATE_SCROLL        0x2E
#define SH1106_SET_VERTICAL_SCROLL_AREA 0xA3
#define SH1106_RIGHT_HORIZONTAL_SCROLL  0x26
#define SH1106_LEFT_HORIZONTAL_SCROLL   0x27
#define SH1106_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SH1106_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A

#define sh1106_swap(a, b) { int16_t t = a; a = b; b = t; }

#define BLACK       0
#define WHITE       1
#define INVERSE     2

#define PI          3.1415926
#define TWO_PI      (2.0 * PI)
#define ONE_RADIAN  (PI / 180.0)

void SH1106_InitDisplay(void);
void SH1106_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void SH1106_InvertDisplay(bool invert);
void SH1106_ClearDisplay(void);
void SH1106_Display(void);
void SH1106_DrawCircle (uint8_t x, uint8_t y, uint8_t r, uint16_t color, bool fill);
void SH1106_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color, bool fill);
