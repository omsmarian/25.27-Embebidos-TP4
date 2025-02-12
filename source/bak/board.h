/***************************************************************************//**
  @file     board.h
  @brief    Board management
  @author   Group 4: - Oms, Mariano
                     - Solari Raigoso, Agustín
                     - Wickham, Tomás
                     - Vieira, Valentin Ulises
  @note     Based on the work of Nicolás Magliola
 ******************************************************************************/

#ifndef _BOARD_H_
#define _BOARD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/***** BOARD defines **********************************************************/

// On Board User LEDs
#define PIN_LED_RED				PORTNUM2PIN(PB, 22)	// PTB22
#define PIN_LED_GREEN			PORTNUM2PIN(PE, 26)	// PTE26
#define PIN_LED_BLUE			PORTNUM2PIN(PB, 21)	// PTB21

#define LED_ACTIVE      		LOW

// On Board User Switches
#define PIN_SW2					PORTNUM2PIN(PC, 6)	// PTC6
#define PIN_SW3					PORTNUM2PIN(PA, 4)	// PTA4

#define SW_ACTIVE				LOW
#define SW_INPUT_TYPE			INPUT_PULLUP

// External User LEDs
#define PIN_LED_EXT_STATUS0		PORTNUM2PIN(PB, 20) // PTA0
#define PIN_LED_EXT_STATUS1		PORTNUM2PIN(PB, 18) // PTA0

// Encoder
#define PIN_ENCODER_RCHA		PORTNUM2PIN(PE, 24)	// PTC12
#define PIN_ENCODER_RCHB		PORTNUM2PIN(PE, 25)	// PTC4
#define PIN_ENCODER_RSWITCH		PORTNUM2PIN(PE, 26) // PTD0

// Display
#define PIN_DISPLAY_CSEGA		PORTNUM2PIN(PD, 1)	// PTA0
#define PIN_DISPLAY_CSEGB		PORTNUM2PIN(PD, 3) 	// PTA0
#define PIN_DISPLAY_CSEGC		PORTNUM2PIN(PD, 2) 	// PTA0
#define PIN_DISPLAY_CSEGD		PORTNUM2PIN(PD, 0)	// PTA0
#define PIN_DISPLAY_CSEGE		PORTNUM2PIN(PC, 4)	// PTA0

#define PIN_DISPLAY_CSEGF		PORTNUM2PIN(PC, 3)	// PTA0
#define PIN_DISPLAY_CSEGG		PORTNUM2PIN(PC, 2) 	// PTA0
#define PIN_DISPLAY_CSEGDP		PORTNUM2PIN(PA, 2)	// PTA0

#define PIN_DISPLAY_SEL0		PORTNUM2PIN(PC, 16)	// PTA0
#define PIN_DISPLAY_SEL1		PORTNUM2PIN(PC, 17)	// PTA0

// Magnetic Stripe Card Reader
#define PIN_MAGCARD_ENABLE		PORTNUM2PIN(PB, 23)	// PTB9
#define PIN_MAGCARD_CLOCK		PORTNUM2PIN(PA, 1)	// PTA1
#define PIN_MAGCARD_DATA		PORTNUM2PIN(PB, 9)	// PTB23

#define PIN_TP_PER				PORTNUM2PIN(PC, 9)
#define PIN_TP_DED              PORTNUM2PIN(PC, 5)

// UART
#define UART0_RX_PIN			PORTNUM2PIN(PB, 16)	// PTA1
#define UART0_TX_PIN			PORTNUM2PIN(PB, 17)	// PTA2
#define UART1_RX_PIN			PORTNUM2PIN(PE, 1)	// PTC3
#define UART1_TX_PIN			PORTNUM2PIN(PE, 0)	// PTC4
#define UART2_RX_PIN			PORTNUM2PIN(PD, 2)	// PTD2
#define UART2_TX_PIN			PORTNUM2PIN(PD, 3)	// PTD3
#define UART3_RX_PIN			PORTNUM2PIN(PE, 5)	// PTC16
#define UART3_TX_PIN			PORTNUM2PIN(PE, 4)	// PTC17
#define UART4_RX_PIN			PORTNUM2PIN(PE, 25)	// PTC14
#define UART4_TX_PIN			PORTNUM2PIN(PE, 24)	// PTC15
#define UART5_RX_PIN			PORTNUM2PIN(PE, 9)	// PTD8
#define UART5_TX_PIN			PORTNUM2PIN(PE, 8)	// PTC9

// FXOS8700CQ Sensor
#define FXOS8700CQ_I2C_SCL_PIN	PORTNUM2PIN(PE, 24)	// PTB0
#define FXOS8700CQ_I2C_SDA_PIN	PORTNUM2PIN(PE, 25)	// PTB1
#define FXOS8700CQ_INT1_PIN		PORTNUM2PIN(PC, 6)	// PTC6
#define FXOS8700CQ_INT2_PIN		PORTNUM2PIN(PC, 13)	// PTC6

/*******************************************************************************
 ******************************************************************************/

#endif // _BOARD_H_
