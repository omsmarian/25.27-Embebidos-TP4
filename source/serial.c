/***************************************************************************//**
  @file     serial.c
  @brief    Serial communication driver for K64F, using UART
  @author   Group 4: - Oms, Mariano
                     - Solari Raigoso, Agustín
                     - Wickham, Tomás
                     - Vieira, Valentin Ulises
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board.h"
#include "uart.h"
//#include "pisr.h"
#include "cqueue.h"
#include "timer.h"
#include "serial.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DEVELOPMENT_MODE			1

#define SERIAL_PORT					UART0_ID

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

// Main Services ///////////////////////////////////////////////////////////////

bool serialInit (void)
{
	uart_cfg_t config = {SERIAL_DEFAULT_BAUDRATE,
						 UART_MODE_8,
						 UART_PARITY_NONE,
						 UART_STOPS_1,
						 UART_RX_TX_ENABLED,
						 UART_FIFO_RX_TX_ENABLED};

	return uartInit(SERIAL_PORT, config);
}

bool serialWriteData (uchar_t* data, uint8_t len)
{
	return len == uartWriteMsg(SERIAL_PORT, data, len);
}

uchar_t* serialReadData (uint8_t* len)
{
	static unsigned char data[QUEUE_MAX_SIZE];

	*len = uartReadMsg(SERIAL_PORT, data, uartGetRxMsgLength(SERIAL_PORT));

	return data;
}

bool serialWriteStatus (void)
{
	return uartIsTxMsgComplete(SERIAL_PORT);
}

bool serialReadStatus (void)
{
	return uartIsRxMsg(SERIAL_PORT);
}

uint8_t serialReadStatusLength (void)
{
	return uartGetRxMsgLength(SERIAL_PORT);
}

/******************************************************************************/
