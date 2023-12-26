#include <xc.h>
#include "delay.h"
#include "i2c.h"
#//include <serial.h>


#if _ENABLE_DEBUG
#define DEBUG_printf(...)	UART1_printf(__VA_ARGS__)
#else
#define DEBUG_printf(...)
#endif

//-------------------Variables-------------------
uint16_t I2Cflags;


//-------------------Functions-------------------

void I2C_Initialize()
{
	// Disable I2C controller until set up (pins will be standard IO).
	I2C1CONbits.I2CEN = 0;			

	// Set pin drive modes.
	//
	// NOTE: the I2C controller block takes over ownership of
	// the pin IO settings so these are only needed when we 
	// drop out of I2C controller mode and operate the pins
	// directly (ex: to reset the bus upon error).
	//
    LATAbits.LATA14 = 1;            // Start with the bus in idle mode - both lines high.
    LATAbits.LATA15 = 1;
	ODCAbits.ODA14 = 1;             // Open drain mode.
    ODCAbits.ODA15 = 1;
    TRISAbits.TRISA14 = 0;          // SCL1 output mode.
    TRISAbits.TRISA15 = 0;          // SDA1 output mode.

	// I2C bus is in an unknown state.  Mark it dirty to
	// indicate it needs to be cleared.
	I2Cflags = 0;
	SetI2C1BusDirty;
}


void I2C_ModuleStart()
{
	// Configure I2C1 (pic24f256gb110 pins 66 & 67) for 400kHz operation.
	I2C1CON = 0x1000;				// Set all bits to known state.
	I2C1CONbits.I2CEN = 0;			// Disable I2C controller until set up (pins will be standard IO).
	I2C1BRG = 37;					// I2C1BRG = (Fcy/Fscl-FCY/10E6)-1 = (16E6/400E3-16E6/10E6)-1 = 40-1.6-1 = 37.4 (actual Fscl=404kHz).
	I2C1CONbits.DISSLW = 0;			// Enable slew rate control for 400kHz operation to limit signal ringing.
	IFS1bits.MI2C1IF = 0;			// Clear the I2C master interrupt flag.
	I2C1CONbits.I2CEN = 1;			// Enable I2C controller.

	// NOTE: used for interrupt-driven code.
	//IEC1bits.MI2C1IE = 1;			// Enable the I2C master interrupt.
}


// Clear any errors that may have occurred.
static void I2C1_M_ClearErrors()
{
	I2C1CONbits.RCEN = 0;			// Cancel receive request.
	I2C1STATbits.IWCOL = 0;			// Clear write-collision flag.

	I2C1STATbits.BCL = 0;			// Clear bus-collision flag by disabling-enabling I2C controller if already enabled.
	if (I2C1CONbits.I2CEN == 1)
	{
		I2C1CONbits.I2CEN = 0;
		Nop();
		I2C1CONbits.I2CEN = 1;
	}
}


// Reset the I2C bus - called by I2C1_M_RecoverBus().
//
// This routine will verify that SCL can be set HIGH, then free SDA by clocking until SDA goes HIGH.
// Then the bus will be reset by issuing a START and STOP condition to restart the I2C bus.
// This should clear any devices hung on incomplete I2C transfers.
//
// NOTE: I2C controller must not be active or this routine will not work as it
// relies on using the pins as standard IO functions.
//
// Return Values:
// 	I2C_OK
//	I2C_Err_SCL_low
//	I2C_Err_SDA_low
//*******************************************************************************
static int I2C1_M_BusReset()
{
	// Start with lines high - sets SCL high if not already there.
	LATAbits.LATA14 = 1;
	LATAbits.LATA15 = 1;

	// Give the output latches time to drive the lines high (~5uS) then
	// confirm SCL goes high.  If it doesn't there's a problem.
	Delay10us(1);
	if (PORTAbits.RA14 == 0)
	{
		// SCL is stuck low - is the pullup resistor loaded?
		return I2C_Err_SCL_low;											
	}

	// SCL is confirmed to be high.  Toggle SCL until SDA goes high (exit the
	// loop with SCL high as well).
	unsigned short i=0;
	for (i=0 ; (i < 10) && (PORTAbits.RA15 == 0) ; i++)
	{
		// Toggle SCL with delays (~5uS).
		LATAbits.LATA14 = 0;
		Delay10us(1);
		LATAbits.LATA14 = 1;
		Delay10us(1);
	}

	// Confirm both SCL and SDA are high.
	if (PORTAbits.RA14 == 0 || PORTAbits.RA15 == 0)
	{
		return I2C_Err_SDA_low;
	}

	// Set SDA low while SCL is high (START).
	LATAbits.LATA15 = 0;
	Delay10us(1);

	// Set SDA high while SCL is high (STOP).
	LATAbits.LATA15 = 1;
	Delay10us(1);
	
	return I2C_OK;
}


