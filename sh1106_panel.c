/*
 * File:   sh1106_panel.c
 * Author: jeffglaum
 *
 * Created on December 26, 2023, 12:19 PM
 */


#include "xc.h"

#include <stdbool.h>       /* Includes true/false definition                  */
#include <string.h>
#include <math.h>

#include "i2c.h"
#include "sh1106_panel.h"

// Display frame buffer.
extern uint8_t buffer[SH1106_LCDHEIGHT * SH1106_LCDWIDTH / 8];

    
static void SH1106_command(uint8_t c)
{    
    I2C1_M_Write(I2C_OLED_ADDRESS, 0, 1, &c);
}

void SH1106_Display(void) {
	
    SH1106_command(SH1106_SETLOWCOLUMN | 0x0);      // low col = 0
    SH1106_command(SH1106_SETHIGHCOLUMN | 0x0);     // hi col = 0
    SH1106_command(SH1106_SETSTARTLINE | 0x0);      // line #0
	
	uint8_t height = SH1106_LCDHEIGHT;
	uint8_t width =  SH1106_LCDWIDTH; 
	uint8_t m_row = 0;
    uint8_t m_col = 0;

	height >>= 3;
	width  >>= 3;
	
	int p = 0;
    uint8_t i, j=0;
	
	for (i = 0; i < height; i++)
    {
        SH1106_command(0xB0 + i + m_row);       //set page address
        SH1106_command(m_col & 0xf);            //set lower column address
        SH1106_command(0x10 | (m_col >> 4));    //set higher column address
		
        for(j = 0; j < 8; j++, p+=width)
        {   
            I2C1_M_Write(I2C_OLED_ADDRESS, 0x40, width, &buffer[p]);
        }
	}
}

void SH1106_ClearDisplay(void)
{
  memset(buffer, 0, (SH1106_LCDWIDTH * SH1106_LCDHEIGHT / 8));
}

void SH1106_InvertDisplay(bool invert)
{
  SH1106_command((invert ? SH1106_INVERTDISPLAY : SH1106_NORMALDISPLAY));
}

void SH1106_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= SH1106_LCDWIDTH || y >= SH1106_LCDHEIGHT)
  {
    return;
  }

  switch (color) 
  {
    case WHITE:   buffer[x + (y/8) * SH1106_LCDWIDTH] |=  (1 << (y&7)); break;
    case BLACK:   buffer[x + (y/8) * SH1106_LCDWIDTH] &= ~(1 << (y&7)); break; 
    case INVERSE: buffer[x + (y/8) * SH1106_LCDWIDTH] ^=  (1 << (y&7)); break; 
  }  
}

void SH1106_InitDisplay(void)
{
    // Initialization sequence for SH1106 (132x64 OLED module, 128x55 centered)
    SH1106_command(SH1106_DISPLAYOFF);
    SH1106_command(SH1106_SETDISPLAYCLOCKDIV);
    SH1106_command(0x80);                                   // The suggested ratio 0x80
    SH1106_command(SH1106_SETMULTIPLEX);
    SH1106_command(0x3F);
    SH1106_command(SH1106_SETDISPLAYOFFSET);
    SH1106_command(0x00);                                   // No offset
    SH1106_command(SH1106_SETSTARTLINE | 0x0);              // Line #0 0x40
    SH1106_command(SH1106_CHARGEPUMP);
    SH1106_command(0x10);
    SH1106_command(SH1106_MEMORYMODE);
    SH1106_command(0x00);                                   // 0x0 Act like KS0108
    SH1106_command(SH1106_SEGREMAP | 0x1);
    SH1106_command(SH1106_COMSCANDEC);
    SH1106_command(SH1106_SETCOMPINS);
    SH1106_command(0x12);
    SH1106_command(SH1106_SETCONTRAST);
    SH1106_command(0x80);
    SH1106_command(SH1106_SETPRECHARGE);
    SH1106_command(0x22);
    SH1106_command(SH1106_SETVCOMDETECT);
    SH1106_command(0x40);
    SH1106_command(SH1106_DISPLAYALLON_RESUME);
    SH1106_command(SH1106_NORMALDISPLAY);
    
    SH1106_command(SH1106_DISPLAYON);
}

void SH1106_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  // Do bounds/limit checks
  if(y < 0 || y >= SH1106_LCDHEIGHT) { return; }

  // make sure we don't try to draw below 0
  if(x < 0) { 
    w += x;
    x = 0;
  }

  // make sure we don't go off the edge of the display
  if( (x + w) > SH1106_LCDWIDTH) { 
    w = (SH1106_LCDWIDTH - x);
  }

  // if our width is now negative, punt
  if(w <= 0) { return; }

  // set up the pointer for  movement through the buffer
  register uint8_t *pBuf = buffer;
  // adjust the buffer pointer for the current row
  pBuf += ((y/8) * SH1106_LCDWIDTH);
  // and offset x columns in
  pBuf += x;

  register uint8_t mask = 1 << (y&7);

  switch (color) 
  {
  case WHITE:         while(w--) { *pBuf++ |= mask; }; break;
    case BLACK: mask = ~mask;   while(w--) { *pBuf++ &= mask; }; break;
  case INVERSE:         while(w--) { *pBuf++ ^= mask; }; break;
  }
}

void SH1106_DrawFastVLine(int16_t x, int16_t __y, int16_t __h, uint16_t color) {

  // do nothing if we're off the left or right side of the screen
  if(x < 0 || x >= SH1106_LCDWIDTH) { return; }

  // make sure we don't try to draw below 0
  if(__y < 0) { 
    // __y is negative, this will subtract enough from __h to account for __y being 0
    __h += __y;
    __y = 0;

  } 

  // make sure we don't go past the height of the display
  if( (__y + __h) > SH1106_LCDHEIGHT) { 
    __h = (SH1106_LCDHEIGHT - __y);
  }

  // if our height is now negative, punt 
  if(__h <= 0) { 
    return;
  }

  // this display doesn't need ints for coordinates, use local byte registers for faster juggling
  register uint8_t y = __y;
  register uint8_t h = __h;


  // set up the pointer for fast movement through the buffer
  register uint8_t *pBuf = buffer;
  // adjust the buffer pointer for the current row
  pBuf += ((y/8) * SH1106_LCDWIDTH);
  // and offset x columns in
  pBuf += x;

  // do the first partial byte, if necessary - this requires some masking
  register uint8_t mod = (y&7);
  if(mod) {
    // mask off the high n bits we want to set 
    mod = 8-mod;

    // note - lookup table results in a nearly 10% performance improvement in fill* functions
    // register uint8_t mask = ~(0xFF >> (mod));
    static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
    register uint8_t mask = premask[mod];

    // adjust the mask if we're not going to reach the end of this byte
    if( h < mod) { 
      mask &= (0XFF >> (mod-h));
    }

  switch (color) 
    {
    case WHITE:   *pBuf |=  mask;  break;
    case BLACK:   *pBuf &= ~mask;  break;
    case INVERSE: *pBuf ^=  mask;  break;
    }
  
    // fast exit if we're done here!
    if(h<mod) { return; }

    h -= mod;

    pBuf += SH1106_LCDWIDTH;
  }


  // write solid bytes while we can - effectively doing 8 rows at a time
  if(h >= 8) { 
    if (color == INVERSE)  {          // separate copy of the code so we don't impact performance of the black/white write version with an extra comparison per loop
      do  {
      *pBuf=~(*pBuf);

        // adjust the buffer forward 8 rows worth of data
        pBuf += SH1106_LCDWIDTH;

        // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
        h -= 8;
      } while(h >= 8);
      }
    else {
      // store a local value to work with 
      register uint8_t val = (color == WHITE) ? 255 : 0;

      do  {
        // write our value in
      *pBuf = val;

        // adjust the buffer forward 8 rows worth of data
        pBuf += SH1106_LCDWIDTH;

        // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
        h -= 8;
      } while(h >= 8);
      }
    }

  // now do the final partial byte, if necessary
  if(h) {
    mod = h & 7;
    // this time we want to mask the low bits of the byte, vs the high bits we did above
    // register uint8_t mask = (1 << mod) - 1;
    // note - lookup table results in a nearly 10% performance improvement in fill* functions
    static uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
    register uint8_t mask = postmask[mod];
    switch (color) 
    {
      case WHITE:   *pBuf |=  mask;  break;
      case BLACK:   *pBuf &= ~mask;  break;
      case INVERSE: *pBuf ^=  mask;  break;
    }
  }
}

void SH1106_DrawCircle (uint8_t x, uint8_t y, uint8_t r, uint16_t color, bool fill)
{
    int8_t mx = 0;
    int8_t my;
    
    for (mx=r ; mx>=-r ; mx--)
    {
        my = sqrt(r*r - mx*mx);
        if (fill)
        {
            SH1106_DrawFastVLine((x+mx), (y-my), (my*2), color);
        }
        else
        {
            SH1106_DrawPixel((x + mx), (y + my), color);
            SH1106_DrawPixel((x + mx), (y - my), color);
        }
    }
}

void SH1106_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color, bool fill)
{
    if (fill)
    {
        uint8_t my = 0;
        for (my=0 ; my<h ; my++)
        {
            SH1106_DrawFastHLine(x, y+my, w, color);
        }
    }
    else
    {
        SH1106_DrawFastHLine(x, y, w, color);
        SH1106_DrawFastHLine(x, y+h, w, color);
        SH1106_DrawFastVLine(x, y, h, color);
        SH1106_DrawFastVLine(x+w, y, h, color);
    }
}