/***************************************************************************//**
  @file     LEDs.c
  @brief    LEDs driver
  @author   Group 4
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "LEDs.h"
#include "board.h"
#include "gpio.h"
//#include "pisr.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define NO_LED 0b00
#define LED1 0b11
#define LED2 0b01
#define LED3 0b10


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Refresh the LEDS
 */
void LEDS_refresh(void);

/**
 * @brief Turns on only one LED
 * @param ledNumber which LED to turn on
 */
void LEDS_On(uint8_t ledNumber);


/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint8_t ledStatus;
static uint8_t counter;


/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

bool LEDS_Init(void)
{
    gpioWrite(PIN_LED_EXT_STATUS0, LOW);
    gpioWrite(PIN_LED_EXT_STATUS0, LOW);
    gpioMode(PIN_LED_EXT_STATUS0, OUTPUT);
    gpioMode(PIN_LED_EXT_STATUS1, OUTPUT);
    
//    pisrRegister(LEDS_refresh, 100);    // 7 ms

    return true;
}

void LEDS_Set(uint8_t num)
{
//    if((num <= 7) && (num >= 0))
//        ledStatus = num;
//    else
//        ledStatus = 0b000;
	if(num == 0b000)
	{
		gpioWrite(PIN_LED_EXT_STATUS0, LOW);
		gpioWrite(PIN_LED_EXT_STATUS1, LOW);
	}
	else if (num == 0b001) {
		gpioWrite(PIN_LED_EXT_STATUS0, LOW);
		gpioWrite(PIN_LED_EXT_STATUS1, HIGH);
	} else if (num == 0b010) {
		gpioWrite(PIN_LED_EXT_STATUS0, HIGH);
		gpioWrite(PIN_LED_EXT_STATUS1, LOW);
	} else if (num == 0b100){
		gpioWrite(PIN_LED_EXT_STATUS0, HIGH);
		gpioWrite(PIN_LED_EXT_STATUS1, HIGH);
	} else {
		gpioWrite(PIN_LED_EXT_STATUS0, LOW);
		gpioWrite(PIN_LED_EXT_STATUS1, LOW);
	}
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void LEDS_On(uint8_t ledNumber)
{
    gpioWrite(PIN_LED_EXT_STATUS0, ledNumber & 0b01);
    gpioWrite(PIN_LED_EXT_STATUS1, ledNumber & 0b10);
}

void LEDS_refresh(void)
{
    if((ledStatus & 0b100) && (counter == 0))
        LEDS_On(LED3);
    else if((ledStatus & 0b010) && (counter == 1))
        LEDS_On(LED2);
    else if((ledStatus & 0b001) && (counter == 2))
        LEDS_On(LED1);
    else
        LEDS_On(NO_LED);
    counter++;
}


/******************************************************************************/