// Attempts to recover after an I2C error.
//
// Return Values:
//	I2C_OK
//	I2C_Err_Hardware
//*******************************************************************************
static int I2C1_M_RecoverBus()
{
	// Try disabling the I2C controller and resetting devices on the I2C bus.
	// With the I2C controller disabled, we can directly control the pins
	// to reset the target device(s).
	I2C1CONbits.I2CEN = 0;

	if(I2C1_M_BusReset() < 0)
	{
		DEBUG_printf("ERROR: Failed to reset I2C bus.\r\n");
		return I2C_Err_Hardware;
	}

	// Re-enable the I2C controller.
	I2C1CONbits.I2CEN = 1;

	return I2C_OK;
}


// Initiates repeated start sequence on I2C bus (usually to switch from write to read).
// 
// This routine waits for the restart to complete before returning.
//
// Return Values:
//	I2C_OK
//	I2C_Err_BCL
//	I2C_Err_SCL_low	(NOTE: SDA stuck low cannot be detected here.)
//*******************************************************************************
static int I2C1_M_Restart()
{
	// I2C master must be in idle mode.
	if ((I2C1CON & 0x1F) != 0)
	{
		DEBUG_printf("WARN: I2C controller isn\'t in idle mode when preparing for repeated start.\r\n");
	}

	// Initiate the restart condition.
	I2C1CONbits.RSEN = 1;

	// Wait for the restart sequence to finish.  The time is around
	// one I2C clock cycles or about 40 instructions at 16MHz.
	unsigned int i=0;
	while (I2C1CONbits.RSEN == 1)
	{
		// Timed out?  Note that this can happen if SCL is stuck low.
		if (i++ > 50)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for restart to finish.\r\n");
			// NOTE: RSEN cannot be cleared in software so the I2C
			// interface must be reset after this error.
			return I2C_Err_SCL_low;
		}
	}

	// Check for I2C bus collision (ex: SDA stuck low).
	if (I2C1STATbits.BCL == 1)
	{
		DEBUG_printf("WARN: I2C controller bus collision detected on restart.\r\n");
		// Clear the error state.
		I2C1STATbits.BCL = 0;
		return I2C_Err_BCL;
	}

	return I2C_OK;
}


