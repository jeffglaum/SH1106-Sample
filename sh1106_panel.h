/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  

// TODO Insert appropriate #include <>

// TODO Insert C++ class definitions if appropriate

// TODO Insert declarations

// Comment a function and leverage automatic documentation with slash star star
/**
    <p><b>Function prototype:</b></p>
  
    <p><b>Summary:</b></p>

    <p><b>Description:</b></p>

    <p><b>Precondition:</b></p>

    <p><b>Parameters:</b></p>

    <p><b>Returns:</b></p>

    <p><b>Example:</b></p>
    <code>
 
    </code>

    <p><b>Remarks:</b></p>
 */
// TODO Insert declarations or function prototypes (right here) to leverage 
// live documentation

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#define I2C_OLED_ADDRESS                0x3C

#define SH1106_LCDWIDTH                 128
#define SH1106_LCDHEIGHT                64

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

#define BLACK 0
#define WHITE 1
#define INVERSE 2

void SH1106_InitDisplay(void);
void SH1106_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void SH1106_InvertDisplay(bool invert);
void SH1106_ClearDisplay(void);
void SH1106_Display(void);
void SH1106_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void SH1106_DrawFastVLine(int16_t x, int16_t __y, int16_t __h, uint16_t color);
void SH1106_DrawCircle (uint8_t x, uint8_t y, uint8_t r, uint16_t color, bool fill);
void SH1106_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color, bool fill);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */

