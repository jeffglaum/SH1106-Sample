/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__PIC24E__)
    	#include <p24Exxxx.h>
    #elif defined (__PIC24F__)||defined (__PIC24FK__)
	#include <p24Fxxxx.h>
    #elif defined(__PIC24H__)
	#include <p24Hxxxx.h>
    #endif
#endif

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */
#include <libpic30.h>

#include "i2c.h"
#include "sh1106_panel.h"
#include "font.h"
#include "Fonts/FreeSans9pt7b.h"

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

uint8_t buffer[SH1106_BUFFER_LINE_WIDTH_BYTES * SH1106_BUFFER_NUM_LINES];

int16_t main(void)
{
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize IO ports and peripherals */
    InitApp();

    // Initialize I2C.
    I2C_Initialize();
    I2C_ModuleStart();
    I2C1_M_Poll(I2C_OLED_ADDRESS);
    
    // Configure display.
    SH1106_InitDisplay();
    SH1106_InvertDisplay(true);
    SH1106_ClearDisplay();
    
    SetFont(&FreeSans9pt7b);
    WriteChar('\n');
    WriteChar('J');
    WriteChar('e');
    WriteChar('f');
    WriteChar('f');
    WriteChar('e');
    WriteChar('r');
    WriteChar('y');
    WriteChar(' ');
    WriteChar('D');
    WriteChar('a');
    WriteChar('n');
    WriteChar('i');
    WriteChar('e');
    WriteChar('l');
    WriteChar('\n');
    WriteChar('G');
    WriteChar('l');
    WriteChar('a');
    WriteChar('u');
    WriteChar('m');
    
    SH1106_Display();

    while(1)
    {
        __delay_ms(500);

        // Regularly check I2C sensors.
        I2C1_M_Poll(I2C_OLED_ADDRESS);
    }
}