// Initiates start sequence on I2C bus.
//
// This routine waits for the restart to complete before returning.
//
// Return Values:
//	I2C_OK
//	I2C_Err_BCL
//	I2C_Err_IWCOL
//	I2C_Err_TimeoutHW
//*******************************************************************************
static int I2C1_M_Start()
{
	// I2C master must be in idle mode.
	if ((I2C1CON & 0x1F) != 0)
	{
		DEBUG_printf("WARN: I2C controller isn\'t in idle mode when preparing for start.\r\n");
	}

	// Initiate the start condition.
	I2C1CONbits.SEN = 1;
	Nop();

	// Check for I2c master bus collision (SCL or SDA may be stuck low).
	if (I2C1STATbits.BCL == 1)
	{
		DEBUG_printf("ERROR: I2C master bus collision when sending start.\r\n");

		// Cancel the request (will still be set if there was a previous BCL) and
		// clear the master bus collision to regain control of the I2C bus.  Note that
		// clearing BCL requires disabling-enabling the I2C controller.
		I2C1CONbits.SEN = 0;
		I2C1STATbits.BCL = 0;
		I2C1CONbits.I2CEN = 0;
		Nop();
		I2C1CONbits.I2CEN = 1;
		return I2C_Err_BCL;
	}

	// Check whether the I2C controller is busy (write collision).
	if (I2C1STATbits.IWCOL == 1)
	{
		DEBUG_printf("ERROR: I2C module busy (write collision) when sending start.\r\n");

		// Cancel the request and clear the error.
		I2C1CONbits.SEN = 0;
		I2C1STATbits.IWCOL = 0;
		return I2C_Err_IWCOL;
	}

	// Wait for the start sequence to finish.  The time is around
	// one I2C clock cycles or about 40 instructions at 16MHz.
	unsigned int i=0;
	while (I2C1CONbits.SEN == 1)
	{
		// Timed out?  Note that this can happen if SCL is stuck low.
		if (i++ > 50)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for start to finish.\r\n");
			return I2C_Err_SCL_low;
		}
	}

	// If a second start request is issued after first one, the I2C module will instead:
	//generate a stop request, clear SEN, and flag BCL.  Test again for BCL here.
	if (I2C1STATbits.BCL == 1)
	{
		DEBUG_printf("ERROR: I2C master bus collision after sending start.\r\n");

		// Clear the master bus collision to regain control of the I2C bus.  Note that
		// clearing BCL requires disabling-enabling the I2C controller.
		I2C1STATbits.BCL = 0;
		I2C1CONbits.I2CEN = 0;
		Nop();
		I2C1CONbits.I2CEN = 1;
		return I2C_Err_BCL;
	}

	return I2C_OK;
}


// Initiates stop sequence on I2C bus.
//
// This routine waits for the restart to complete before returning.
//
// Return Values:
//	I2C_OK
//	I2C_Err_BCL
//	I2C_Err_SCL_low	(NOTE: SDA stuck low cannot be detected here.)
//*******************************************************************************
static int I2C1_M_Stop()
{
	// I2C master must be in idle mode.
	if ((I2C1CON & 0x1F) != 0)
	{
		DEBUG_printf("WARN: I2C controller isn\'t in idle mode when preparing for stop.\r\n");
	}

	// Initiate the stop condition.
	I2C1CONbits.PEN = 1;
	Nop();

	// Check for I2c master bus collision (SCL or SDA may be stuck low).
	if (I2C1STATbits.BCL == 1)
	{
		DEBUG_printf("ERROR: I2C master bus collision when sending stop.\r\n");

		// Cear the master bus collision to regain control of the I2C bus.  Note that
		// clearing BCL requires disabling-enabling the I2C controller.
		I2C1STATbits.BCL = 0;
		I2C1CONbits.I2CEN = 0;
		Nop();
		I2C1CONbits.I2CEN = 1;
		return I2C_Err_BCL;
	}

	// Wait for the stop sequence to finish.  The time is around
	// one I2C clock cycles or about 40 instructions at 16MHz.
	unsigned int i=0;
	while (I2C1CONbits.PEN == 1)
	{
		// Timed out?  Note that this can happen if SCL is stuck low.
		if (i++ > 50)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for stop to finish.\r\n");
			// Note that PEN cannot be cleared in software and we need to reset I2C interface.

			return I2C_Err_SCL_low;
		}
	}

	return I2C_OK;
}


