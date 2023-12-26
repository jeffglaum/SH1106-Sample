#pragma once


//--------------------Constants--------------------
// Used for I2C1_M_RcvByte().
#define I2C_M_ACK	        0
#define I2C_M_NACK	        1

// Return values.  Errors must be negative.
#define I2C_OK				0		//Success
#define I2C_ACK				0		//ACK, same as OK
#define I2C_Err_Hardware	-1		//Hardware error with I2C bus, inspect PCB
#define I2C_Err_SCL_low		-2		//Clock line stuck low - HW problem on PCB, or I2C slave holding line low
#define I2C_Err_SDA_low		-3		//Data line stuck low
#define I2C_Err_BCL			-4		//Bus collision detected during master operation. Reset I2C interface.
#define I2C_Err_IWCOL		-5		//Attempted to write to I2CxTRN while byte still qued for sending
#define I2C_Err_NAK			-6		//Slave refused/ignored byte sent by master - could re-send byte
#define I2C_Err_TBF			-7		//Transmit buffer full - a byte already qued for sending. Indicates programming error.
#define I2C_Err_Overflow	-9		//Received new byte without processing previous one
#define I2C_Err_RcvTimeout	-10		//Timeout while waiting for byte from slave
#define I2C_Err_BusDirty	-100	//Need to reset I2C bus before high level routines will work
#define I2C_Err_TimeoutHW	-101	//Timeout, unknown reason
#define I2C_Err_CommFail	-102	//General communications failure
#define I2C_Err_BadAddr		-103	//Bad device address or device stopped responding


//--------------------Variables--------------------
extern uint16_t I2Cflags;


//--------------------Functions--------------------
void I2C_Initialize(void);
void I2C_ModuleStart(void);
int  I2C1_M_Poll(uint8_t);
int  I2C1_M_Read(uint8_t, uint8_t, uint16_t, uint8_t *);
int  I2C1_M_ReadByte(uint8_t);
int  I2C1_M_Write(uint8_t, uint8_t, uint16_t, uint8_t *);
int  I2C1_M_WriteByte(uint8_t);


//-------------------Macros-------------------
#define SetI2C1BusDirty	I2Cflags |=  0x0001
#define ClrI2C1BusDirty	I2Cflags &= ~0x0001
#define IsI2C1BusDirty	(I2Cflags & 0x0001)