// Poll an I2C device to see if it is alive.
//
// This should be done periodically (say every 1 second).  Also handle
// error recovery of the I2C bus if needed.
//
// Return Values:
//	I2C_OK
//	I2C_Err_BadAddr
//	I2C_Err_CommFail
//	I2C_Err_Hardware
//*******************************************************************************
int  I2C1_M_Poll(uint8_t DevAddr)
{
	uint8_t SlaveAddr = (DevAddr << 1) | 0;


	// Handle cleaning the I2C bus if needed.
	if (IsI2C1BusDirty)
	{
		DEBUG_printf("INFO: Initializing I2C bus to known state.\r\n");

		// Clear any pending errors.
		I2C1_M_ClearErrors();

		if (I2C1_M_RecoverBus() != I2C_OK)
		{
			DEBUG_printf("ERROR: Failed to initialize I2C bus.\r\n");
			return I2C_Err_Hardware;
		}

		// Indicate I2C bus has been cleaned.
		ClrI2C1BusDirty;
	}

	// Cycle through a START -> write -> STOP cycle to the target.
	if (I2C1_M_Start() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C START indication on poll.\r\n");
		goto FailureExit;
	}

	int retval = I2C1_M_WriteByte(SlaveAddr);

	// Even if we have an error sending, try to close the I2C transaction.
	if (I2C1_M_Stop() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C STOP indication on poll.\r\n");
		goto FailureExit;
	}	

	// Confirm write was successful.
	if (retval == I2C_ACK)
	{
		return I2C_OK;
	}

	if (retval == I2C_Err_NAK)
	{
		DEBUG_printf("ERROR: Failed to poll I2C target (NAK received).  Check that correct address is used.\r\n");
		return I2C_Err_BadAddr;	
	}
		
	DEBUG_printf("ERROR: Failed to poll I2C target.\r\n");

FailureExit:

	// I2C bus needs to be initialized to a known good state.
	SetI2C1BusDirty;

	return I2C_Err_CommFail;
}


// Reads data from target into buffer provided.
// 
// Return Values:
//	I2C_Ok
//	I2C_Err_BadAddr
//	I2C_Err_BusDirty
//	I2C_Err_CommFail
//*******************************************************************************
int  I2C1_M_Read(uint8_t DevAddr, uint8_t SubAddr, uint16_t ByteCnt, uint8_t *buffer)
{
	int errorValue = I2C_Err_CommFail;
	uint8_t SlaveAddr = (DevAddr << 1) | 0;		// Write bit.

	
	// Skip any reads until the I2C bus is in a known state.
	if (IsI2C1BusDirty)
	{
		DEBUG_printf("WARN: I2C read request made on an uninitialized bus.\r\n");
		errorValue = I2C_Err_BusDirty;
		goto FailureExit;
	}										

	// Send START -> write -> STOP cycle to the target.
	if (I2C1_M_Start() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C START indication on read.\r\n");
		goto FailureExit;
	}

	int retval = I2C1_M_WriteByte(SlaveAddr);

	// Either a bad target address or I2C device stopped responding.
	if (retval == I2C_Err_NAK)
	{
		DEBUG_printf("ERROR: Received I2C NAK indication on read.\r\n");
		I2C1_M_Stop();
		errorValue = I2C_Err_BadAddr;
		goto FailureExit;
	}

	if (retval < 0)
	{
		DEBUG_printf("ERROR: Failed to address target I2C device on read.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}

	// Send the I2C target device register address.
	if (I2C1_M_WriteByte(SubAddr) != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to address target I2C device register on read.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}

	// Send repeated START message to switch to read mode.
	if (I2C1_M_Restart() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send repeated start indication on read.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}

	SlaveAddr = (DevAddr << 1) | 0x01;		// Read bit.

	// Read back the response from the target.
	if (I2C1_M_WriteByte(SlaveAddr) != I2C_ACK)
	{
		DEBUG_printf("ERROR: Failed to read from target device.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}
	
	// Fetch each of the response bytes from the target.
	unsigned int i=0;
	for (i=0; i < ByteCnt; i++)
	{
		// Send a NACK on the last byte so the target knows this is the end.
		retval = (i == (ByteCnt - 1) ? I2C1_M_ReadByte(I2C_M_NACK) : I2C1_M_ReadByte(I2C_M_ACK));
	
		if (retval < 0)
		{
			DEBUG_printf("ERROR: Failed to read byte %d from I2C device.\r\n", i);
			I2C1_M_Stop();
			goto FailureExit;
		}

		buffer[i] = retval;
	}

	// Send a STOP message and close the I2C transaction.
	if (I2C1_M_Stop() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C stop indication on read.\r\n");
		goto FailureExit;
	}

	return I2C_OK;

FailureExit:

	// I2C bus needs to be initialized to a known good state.
	SetI2C1BusDirty;

	return I2C_Err_CommFail;
}


// Reads data byte from target.
//
// Note that the target must already be addressed and in read mode.  This routine
// waits for completion before returning.
// 
// ** Caution ** the target can cause a timeout by clock stretching too long.
//
// Return Values:
//	0x0000-0x00FF Read value stored in low byte (returned integer will always be positive)
// 
//  Error status is indicated by negative return values:
//	I2C_Err_Overflow
//	I2C_Err_RcvTimeout 		(will happen if slave is clock stretching or if SCL suddenly shorted to ground).
//	I2C_Err_SCL_stucklow  	SDA stuck low cannot be detected here.
//*******************************************************************************
int I2C1_M_ReadByte(uint8_t ACKflag)
{
	// I2C master must be in idle mode.
	if ((I2C1CON & 0x1F) != 0)
	{
		DEBUG_printf("WARN: I2C controller isn\'t in idle mode when preparing for read byte.\r\n");
	}

	// Set the value to be transmitted to the target during the acknowledgement sequence.
	I2C1CONbits.ACKDT = (ACKflag == I2C_M_NACK ? 1 : 0);	// 1 =  NACK, 0 = ACK

	// Start a receive cycle.
	I2C1CONbits.RCEN = 1;

	// Wait for the receive buffer to indicate full (received byte).  Note
	// that we could also wait for RCEN to go low.  The time to read 8 bits
	// at ~400KHz should be relatively short.  Instruction clock runs at 16MHz
	// so that's about 320 instructions worth of time.
	unsigned int i=0;
	while (I2C1STATbits.RBF == 0)
	{
		// Timed out?
		if (i++ > 500)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for byte on read.\r\n");
			// NOTE: RCEN cannot be cleared in software so we need to 
			// reset I2C interface or wait until SCL goes high again.
			return I2C_Err_RcvTimeout;
		}
	}

	// Confirm that the receive enable bit was cleared by the hardware.
	if (I2C1CONbits.RCEN == 1)
	{
		DEBUG_printf("WARN: I2C controller receive enable still set after read byte.\r\n");
	}

	// I2C controller must ACK (or NACK) every byte so the target 
	// knows whether it's okay to send another byte.  We set the bit
	// above so trigger the acknowledgement sequence here.
	I2C1CONbits.ACKEN = 1;

	// Wait for the acknowledgement sequence to finish.  The time is roughly
	// one I2C clock cycle or about 40 instructions at 16MHz.
	i=0;
	while (I2C1CONbits.ACKEN == 1)
	{
		// Timed out?  Note that this can happen if SCL is stuck low.
		if (i++ > 50)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for ack sequence to finish on read.\r\n");
			// NOTE: ACKEN cannot be cleared in software so the I2C
			// interface must be reset after this error.
			return I2C_Err_SCL_low;
		}
	}

	// Check for overflow.  It means we received a new byte before
	// reading the last byte.
	if (I2C1STATbits.I2COV == 1)
	{
		DEBUG_printf("WARN: I2C controller overflow detected on read byte.\r\n");
		I2C1STATbits.I2COV = 0;
		return I2C_Err_Overflow;
	}

	// RBF is cleared when reading the receive register.
	return I2C1RCV;
}


// Writes data provided in buffer to target address.
//
// Return Values:
//	I2C_OK
//	I2C_Err_BadAddr
//	I2C_Err_BusDirty
//	I2C_Err_CommFail
//*******************************************************************************
int I2C1_M_Write(uint8_t DevAddr, uint8_t SubAddr, uint16_t ByteCnt, uint8_t *buffer)
{
	int errorValue = I2C_Err_CommFail;
	uint8_t SlaveAddr = (DevAddr << 1) | 0;		// Write bit.

	
	// Skip any writes until the I2C bus is in a known state.
	if (IsI2C1BusDirty)
	{
		DEBUG_printf("WARN: I2C write request made on an uninitialized bus.\r\n");
		errorValue = I2C_Err_BusDirty;
		goto FailureExit;
	}										

	// Send START -> write -> STOP cycle to the target.
	if (I2C1_M_Start() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C START indication on write.\r\n");
		goto FailureExit;
	}

	int retval = I2C1_M_WriteByte(SlaveAddr);

	// Either a bad target address or I2C device stopped responding.
	if (retval == I2C_Err_NAK)
	{
		DEBUG_printf("ERROR: Received I2C NAK indication on write.\r\n");
		I2C1_M_Stop();
		errorValue = I2C_Err_BadAddr;
		goto FailureExit;
	}

	if (retval < 0)
	{
		DEBUG_printf("ERROR: Failed to address target I2C device on write.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}

	// Send the I2C target device register address.
	if (I2C1_M_WriteByte(SubAddr) != I2C_ACK)
	{
		DEBUG_printf("ERROR: Failed to address target I2C device register on write.\r\n");
		I2C1_M_Stop();
		goto FailureExit;
	}
	
	// Write each of the buffer's bytes to the target.
	unsigned int i=0;
	for (i=0; i < ByteCnt; i++)
	{
		if (I2C1_M_WriteByte(buffer[i]) != I2C_ACK)
		{			
			DEBUG_printf("ERROR: Failed to write byte %d to I2C device.\r\n", i);
			I2C1_M_Stop();
			goto FailureExit;
		}
	}

	// Send a STOP message and close the I2C transaction.
	if (I2C1_M_Stop() != I2C_OK)
	{
		DEBUG_printf("ERROR: Failed to send I2C stop indication on write.\r\n");
		goto FailureExit;
	}

	return I2C_OK;

FailureExit:

	// I2C bus needs to be initialized to a known good state.
	SetI2C1BusDirty;

	return I2C_Err_CommFail;
}


// Sends a byte to a target device.
//
// Note that the target must already be addressed and in write mode.  This routine
// waits for completion before returning.
//
// Return Values:
//	I2C_ACK
//	I2C_Err_BCL
//	I2C_Err_NAK
//	I2C_Err_SCL_low
//	I2C_Err_TBF
//*******************************************************************************
int I2C1_M_WriteByte(uint8_t cData)
{
	// Check whether there's already a byte waiting to be sent.
	if (I2C1STATbits.TBF == 1)
	{
		DEBUG_printf("WARN: I2C controller transmit buffer full on write byte.\r\n");
		return I2C_Err_TBF;
	}

	// Send the byte.
	I2C1TRN = cData;
	
	// Wait for the transmit status bit to go low.  The time to write 8 bits
	// at ~400KHz should be relatively short.  Instruction clock runs at 16MHz
	// so that's about 320 instructions worth of time.
	unsigned int i=0;
	while (I2C1STATbits.TRSTAT == 1)
	{
		// Timed out?
		if (i++ > 500)
		{
			DEBUG_printf("WARN: I2C controller timed out waiting for transmit indication on write byte.\r\n");
			// NOTE: this is bad because TRSTAT will still be set.
			return I2C_Err_SCL_low;
		}
	}

	// Check for master bus collision.
	if (I2C1STATbits.BCL == 1)
	{
		DEBUG_printf("ERROR: I2C master bus collision when writing byte.\r\n");

		// Cear the master bus collision to regain control of the I2C bus.  Note that
		// clearing BCL requires disabling-enabling the I2C controller.
		I2C1STATbits.BCL = 0;
		I2C1CONbits.I2CEN = 0;
		Nop();
		I2C1CONbits.I2CEN = 1;
		return I2C_Err_BCL;
	}

	// Finished sending byte.  Check out the target device responded.
	if (I2C1STATbits.ACKSTAT == 1)
	{
		DEBUG_printf("WARN: I2C target responsed with a NAK on write byte.\r\n");
		return I2C_Err_NAK;
	}

	// Otherwise the target device responded with an ACK.
	return I2C_ACK;
}
